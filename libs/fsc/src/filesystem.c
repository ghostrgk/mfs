#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "fsc/internal/filesystem.h"
#include "fsc/internal/logging.h"
#include "fsc/internal/compiler.h"
#include "fsc/internal/string.h"

#define FSC_HANDLE_ERROR(msg) \
  do {                        \
    perror(msg);              \
    abort();                  \
  } while (false)

static int initRootInode(int ffile_fd, struct SuperBlock* superblock) {
  uint8_t* ffile_content = mmap(NULL, FileSystemSize(superblock), PROT_WRITE | PROT_READ, MAP_SHARED, ffile_fd, 0);

  if (ffile_content == MAP_FAILED) {
    perror("Can't mmap ffile for root inode initializing");
    return -1;
  }

  struct SuperBlock* superblock_ptr = (struct SuperBlock*)(ffile_content);
  --(superblock_ptr->free_inode_num);

  Inode* root_inode_ptr = (Inode*)(ffile_content + InodesOffset(superblock));
  root_inode_ptr->is_dir = true;

  struct BitSet inode_bitset;
  BitSet_init(&inode_bitset, superblock->inode_num, ffile_content + InodeBitSetOffset(superblock));
  setBit(&inode_bitset, 0);

  if (munmap(ffile_content, FileSystemSize(superblock)) == -1) {
    perror("munmap ffile failed");
    return -1;
  }

  return 0;
}

int FileSystem_init(struct FileSystem* fs, const char* ffile_path) {
  fs->fd_ = open(ffile_path, O_RDWR);
  if (fs->fd_ == -1) {
    if (errno != ENOENT) {
      FSC_HANDLE_ERROR("Can't open ffile");
    }

    FSC_LOG("FSM", "No ffile found. Creating default ffile.");

    fs->fd_ = open(ffile_path, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fs->fd_ == -1) {
      FSC_HANDLE_ERROR("Can't open ffile");
    }

    struct SuperBlock super_block = {.block_num = DEFAULT_BLOCK_COUNT,
                                     .free_block_num = super_block.block_num,
                                     .inode_num = DEFAULT_INODE_COUNT,
                                     .free_inode_num = super_block.inode_num};

    if (ftruncate(fs->fd_, (off_t)FileSystemSize(&super_block)) == -1) {
      FSC_HANDLE_ERROR("Truncation failed");
    }

    struct SuperBlock* file_start =
        mmap(NULL, sizeof(struct SuperBlock), PROT_WRITE | PROT_READ, MAP_SHARED, fs->fd_, 0);
    if (file_start == MAP_FAILED) {
      FSC_HANDLE_ERROR("Can't mmap truncated file");
    }

    *file_start = super_block;
    if (munmap(file_start, sizeof(struct SuperBlock)) == -1) {
      FSC_HANDLE_ERROR("munmap truncated file failed");
    }

    if (initRootInode(fs->fd_, &super_block) < 0) {
      abort();
    }
  }

  struct SuperBlock* file_start = mmap(NULL, sizeof(struct SuperBlock), PROT_READ, MAP_SHARED, fs->fd_, 0);
  if (file_start == MAP_FAILED) {
    FSC_HANDLE_ERROR("Can't mmap metadata for read");
  }

  struct SuperBlock super_block = *file_start;
  if (munmap(file_start, sizeof(struct SuperBlock)) == -1) {
    FSC_HANDLE_ERROR("munmap metadata failed");
  }

  FSC_LOG("FSM", "Initializing fsm object.");
  fs->file_bytes_ = mmap(NULL, FileSystemSize(&super_block), PROT_WRITE | PROT_READ, MAP_SHARED, fs->fd_, 0);

  if (fs->file_bytes_ == MAP_FAILED) {
    FSC_HANDLE_ERROR("Can't mmap ffile");
  }

  fs->super_block_ptr_ = (struct SuperBlock*)(fs->file_bytes_);

  struct BitSet blocks_bitset;
  BitSet_init(&blocks_bitset, fs->super_block_ptr_->block_num,
              fs->file_bytes_ + BlockBitSetOffset(fs->super_block_ptr_));
  Blocks_init(&fs->blocks_, fs->file_bytes_ + BlocksOffset(fs->super_block_ptr_), &fs->super_block_ptr_->free_block_num,
              blocks_bitset);

  struct BitSet inodes_bitset;
  BitSet_init(&inodes_bitset, fs->super_block_ptr_->inode_num,
              fs->file_bytes_ + InodeBitSetOffset(fs->super_block_ptr_));
  Inodes_init(&fs->inodes_, fs->file_bytes_ + InodesOffset(fs->super_block_ptr_), &fs->blocks_,
              &fs->super_block_ptr_->free_inode_num, inodes_bitset);

  return 0;
}

