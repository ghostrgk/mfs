#pragma once

#include <string>

#include "internal/superblock.h"
#include "internal/inode.h"
#include "internal/block.h"

namespace fspp {

class FileSystem {
 public:
  explicit FileSystem(const std::string& ffile_path);
  ~FileSystem();
  bool existsDir(const std::string& dir_path);
  int createDir(const std::string& dir_path);
  int createDir(const std::string& parent_path, const std::string& dir_name);
  int deleteDir(const std::string& dir_path);
  int listDir(const std::string& dir_path);

  bool existsFile(const std::string& file_path);
  int createFile(const std::string& file_path);
  int deleteFile(const std::string& file_path);

  int readFileContent(const std::string& file_path, uint64_t offset, void* buffer, uint64_t size);
  int writeFileContent(const std::string& file_path, uint64_t offset, const void* buffer, uint64_t size);

 private:
  int getChildId(internal::Inode& parent_inode, const std::string& name, uint64_t* result_ptr);
  bool existsChild(internal::Inode& parent_inode, const std::string& name);
  int createChild(internal::Inode& parent_inode, const std::string& name, bool is_dir);
  int getInodeIdByPath(std::string path, uint64_t* result_ptr);

 private:
  uint8_t* file_bytes_{nullptr};
  internal::SuperBlock* super_block_ptr_;
  internal::InodeSpace inodes_;
  internal::BlockSpace blocks_;
};

}  // namespace fspp
