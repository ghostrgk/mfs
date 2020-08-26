#include "fs++/filesystem.h"

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <cstring>
#include <regex>

#include "fs++/internal/logging.h"
#include "fs++/internal/config.h"

#define FSPP_HANDLE_ERROR(msg) \
  do {                         \
    perror(msg);               \
    std::abort();              \
  } while (false)

// filesystem file layout
// | superblock | inode_bitset | block_bitset | inodes | blocks |

namespace fspp {

namespace in = internal;
using internal::Link;

static int initRootInode(int ffile_fd, in::SuperBlock& superblock) {
  auto* ffile_content = static_cast<uint8_t*>(
      mmap64(nullptr, superblock.FileSystemSize(), PROT_WRITE | PROT_READ, MAP_SHARED, ffile_fd, 0));

  if (ffile_content == MAP_FAILED) {
    perror("Can't mmap ffile for root inode initializing");
    return -1;
  }

  auto* superblock_ptr = reinterpret_cast<in::SuperBlock*>(ffile_content);
  --(superblock_ptr->free_inode_num);

  auto* root_inode_ptr = reinterpret_cast<in::Inode*>(ffile_content + superblock.InodesOffset());
  root_inode_ptr->is_dir = true;
  root_inode_ptr->inodes_list.setSize(0);
  root_inode_ptr->blocks_count = 0;
  root_inode_ptr->file_size = 0;

  in::BitSet inode_bitset(superblock.inode_num, ffile_content + superblock.InodeBitSetOffset());
  inode_bitset.setBit(0);

  if (munmap(ffile_content, superblock.FileSystemSize()) == -1) {
    perror("munmap ffile failed");
    return -1;
  }

  return 0;
}

// todo: remove abort
FileSystem::FileSystem(const std::string& ffile_path) {
  int fd = open(ffile_path.c_str(), O_RDWR);
  if (fd == -1) {
    if (errno != ENOENT) {
      FSPP_HANDLE_ERROR("Can't open ffile");
    }

    FSPP_LOG("No ffile found.\nCreating default ffile.");

    fd = open(ffile_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fd == -1) {
      FSPP_HANDLE_ERROR("Can't open ffile");
    }

    in::SuperBlock super_block = {.block_num = DEFAULT_BLOCK_COUNT,
                                  .free_block_num = super_block.block_num,
                                  .inode_num = DEFAULT_INODE_COUNT,
                                  .free_inode_num = super_block.inode_num};

    if (ftruncate(fd, super_block.FileSystemSize()) == -1) {
      FSPP_HANDLE_ERROR("Truncation failed");
    }
    auto* file_start = static_cast<in::SuperBlock*>(
        mmap64(nullptr, sizeof(in::SuperBlock), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0));
    if (file_start == MAP_FAILED) {
      FSPP_HANDLE_ERROR("Can't mmap truncated file");
    }

    *file_start = super_block;
    if (munmap(file_start, sizeof(in::SuperBlock)) == -1) {
      FSPP_HANDLE_ERROR("munmap truncated file failed");
    }

    if (initRootInode(fd, super_block) < 0) {
      std::abort();
    }
  }

  auto* file_start =
      static_cast<in::SuperBlock*>(mmap64(nullptr, sizeof(in::SuperBlock), PROT_READ, MAP_SHARED, fd, 0));
  if (file_start == MAP_FAILED) {
    FSPP_HANDLE_ERROR("Can't mmap metadata for read");
  }

  in::SuperBlock super_block = *file_start;
  if (munmap(file_start, sizeof(in::SuperBlock)) == -1) {
    FSPP_HANDLE_ERROR("munmap metadata failed");
  }

  file_bytes_ =
      static_cast<uint8_t*>(mmap64(nullptr, super_block.FileSystemSize(), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0));

  if (file_bytes_ == MAP_FAILED) {
    FSPP_HANDLE_ERROR("Can't mmap ffile");
  }

  super_block_ptr_ = reinterpret_cast<internal::SuperBlock*>(file_bytes_);
  blocks_ = in::BlockSpace(&super_block_ptr_->block_num, &super_block_ptr_->free_block_num,
                           file_bytes_ + super_block_ptr_->BlockBitSetOffset(),
                           reinterpret_cast<internal::Block*>(file_bytes_ + super_block_ptr_->BlocksOffset()));
  inodes_ = in::InodeSpace(&super_block_ptr_->inode_num, &super_block_ptr_->free_inode_num,
                           file_bytes_ + super_block_ptr_->InodeBitSetOffset(), &blocks_,
                           reinterpret_cast<internal::Inode*>(file_bytes_ + super_block_ptr_->InodesOffset()));
}

FileSystem::~FileSystem() {
  if (munmap(file_bytes_, super_block_ptr_->FileSystemSize()) == -1) {
    FSPP_HANDLE_ERROR("Can't unmap ffile");
  }
}

bool FileSystem::existsDir(const std::string& dir_path) {
  return false;
}

int FileSystem::createDir(const std::string& dir_path) {
  return -1;
}

int FileSystem::createDir(const std::string& parent_path, const std::string& dir_name) {
  return -1;
}

// file leak todo:fix
int FileSystem::deleteDir(const std::string& dir_path) {
  if (!existsDir(dir_path)) {
    return -1;
  }

  uint64_t inode_id;
  if (getInodeIdByPath(dir_path, &inode_id) < 0) {
    return -1;
  }

  inodes_.deleteInode(inode_id);

  return 0;
}

bool FileSystem::existsFile(const std::string& file_path) {
  return false;
}

int FileSystem::createFile(const std::string& file_path) {
  if (file_path == "/") {
    return 0;
  }

  assert(file_path[0] == '/');
  assert(file_path[file_path.size() - 1] != '/');

  const char* next_name_ptr = file_path.c_str() + 1;
  const char* next_name_end = strchr(next_name_ptr, '/');

  uint64_t current_inode_id = 0;
  while (next_name_end != nullptr) {
    uint64_t next_name_len = next_name_end - next_name_ptr;

    char* name = new char[next_name_len + 1];

    strncpy(name, next_name_ptr, next_name_len);
    name[next_name_len] = 0;

    uint64_t child_id = 0;
    in::Inode& current_inode = inodes_.getInodeById(current_inode_id);
    if (!existsChild(current_inode, name)) {
      if (createChild(current_inode, name, true) < 0) {
        delete[] name;
        return -1;
      }
    }

    if (getChildId(current_inode, std::string(name), &child_id) < 0) {
      delete[] name;
      return -1;
    }

    delete[] name;

    current_inode_id = child_id;
    next_name_ptr = next_name_end + 1;
    next_name_end = strchr(next_name_ptr, '/');
  }

  in::Inode& current_inode = inodes_.getInodeById(current_inode_id);
  if (!existsChild(current_inode, next_name_ptr)) {
    if (createChild(current_inode, next_name_ptr, false) < 0) {
      return -1;
    }
  }

  return 0;
}

int FileSystem::deleteFile(const std::string& file_path) {
  if (!existsFile(file_path)) {
    return -1;
  }

  uint64_t inode_id;
  if (getInodeIdByPath(file_path, &inode_id) < 0) {
    return -1;
  }

  inodes_.deleteInode(inode_id);

  return 0;
}

int FileSystem::readFileContent(const std::string& file_path, uint64_t offset, void* buffer, uint64_t size) {
  uint64_t inode_id;
  if (getInodeIdByPath(file_path, &inode_id) < 0) {
    return -1;
  }

  return inodes_.read(&inodes_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystem::writeFileContent(const std::string& file_path, uint64_t offset, const void* buffer, uint64_t size) {
  uint64_t inode_id;
  if (getInodeIdByPath(file_path, &inode_id) < 0) {
    return -1;
  }

  return inodes_.write(&inodes_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystem::getInodeIdByPath(std::string path, uint64_t* result_ptr) {
  if (path == "/") {
    *result_ptr = 0;
    return 0;
  }

  assert(path[0] == '/');
  assert(path[path.size() - 1] != '/');

  const char* next_name_ptr = path.c_str() + 1;
  const char* next_name_end = strchr(next_name_ptr, '/');

  uint64_t current_inode_id = 0;
  while (next_name_ptr != nullptr) {
    uint64_t next_name_len = next_name_end - next_name_ptr;

    char* name = new char[next_name_len + 1];

    strncpy(name, next_name_ptr, next_name_len + 1);

    uint64_t child_id = 0;
    if (getChildId(inodes_.getInodeById(current_inode_id), std::string(name), &child_id) < 0) {
      delete[] name;
      return -1;
    }

    delete[] name;

    current_inode_id = child_id;
    next_name_ptr = next_name_end + 1;
    next_name_end = strchr(next_name_ptr, '/');
  }

  uint64_t child_id = 0;
  if (getChildId(inodes_.getInodeById(current_inode_id), std::string(next_name_ptr), &child_id) < 0) {
    return -1;
  }

  *result_ptr = child_id;
  return 0;
}

bool FileSystem::existsChild(internal::Inode& parent_inode, const std::string& name) {
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode.file_size; ++i) {
    Link link;
    inodes_.read(&parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (strcmp(link.name, name.c_str()) == 0) {
      return true;
    }
  }
  return false;
}

int FileSystem::getChildId(internal::Inode& parent_inode, const std::string& name, uint64_t* result_ptr) {
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode.file_size; ++i) {
    Link link;
    inodes_.read(&parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (strcmp(link.name, name.c_str()) == 0) {
      *result_ptr = link.inode_id;
      return 0;
    }
  }

  return -1;
}

int FileSystem::createChild(internal::Inode& parent_inode, const std::string& name, bool is_dir) {
  assert(name.size() <= MAX_LINK_NAME_LEN);

  if (existsChild(parent_inode, name)) {
    return -1;
  }

  if (inodes_.addDirectoryEntry(&parent_inode, name.c_str()) < 0) {
    return -1;
  }

  uint64_t child_id;
  int result = getChildId(parent_inode, name, &child_id);

  assert(result >= 0);

  inodes_.getInodeById(child_id).is_dir = is_dir;

  return 0;
}

}  // namespace fspp