void FileSystem_free(struct FileSystem* fs) {
  FSC_LOG("FSM", "Destroying fsm object.");
  if (munmap(fs->file_bytes_, FileSystemSize(fs->super_block_ptr_)) == -1) {
    FSC_HANDLE_ERROR("Can't unmap ffile");
  }

  close(fs->fd_);
}

int createFDE(struct FileSystem* fs, const char* fde_path, bool is_dir) {
  if (existsFDE(fs, fde_path)) {
    return -1;
  }

  if (fde_path[0] != '/') {
    return -1;
  }

  if (fde_path[strlen(fde_path) - 1] == '/') {
    return -1;
  }

  const char* next_name_ptr = fde_path + 1;
  const char* next_name_end = strchr(next_name_ptr, '/');

  uint64_t current_inode_id = 0;
  while (next_name_end != NULL) {
    uint64_t next_name_len = next_name_end - next_name_ptr;

    char* name = malloc(next_name_len + 1);

    memcpy(name, next_name_ptr, next_name_len);
    name[next_name_len] = 0;

    uint64_t child_id = 0;
    Inode* current_inode = getInodeById(&fs->inodes_, current_inode_id);
    if (!existsChild(fs, current_inode, name)) {
      if (createChild(fs, current_inode, name, true) < 0) {
        free(name);
        return -1;
      }
    }

    if (getChildId(fs, current_inode, name, &child_id) < 0) {
      free(name);
      return -1;
    }

    free(name);

    if (!getInodeById(&fs->inodes_, child_id)->is_dir) {
      return -1;
    }

    current_inode_id = child_id;
    next_name_ptr = next_name_end + 1;
    next_name_end = strchr(next_name_ptr, '/');
  }

  Inode* current_inode = getInodeById(&fs->inodes_, current_inode_id);
  if (!existsChild(fs, current_inode, next_name_ptr)) {
    if (createChild(fs, current_inode, next_name_ptr, is_dir) < 0) {
      return -1;
    }
  }

  return 0;
}

int getFDEInodeId(struct FileSystem* fs, const char* fde_path, uint64_t* result_ptr) {
  if (strcmp(fde_path, "/") == 0) {
    *result_ptr = 0;
    return 0;
  }

  if (fde_path[0] != '/') {
    return -1;
  }

  if (fde_path[strlen(fde_path) - 1] == '/') {
    return -1;
  }

  const char* next_name_ptr = fde_path + 1;
  const char* next_name_end = strchr(next_name_ptr, '/');

  uint64_t current_inode_id = 0;
  while (next_name_end != NULL) {
    uint64_t next_name_len = next_name_end - next_name_ptr;

    char* name = malloc(next_name_len + 1);

    memcpy(name, next_name_ptr, next_name_len);
    name[next_name_len] = 0;

    uint64_t child_id = 0;
    if (getChildId(fs, getInodeById(&fs->inodes_, current_inode_id), name, &child_id) < 0) {
      free(name);
      return -1;
    }

    free(name);

    if (!getInodeById(&fs->inodes_, child_id)->is_dir) {
      return -1;
    }

    current_inode_id = child_id;
    next_name_ptr = next_name_end + 1;
    next_name_end = strchr(next_name_ptr, '/');
  }

  uint64_t child_id = 0;
  if (getChildId(fs, getInodeById(&fs->inodes_, current_inode_id), next_name_ptr, &child_id) < 0) {
    return -1;
  }

  *result_ptr = child_id;
  return 0;
}

bool existsFDE(struct FileSystem* fs, const char* fde_path) {
  uint64_t inode_id;
  return getFDEInodeId(fs, fde_path, &inode_id) >= 0;
}

int deleteFDE(struct FileSystem* fs, const char* fde_path, bool is_dir) {
  if (strcmp(fde_path, "/") == 0) {
    return -1;
  }

  uint64_t inode_id;
  if (getFDEInodeId(fs, fde_path, &inode_id) < 0 || getInodeById(&fs->inodes_, inode_id)->is_dir != is_dir) {
    return -1;
  }

  if (FS_deleteInode(fs, inode_id) < 0) {
    return -1;
  }

  uint64_t parent_inode_id;
  int rc = getFDEInodeParentId(fs, fde_path, &parent_inode_id);
  assert(rc >= 0);
  FSC_USED_BY_ASSERT(rc);

  Inode* parent_inode = getInodeById(&fs->inodes_, parent_inode_id);
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode->file_size; ++i) {
    Link link;
    FS_read(fs, parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (link.inode_id == inode_id) {
      assert(link.is_alive);
      link.is_alive = false;
      if (FS_write(fs, parent_inode, &link, i * sizeof(Link), sizeof(Link)) < 0) {
        return -1;
      }

      break;
    }
  }

  return 0;
}

