#include "fs++/filesystem.h"

#include <cstring>

// filesystem file layout
// | superblock | inode_bitset | block_bitset | inodes | blocks |

namespace fspp {

FileSystem::FileSystem(const std::string& ffile_path) : fsm_(ffile_path) {
}

bool FileSystem::existsDir(const std::string& dir_path) {
  uint64_t inode_id;

  if (fsm_.getFDEInodeId(dir_path, &inode_id)) {
    return -1;
  }

  return fsm_.getInodeById(inode_id).is_dir;
}

int FileSystem::createDir(const std::string& dir_path) {
  return fsm_.createFDE(dir_path, /*is_dir=*/true);
}

// file leak todo:fix
int FileSystem::deleteDir(const std::string& dir_path) {
  return fsm_.deleteFDE(dir_path, /*is_dir=*/true);
}

bool FileSystem::existsFile(const std::string& file_path) {
  uint64_t inode_id;

  if (fsm_.getFDEInodeId(file_path, &inode_id)) {
    return -1;
  }

  return !fsm_.getInodeById(inode_id).is_dir;
}

int FileSystem::createFile(const std::string& file_path) {
  return fsm_.createFDE(file_path, /*is_dir=*/false);
}

int FileSystem::deleteFile(const std::string& file_path) {
  return fsm_.deleteFDE(file_path, /*is_dir=*/false);
}

int FileSystem::readFileContent(const std::string& file_path, uint64_t offset, void* buffer, uint64_t size) {
  uint64_t inode_id;
  if (fsm_.getFDEInodeId(file_path, &inode_id) < 0) {
    return -1;
  }

  return fsm_.read(&fsm_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystem::writeFileContent(const std::string& file_path, uint64_t offset, const void* buffer, uint64_t size) {
  uint64_t inode_id;
  if (fsm_.getFDEInodeId(file_path, &inode_id) < 0) {
    return -1;
  }

  return fsm_.write(&fsm_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystem::listDir(const std::string& dir_path, std::string& output) {
  if (!existsDir(dir_path)) {
    return -1;
  }

  uint64_t inode_id;
  if (fsm_.getFDEInodeId(dir_path, &inode_id) < 0) {
    return -1;
  }
  internal::Inode& inode = fsm_.getInodeById(inode_id);

  output.clear();

  for (uint64_t i = 0; i * sizeof(internal::Link) < inode.file_size; ++i) {
    internal::Link link;
    fsm_.read(&inode, &link, i * sizeof(internal::Link), sizeof(internal::Link));
    if (link.is_alive) {
      output += std::string(link.name) + ((fsm_.getInodeById(link.inode_id).is_dir) ? "/ " : " ");
    }
  }

  return 0;
}

}  // namespace fspp
