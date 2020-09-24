#include "fsc/filesystem_client.h"

// ffile layout
// | superblock | inode_bitset | block_bitset | inodes | blocks |

int FileSystemClient_init(struct FileSystemClient* fsc, const char* ffile_path) {
  FileSystem_init(&fsc->fs_, ffile_path);
  return 0;
}

bool existsDir(struct FileSystemClient* fsc, const char* dir_path) {
  uint64_t inode_id;

  if (getFDEInodeId(&fsc->fs_, dir_path, &inode_id) < 0) {
    return false;
  }

  return FS_getInodeById(&fsc->fs_, inode_id)->is_dir;
}

int createDir(struct FileSystemClient* fsc, const char* dir_path) {
  return createFDE(&fsc->fs_, dir_path, /*is_dir=*/true);
}

int deleteDir(struct FileSystemClient* fsc, const char* dir_path) {
  return deleteFDE(&fsc->fs_, dir_path, /*is_dir=*/true);
}

/*
 * You must free *_output_
 */
int listDir(struct FileSystemClient* fsc, const char* dir_path, char** output);

bool existsFile(struct FileSystemClient* fsc, const char* file_path);
int createFile(struct FileSystemClient* fsc, const char* file_path);
int deleteFile(struct FileSystemClient* fsc, const char* file_path);

// one must use only after successful existsFile
uint64_t fileSize(struct FileSystemClient* fsc, const char* file_path);

int readFileContent(struct FileSystemClient* fsc, const char* file_path, uint64_t offset, void* buffer, uint64_t size);
int writeFileContent(struct FileSystemClient* fsc, const char* file_path, uint64_t offset, const void* buffer,
                     uint64_t size);


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

  return fs_.Inode_read(&fs_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystemClient::writeFileContent(const std::string& file_path, uint64_t offset, const void* buffer,
                                       uint64_t size) {
  uint64_t inode_id;
  if (fs_.getFDEInodeId(file_path, &inode_id) < 0) {
    return -1;
  }

  return fs_.Inode_write(&fs_.getInodeById(inode_id), buffer, offset, size);
}

int FileSystemClient::listDir(const std::string& dir_path, std::string& output) {
  return fs_.FS_listDir(dir_path, output);
}

uint64_t FileSystemClient::fileSize(const std::string& file_path) {
  uint64_t inode_id;
  int rc = fs_.getFDEInodeId(file_path, &inode_id);

  assert(rc >= 0);
  FSC_USED_BY_ASSERT(rc);

  return fs_.getInodeById(inode_id).file_size;
}

}  // namespace fspp
