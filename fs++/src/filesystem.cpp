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

// todo: remove abort
FileSystem::FileSystem(const std::string& filesystem_file_path) {
  int fd = open(filesystem_file_path.c_str(), O_RDWR);
  if (fd == -1) {
    if (errno != ENOENT) {
      perror("Can't open file that contains filesystem");
      std::abort();
    }

    FSPP_LOG("No filesystem file found.\nTrying to create default filesystem file.");

    fd = open(filesystem_file_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fd == -1) {
      FSPP_HANDLE_ERROR("Can't open file that contains filesystem");
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
  }

  auto* file_start =
      static_cast<in::SuperBlock*>(mmap64(nullptr, sizeof(in::SuperBlock), PROT_READ, MAP_SHARED, fd, 0));
  if (file_start == MAP_FAILED) {
    FSPP_HANDLE_ERROR("Can't mmap metadata for read");
  }

  super_block_ = *file_start;
  if (munmap(file_start, sizeof(in::SuperBlock)) == -1) {
    FSPP_HANDLE_ERROR("munmap metadata failed");
  }

  file_bytes_ =
      static_cast<uint8_t*>(mmap64(nullptr, super_block_.FileSystemSize(), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0));

  if (file_bytes_ == MAP_FAILED) {
    FSPP_HANDLE_ERROR("Can't mmap filesystem file");
  }

  blocks_ = in::BlockSpace(&super_block_.block_num, &super_block_.free_block_num,
                           file_bytes_ + super_block_.BlockBitSetOffset(),
                           reinterpret_cast<internal::Block*>(file_bytes_ + super_block_.BlocksOffset()));
  inodes_ = in::InodeSpace(&super_block_.inode_num, &super_block_.free_inode_num,
                           file_bytes_ + super_block_.InodeBitSetOffset(), &blocks_,
                           reinterpret_cast<internal::Inode*>(file_bytes_ + super_block_.InodesOffset()));
}

FileSystem::~FileSystem() {
  if (munmap(file_bytes_, super_block_.FileSystemSize()) == -1) {
    FSPP_HANDLE_ERROR("Can't unmap filesystem file");
  }
}

bool FileSystem::existsDir(const std::string& dir_path) {
  return false;
}

int FileSystem::createDir(const std::string& dir_path) {
  return -1;
}

int FileSystem::createDir(const std::string& parent_path, const std::string& dir_name) {
  //  uint64_t inode_id = getInodeIdByPath(parent_path);
  //  auto& inode = inodes_.getInodeById(inode_id);
  return -1;
}

// file leak
int FileSystem::deleteDir(const std::string& dir_path) {
  if (!existsDir(dir_path)) {
    return -1;
  }

  uint64_t inode_id = getInodeIdByPath(dir_path);
  inodes_.deleteInode(inode_id);

  return 0;
}

bool FileSystem::existsFile(const std::string& file_path) {
  return false;
}

int FileSystem::createFile(const std::string& file_path) {
  return -1;
}

int FileSystem::createFile(const std::string& parent_path, const std::string& file_name) {
  return -1;
}

int FileSystem::deleteFile(const std::string& file_path) {
  if (!existsFile(file_path)) {
    return -1;
  }

  uint64_t inode_id = getInodeIdByPath(file_path);
  inodes_.deleteInode(inode_id);

  return 0;
}

int FileSystem::readFileContent(const std::string& file_path, uint64_t offset, void* buffer, uint64_t size) {
  uint64_t inode_id = getInodeIdByPath(file_path);
  return inodes_.read(&inodes_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystem::writeFileContent(const std::string& file_path, uint64_t offset, const void* buffer, uint64_t size) {
  uint64_t inode_id = getInodeIdByPath(file_path);
  return inodes_.write(&inodes_.getInodeById(inode_id), buffer, offset, size);
}

uint64_t FileSystem::getInodeIdByPath(std::string path) {
  std::regex path_regex("^(/([^/]+))(.*)$");
  std::smatch match;

  uint64_t inode_id = 0;
  while (std::regex_match(path, match, path_regex)) {
    std::string name = match[2];
    assert(existsChild(inodes_.getInodeById(inode_id), name));
    inode_id = getChildId(inodes_.getInodeById(inode_id), name);

    path = match[3];
  }

  return inode_id;
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

uint64_t FileSystem::getChildId(internal::Inode& parent_inode, const std::string& name) {
  for (uint64_t i = 0; i * sizeof(Link) < parent_inode.file_size; ++i) {
    Link link;
    inodes_.read(&parent_inode, &link, i * sizeof(Link), sizeof(Link));
    if (strcmp(link.name, name.c_str()) == 0) {
      return link.inode_id;
    }
  }

  std::abort();
}

}  // namespace fspp