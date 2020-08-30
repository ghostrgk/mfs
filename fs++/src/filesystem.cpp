#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

#include "fs++/internal/filesystem.h"
#include "fs++/internal/logging.h"
#include "fs++/internal/compiler.h"

#define FSPP_HANDLE_ERROR(msg) \
  do {                         \
    perror(msg);               \
    std::abort();              \
  } while (false)

namespace fspp::internal {

namespace in = internal;
using internal::Link;

static int initRootInode(int ffile_fd, SuperBlock& superblock) {
  auto* ffile_content = static_cast<uint8_t*>(
      mmap64(nullptr, superblock.FileSystemSize(), PROT_WRITE | PROT_READ, MAP_SHARED, ffile_fd, 0));

  if (ffile_content == MAP_FAILED) {
    perror("Can't mmap ffile for root inode initializing");
    return -1;
  }

  auto* superblock_ptr = reinterpret_cast<SuperBlock*>(ffile_content);
  --(superblock_ptr->free_inode_num);

  auto* root_inode_ptr = reinterpret_cast<Inode*>(ffile_content + superblock.InodesOffset());
  root_inode_ptr->is_dir = true;
  root_inode_ptr->inodes_list.setSize(0);
  root_inode_ptr->blocks_count = 0;
  root_inode_ptr->file_size = 0;

  BitSet inode_bitset(superblock.inode_num, ffile_content + superblock.InodeBitSetOffset());
  inode_bitset.setBit(0);

  if (munmap(ffile_content, superblock.FileSystemSize()) == -1) {
    perror("munmap ffile failed");
    return -1;
  }

  return 0;
}

// todo: remove abort
FileSystem::FileSystem(const std::string& ffile_path) {
  fd_ = open(ffile_path.c_str(), O_RDWR);
  if (fd_ == -1) {
    if (errno != ENOENT) {
      FSPP_HANDLE_ERROR("Can't open ffile");
    }

    FSPP_LOG("FSM", "No ffile found. Creating default ffile.");

    fd_ = open(ffile_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fd_ == -1) {
      FSPP_HANDLE_ERROR("Can't open ffile");
    }

    SuperBlock super_block = {.block_num = DEFAULT_BLOCK_COUNT,
                              .free_block_num = super_block.block_num,
                              .inode_num = DEFAULT_INODE_COUNT,
                              .free_inode_num = super_block.inode_num};

    if (ftruncate(fd_, super_block.FileSystemSize()) == -1) {
      FSPP_HANDLE_ERROR("Truncation failed");
    }
    auto* file_start =
        static_cast<SuperBlock*>(mmap64(nullptr, sizeof(SuperBlock), PROT_WRITE | PROT_READ, MAP_SHARED, fd_, 0));
    if (file_start == MAP_FAILED) {
      FSPP_HANDLE_ERROR("Can't mmap truncated file");
    }

    *file_start = super_block;
    if (munmap(file_start, sizeof(SuperBlock)) == -1) {
      FSPP_HANDLE_ERROR("munmap truncated file failed");
    }

    if (initRootInode(fd_, super_block) < 0) {
      std::abort();
    }
  }

  auto* file_start = static_cast<SuperBlock*>(mmap64(nullptr, sizeof(SuperBlock), PROT_READ, MAP_SHARED, fd_, 0));
  if (file_start == MAP_FAILED) {
    FSPP_HANDLE_ERROR("Can't mmap metadata for read");
  }

  SuperBlock super_block = *file_start;
  if (munmap(file_start, sizeof(SuperBlock)) == -1) {
    FSPP_HANDLE_ERROR("munmap metadata failed");
  }

  FSPP_LOG("FSM", "Initializing fsm object.");
  file_bytes_ =
      static_cast<uint8_t*>(mmap64(nullptr, super_block.FileSystemSize(), PROT_WRITE | PROT_READ, MAP_SHARED, fd_, 0));

  if (file_bytes_ == MAP_FAILED) {
    FSPP_HANDLE_ERROR("Can't mmap ffile");
  }

  super_block_ptr_ = reinterpret_cast<internal::SuperBlock*>(file_bytes_);

  BitSet blocks_bitset(super_block_ptr_->block_num, file_bytes_ + super_block_ptr_->BlockBitSetOffset());
  blocks_ = Blocks(file_bytes_ + super_block_ptr_->BlocksOffset(), &super_block_ptr_->free_block_num,
                   std::move(blocks_bitset));

  BitSet inodes_bitset(super_block_ptr_->inode_num, file_bytes_ + super_block_ptr_->InodeBitSetOffset());
  inodes_ = Inodes(file_bytes_ + super_block_ptr_->InodesOffset(), &blocks_, &super_block_ptr_->free_inode_num,
                   std::move(inodes_bitset));

  //  blocks_ = Blocks(&super_block_ptr_->block_num, &super_block_ptr_->free_block_num,
  //                           file_bytes_ + super_block_ptr_->BlockBitSetOffset(),
  //                           reinterpret_cast<internal::Block*>(file_bytes_ + super_block_ptr_->BlocksOffset()));

  //  inodes_ = Inodes(&super_block_ptr_->inode_num, &super_block_ptr_->free_inode_num,
  //                   file_bytes_ + super_block_ptr_->InodeBitSetOffset(), &blocks_,
  //                   reinterpret_cast<Inode*>(file_bytes_ + super_block_ptr_->InodesOffset()));

  FSPP_LOG("FSM", "inode bitset: " + std::to_string(super_block_ptr_->InodeBitSetOffset()));
  FSPP_LOG("FSM", "block bitset: " + std::to_string(super_block_ptr_->BlockBitSetOffset()));
  FSPP_LOG("FSM", "inodes: " + std::to_string(super_block_ptr_->InodesOffset()));
  FSPP_LOG("FSM", "blocks: " + std::to_string(super_block_ptr_->BlocksOffset()));
}

FileSystem::~FileSystem() {
  FSPP_LOG("FSM", "Destroying fsm object.");
  if (munmap(file_bytes_, super_block_ptr_->FileSystemSize()) == -1) {
    FSPP_HANDLE_ERROR("Can't unmap ffile");
  }

  close(fd_);
}

int FileSystem::deleteInode(uint64_t inode_id) {
  inodes_.deleteInode(inode_id);
  return 0;
}

int FileSystem::getFDEInodeId(std::string fde_path, uint64_t* result_ptr) {
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

    memcpy(name, next_name_ptr, next_name_len);
    name[next_name_len] = 0;

    uint64_t child_id = 0;
    if (getChildId(inodes_.getInodeById(current_inode_id), std::string(name), &child_id) < 0) {
      delete[] name;
      return -1;
    }

    delete[] name;

    if (!getInodeById(child_id).is_dir) {
      return -1;
    }

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
    if (strcmp(link.name, name.c_str()) == 0 && link.is_alive) {
      return true;
    }
  }
  return false;
}

