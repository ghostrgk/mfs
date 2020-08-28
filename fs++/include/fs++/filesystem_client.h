#pragma once

#include <string>

#include "internal/filesystem.h"

namespace fspp {

class FileSystemClient {
 public:
  explicit FileSystemClient(const std::string& ffile_path);

  bool existsDir(const std::string& dir_path);
  int createDir(const std::string& dir_path);
  int deleteDir(const std::string& dir_path);
  int listDir(const std::string& dir_path, std::string& output);

  bool existsFile(const std::string& file_path);
  int createFile(const std::string& file_path);
  int deleteFile(const std::string& file_path);
  uint64_t fileSize(const std::string& file_path);

  int readFileContent(const std::string& file_path, uint64_t offset, void* buffer, uint64_t size);
  int writeFileContent(const std::string& file_path, uint64_t offset, const void* buffer, uint64_t size);

 private:
  internal::FileSystem fs_;
};

}  // namespace fspp