Inode* FS_getInodeById(struct FileSystem* fs, uint64_t inode_id) {
  return getInodeById(&fs->inodes_, inode_id);
}

int createChild(struct FileSystem* fs, Inode* parent_inode, const char* name, bool is_dir) {
  if (strlen(name) > MAX_LINK_NAME_LEN) {
    return -1;
  }

  if (existsChild(fs, parent_inode, name)) {
    return -1;
  }

  if (addDirectoryEntry(&fs->inodes_, parent_inode, name, is_dir) < 0) {
    return -1;
  }

#ifdef REDUNDANT_CHECKS
  uint64_t child_id;
  int rc = getChildId(fs, parent_inode, name, &child_id);
  assert(rc >= 0);
  FSC_USED_BY_ASSERT(rc);
#endif

  return 0;
}

int getChildId(struct FileSystem* fs, Inode* parent_inode, const char* name, uint64_t* result_ptr) {
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode->file_size; ++i) {
    Link link;
    Inode_read(&fs->inodes_, parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (strcmp(link.name, name) == 0 && link.is_alive) {
      *result_ptr = link.inode_id;
      return 0;
    }
  }

  return -1;
}

bool existsChild(struct FileSystem* fs, Inode* parent_inode, const char* name) {
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode->file_size; ++i) {
    Link link;
    Inode_read(&fs->inodes_, parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (strcmp(link.name, name) == 0 && link.is_alive) {
      return true;
    }
  }
  return false;
}

int FS_read(struct FileSystem* fs, Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) {
  return Inode_read(&fs->inodes_, inode_ptr, buffer, offset, count);
}

int FS_write(struct FileSystem* fs, Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count) {
  return Inode_write(&fs->inodes_, inode_ptr, buffer, offset, count);
}

int FS_append(struct FileSystem* fs, Inode* inode_ptr, const void* buffer, uint64_t count) {
  return Inode_append(&fs->inodes_, inode_ptr, buffer, count);
}

int FS_listDir(struct FileSystem* fs, const char* dir_path, char** output) {
  uint64_t inode_id;
  if (getFDEInodeId(fs, dir_path, &inode_id) < 0 || !getInodeById(&fs->inodes_, inode_id)->is_dir) {
    return -1;
  }

  struct string str;
  string_init(&str);

  Inode* inode = getInodeById(&fs->inodes_, inode_id);

  for (uint64_t i = 0; i * sizeof(Link) < inode->file_size; ++i) {
    Link link;
    FS_read(fs, inode, &link, i * sizeof(Link), sizeof(Link));
    if (link.is_alive) {
      string_append(&str, link.name);
      string_append(&str, ((getInodeById(&fs->inodes_, link.inode_id)->is_dir) ? "/ " : " "));
    }
  }

  *output = str.data;

  return 0;
}

int getFDEInodeParentId(struct FileSystem* fs, const char* fde_path, uint64_t* result_ptr) {
  if (strcmp(fde_path, "/") == 0) {
    return -1;
  }

  if (fde_path[0] != '/') {
    return -1;
  }

  if (fde_path[strlen(fde_path) - 1] == '/') {
    return -1;
  }

  const char* next_name_ptr = fde_path + 1;
  const char* next_name_end = strchr(next_name_ptr, '/');

  uint64_t current_inode_id = 0;
  while (next_name_end != NULL) {
    uint64_t next_name_len = next_name_end - next_name_ptr;

    char* name = malloc(next_name_len + 1);

    memcpy(name, next_name_ptr, next_name_len);
    name[next_name_len] = 0;

    uint64_t child_id = 0;
    if (getChildId(fs, getInodeById(&fs->inodes_, current_inode_id), name, &child_id) < 0) {
      free(name);
      return -1;
    }

    free(name);

    if (!getInodeById(&fs->inodes_, child_id)->is_dir) {
      return -1;
    }

    current_inode_id = child_id;
    next_name_ptr = next_name_end + 1;
    next_name_end = strchr(next_name_ptr, '/');
  }

  *result_ptr = current_inode_id;
  return 0;
}

int FS_deleteInode(struct FileSystem* fs, uint64_t inode_id) {
  deleteInode(&fs->inodes_, inode_id);
  return 0;
}
