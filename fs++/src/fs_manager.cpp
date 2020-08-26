#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

#include "fs++/internal/fs_manager.h"
#include "fs++/internal/logging.h"
#include "fs++/internal/config.h"

#define FSPP_HANDLE_ERROR(msg) \
  do {                         \
    perror(msg);               \
    std::abort();              \
  } while (false)

namespace fspp::internal {

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
FSManager::FSManager(const std::string& ffile_path) {
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

FSManager::~FSManager() {
  if (munmap(file_bytes_, super_block_ptr_->FileSystemSize()) == -1) {
    FSPP_HANDLE_ERROR("Can't unmap ffile");
  }
}

int FSManager::deleteInode(uint64_t inode_id) {
  inodes_.deleteInode(inode_id);
  return 0;
}

int FSManager::getFDEInodeId(std::string fde_path, uint64_t* result_ptr) {
  if (fde_path == "/") {
    *result_ptr = 0;
    return 0;
  }

  assert(fde_path[0] == '/');
  assert(fde_path[fde_path.size() - 1] != '/');

  const char* next_name_ptr = fde_path.c_str() + 1;
  const char* next_name_end = strchr(next_name_ptr, '/');

  uint64_t current_inode_id = 0;
  while (next_name_end != nullptr) {
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

bool FSManager::existsChild(internal::Inode& parent_inode, const std::string& name) {
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode.file_size; ++i) {
    Link link;
    inodes_.read(&parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (strcmp(link.name, name.c_str()) == 0) {
      return true;
    }
  }
  return false;
}

int FSManager::getChildId(internal::Inode& parent_inode, const std::string& name, uint64_t* result_ptr) {
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

int FSManager::createChild(internal::Inode& parent_inode, const std::string& name, bool is_dir) {
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

int FSManager::read(Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) const {
  return inodes_.read(inode_ptr, buffer, offset, count);
}

int FSManager::write(Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count) {
  return inodes_.write(inode_ptr, buffer, offset, count);
}

int FSManager::append(Inode* inode_ptr, const void* buffer, uint64_t count) {
  return inodes_.append(inode_ptr, buffer, count);
}

Inode& FSManager::getInodeById(uint64_t inode_id) {
  return inodes_.getInodeById(inode_id);
}

int FSManager::createFDE(const std::string& fde_path, bool is_dir) {
  if (fde_path == "/") {
    return 0;
  }

  assert(fde_path[0] == '/');
  assert(fde_path[fde_path.size() - 1] != '/');

  const char* next_name_ptr = fde_path.c_str() + 1;
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

  Inode& current_inode = getInodeById(current_inode_id);
  if (!existsChild(current_inode, next_name_ptr)) {
    if (createChild(current_inode, next_name_ptr, is_dir) < 0) {
      return -1;
    }
  }

  return 0;
}

bool FSManager::existsFDE(const std::string& fde_path) {
  uint64_t inode_id;
  return getFDEInodeId(fde_path, &inode_id) >= 0;
}

}  // namespace fspp::internal