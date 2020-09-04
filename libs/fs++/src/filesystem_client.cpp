#include "fs++/filesystem_client.h"

// ffile layout
// | superblock | inode_bitset | block_bitset | inodes | blocks |

namespace fspp {

FileSystemClient::FileSystemClient(const std::string& ffile_path) : fs_(ffile_path) {
}

bool FileSystemClient::existsDir(const std::string& dir_path) {
  uint64_t inode_id;

  if (fs_.getFDEInodeId(dir_path, &inode_id) < 0) {
    return false;
  }

  return fs_.getInodeById(inode_id).is_dir;
}

int FileSystemClient::createDir(const std::string& dir_path) {
  return fs_.createFDE(dir_path, /*is_dir=*/true);
}

int FileSystemClient::deleteDir(const std::string& dir_path) {
  return fs_.deleteFDE(dir_path, /*is_dir=*/true);
}

bool FileSystemClient::existsFile(const std::string& file_path) {
  uint64_t inode_id;

  if (fs_.getFDEInodeId(file_path, &inode_id) < 0) {
    return false;
  }

  return !fs_.getInodeById(inode_id).is_dir;
}

int FileSystemClient::createFile(const std::string& file_path) {
  return fs_.createFDE(file_path, /*is_dir=*/false);
}

int FileSystemClient::deleteFile(const std::string& file_path) {
  return fs_.deleteFDE(file_path, /*is_dir=*/false);
}

int FileSystemClient::readFileContent(const std::string& file_path, uint64_t offset, void* buffer, uint64_t size) {
  uint64_t inode_id;
  if (fs_.getFDEInodeId(file_path, &inode_id) < 0) {
    return -1;
  }

  return fs_.read(&fs_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystemClient::writeFileContent(const std::string& file_path, uint64_t offset, const void* buffer,
                                       uint64_t size) {
  uint64_t inode_id;
  if (fs_.getFDEInodeId(file_path, &inode_id) < 0) {
    return -1;
  }

  return fs_.write(&fs_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystemClient::listDir(const std::string& dir_path, std::string& output) {
  return fs_.listDir(dir_path, output);
}

uint64_t FileSystemClient::fileSize(const std::string& file_path) {
  uint64_t inode_id;
  int rc = fs_.getFDEInodeId(file_path, &inode_id);

  assert(rc >= 0);
  FSPP_USED_BY_ASSERT(rc);

  return fs_.getInodeById(inode_id).file_size;
}

}  // namespace fspp