int FileSystem::getChildId(internal::Inode& parent_inode, const std::string& name, uint64_t* result_ptr) {
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode.file_size; ++i) {
    Link link;
    inodes_.read(&parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (strcmp(link.name, name.c_str()) == 0 && link.is_alive) {
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

  if (inodes_.addDirectoryEntry(&parent_inode, name.c_str(), is_dir) < 0) {
    return -1;
  }

#ifdef REDUNDANT_CHECKS
  uint64_t child_id;
  assert(getChildId(parent_inode, name, &child_id) >= 0);
#endif

  return 0;
}

int FileSystem::read(Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) const {
  return inodes_.read(inode_ptr, buffer, offset, count);
}

int FileSystem::write(Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count) {
  return inodes_.write(inode_ptr, buffer, offset, count);
}

[[maybe_unused]] int FileSystem::append(Inode* inode_ptr, const void* buffer, uint64_t count) {
  return inodes_.append(inode_ptr, buffer, count);
}

Inode& FileSystem::getInodeById(uint64_t inode_id) {
  return inodes_.getInodeById(inode_id);
}

int FileSystem::createFDE(const std::string& fde_path, bool is_dir) {
  if (existsFDE(fde_path)) {
    return -1;
  }

  assert(fde_path[0] == '/');
  assert(fde_path[fde_path.size() - 1] != '/');

  const char* next_name_ptr = fde_path.c_str() + 1;
  const char* next_name_end = strchr(next_name_ptr, '/');

  uint64_t current_inode_id = 0;
  while (next_name_end != nullptr) {
    uint64_t next_name_len = next_name_end - next_name_ptr;

    char* name = new char[next_name_len + 1];

    memcpy(name, next_name_ptr, next_name_len);
    name[next_name_len] = 0;

    uint64_t child_id = 0;
    Inode& current_inode = inodes_.getInodeById(current_inode_id);
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

    if (!getInodeById(child_id).is_dir) {
      return -1;
    }

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

bool FileSystem::existsFDE(const std::string& fde_path) {
  uint64_t inode_id;
  return getFDEInodeId(fde_path, &inode_id) >= 0;
}

int FileSystem::deleteFDE(const std::string& fde_path, bool is_dir) {
  if (fde_path == "/") {
    return -1;
  }

  uint64_t inode_id;
  if (getFDEInodeId(fde_path, &inode_id) < 0 || getInodeById(inode_id).is_dir != is_dir) {
    return -1;
  }

  if (deleteInode(inode_id) < 0) {
    return -1;
  }

  uint64_t parent_inode_id;
  assert(getFDEInodeParentId(fde_path, &parent_inode_id) >= 0);

  Inode& parent_inode = getInodeById(parent_inode_id);
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode.file_size; ++i) {
    Link link;
    read(&parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (link.inode_id == inode_id) {
      assert(link.is_alive);
      link.is_alive = false;
      if (write(&parent_inode, &link, i * sizeof(Link), sizeof(Link)) < 0) {
        return -1;
      }

      break;
    }
  }

  return 0;
}

int FileSystem::getFDEInodeParentId(std::string fde_path, uint64_t* result_ptr) {
  if (fde_path == "/") {
    return -1;
  }

  assert(fde_path[0] == '/');
  assert(fde_path[fde_path.size() - 1] != '/');

  const char* next_name_ptr = fde_path.c_str() + 1;
  const char* next_name_end = strchr(next_name_ptr, '/');

  uint64_t current_inode_id = 0;
  while (next_name_end != nullptr) {
    uint64_t next_name_len = next_name_end - next_name_ptr;

    char* name = new char[next_name_len + 1];

    memcpy(name, next_name_ptr, next_name_len);
    name[next_name_len] = 0;

    uint64_t child_id = 0;
    if (getChildId(inodes_.getInodeById(current_inode_id), std::string(name), &child_id) < 0) {
      delete[] name;
      return -1;
    }

    delete[] name;

    if (!getInodeById(child_id).is_dir) {
      return -1;
    }

    current_inode_id = child_id;
    next_name_ptr = next_name_end + 1;
    next_name_end = strchr(next_name_ptr, '/');
  }

  *result_ptr = current_inode_id;
  return 0;
}

int FileSystem::listDir(const std::string& dir_path, std::string& output) {
  uint64_t inode_id;
  if (getFDEInodeId(dir_path, &inode_id) < 0 || !getInodeById(inode_id).is_dir) {
    return -1;
  }

  internal::Inode& inode = getInodeById(inode_id);

  output.clear();

  for (uint64_t i = 0; i * sizeof(internal::Link) < inode.file_size; ++i) {
    internal::Link link;
    read(&inode, &link, i * sizeof(internal::Link), sizeof(internal::Link));
    if (link.is_alive) {
      output += std::string(link.name) + ((getInodeById(link.inode_id).is_dir) ? "/ " : " ");
    }
  }

  return 0;
}

}  // namespace fspp::internal