#include "cmds.h"

#include <iostream>
#include <regex>

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int mkfile(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
  static const std::regex full_regex(R"(^\s*mkfile\s+(/|((/[\w.]+)+))\s*$)");
  std::cerr << "mkfile command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    user_output << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (fs.existsDir(path)) {
    user_output << "Already exists directory with the same name" << std::endl;
    return -1;
  }

  if (fs.existsFile(path)) {
    user_output << "File already exists" << std::endl;
    return -1;
  }

  if (fs.createFile(path) < 0) {
    user_output << "Can't create file" << std::endl;
    return -1;
  }

  user_output << "Ok" << std::endl;

  return 0;
}

int rmfile(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
  static const std::regex full_regex(R"(^\s*rmfile\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "rmfile command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    user_output << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (!fs.existsFile(path)) {
    user_output << "File doesn't exist" << std::endl;
    return -1;
  }

  if (fs.deleteFile(path) < 0) {
    user_output << "Can't delete file" << std::endl;
    return -1;
  }

  user_output << "Ok" << std::endl;
  return 0;
}

int mkdir(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
  static const std::regex full_regex(R"(^\s*mkdir\s+(/|((/[\w.]+)+))\s*$)");
  std::cerr << "mkdir command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    user_output << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (fs.existsDir(path)) {
    user_output << "Directory already exists" << std::endl;
    return -1;
  }

  if (fs.existsFile(path)) {
    user_output << "Already exists file with the same name" << std::endl;
    return -1;
  }

  if (fs.createDir(path) < 0) {
    user_output << "Can't create directory" << std::endl;
    return -1;
  }

  user_output << "Ok" << std::endl;
  return 0;
}

int rmdir(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
  static const std::regex full_regex(R"(^\s*rmdir\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "rmdir command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    user_output << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (!fs.existsDir(path)) {
    user_output << "Directory doesn't exist" << std::endl;
    return -1;
  }

  if (path == "/") {
    user_output << "You can't remove root directory" << std::endl;
    return -1;
  }

  if (fs.deleteDir(path) < 0) {
    user_output << "Can't delete directory" << std::endl;
    return -1;
  }

  user_output << "Ok" << std::endl;
  return 0;
}

int lsdir(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
  static const std::regex full_regex(R"(^\s*lsdir\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "lsdir command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    user_output << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (!fs.existsDir(path)) {
    user_output << "Directory doesn't exist" << std::endl;
    return -1;
  }

  std::string output;
  if (fs.listDir(path, output) < 0) {
    user_output << "Can't list directory" << std::endl;
    return -1;
  }

  user_output << path << ": " << output << std::endl;

  return 0;
}

int store(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
  static const std::regex full_regex(R"(^\s*store\s+(/|(/[-\d\w.]+)+)\s+(/|(/[-\d\w.]+)+)\s*$)");
  std::cerr << "store command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    user_output << "Wrong from_path or to_path format" << std::endl;
    return -1;
  }

  const std::string& from_path = match[1];
  std::string to_path = match[3];
  std::cerr << "(from_path=" << from_path << ") ";
  std::cerr << "(to_path=" << to_path << ") ";

  const char* from_basename_start = strrchr(from_path.c_str(), '/') + 1;
  assert(from_basename_start != nullptr);
  std::string from_basename(from_basename_start);

  if (fs.existsDir(to_path)) {
    to_path += from_basename;
  }

  int from_fd = open(from_path.c_str(), O_RDONLY);
  if (from_fd < 0) {
    user_output << "Can't open from_file" << std::endl;
    return -1;
  }

  struct stat stat_buf {};
  if (fstat(from_fd, &stat_buf) < 0) {
    user_output << "Can't read from_file length" << std::endl;

    close(from_fd);
    return -1;
  }

  if (S_ISDIR(stat_buf.st_mode)) {
    user_output << "You can't store directory. Only storing files is supporting" << std::endl;

    close(from_fd);
    return -1;
  }

  uint64_t from_file_len = stat_buf.st_size;
  void* from_file_content = mmap64(nullptr, from_file_len, PROT_READ, MAP_PRIVATE, from_fd, 0);

  if (!fs.existsFile(to_path)) {
    if (fs.createFile(to_path) < 0) {
      user_output << "Can't create file in app filesystem" << std::endl;

      munmap(from_file_content, from_file_len);
      close(from_fd);
      return -1;
    }
  }

  int bytes_written = fs.writeFileContent(to_path, 0, from_file_content, from_file_len);
  if (bytes_written == -1) {
    user_output << "Can't write to app filesystem" << std::endl;

    munmap(from_file_content, from_file_len);
    close(from_fd);
    return -1;
  }

  if ((uint64_t)bytes_written != from_file_len) {
    std::cerr << "possible file corruption" << std::endl;
  }

  munmap(from_file_content, from_file_len);
  close(from_fd);

  user_output << "Ok" << std::endl;
  return 0;
}

int load(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
  static const std::regex full_regex(R"(^\s*load\s+(/|(/[\w.]+)+)\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "load command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    user_output << "Wrong from_path or to_path format" << std::endl;
    return -1;
  }

  const std::string& from_path = match[1];
  std::string to_path = match[3];
  std::cerr << "(from_path=" << from_path << ") ";
  std::cerr << "(to_path=" << to_path << ") ";

  if (!fs.existsFile(from_path)) {
    user_output << "Requested file doesn't exist" << std::endl;
    return -1;
  }

  const char* from_basename_start = strrchr(from_path.c_str(), '/') + 1;
  assert(from_basename_start != nullptr);
  std::string from_basename(from_basename_start);

  int to_fd = open(to_path.c_str(), O_RDWR);
  if (to_fd < 0) {
    if (errno != ENOENT) {
      user_output << "Can't open to_file/to_directory" << std::endl;
      return -1;
    }

    to_fd = open(to_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0640);
    if (to_fd < 0) {
      user_output << "Can't create to_file" << std::endl;
      return -1;
    }
  }

  struct stat stat_buf {};
  if (fstat(to_fd, &stat_buf) < 0) {
    user_output << "Can't read to_file stat" << std::endl;

    close(to_fd);
    return -1;
  }

  if (S_ISDIR(stat_buf.st_mode)) {
    to_path += from_basename;
    close(to_fd);
    to_fd = open(to_path.c_str(), O_RDWR);
    if (to_fd < 0) {
      if (errno != ENOENT) {
        user_output << "Can't open to_file" << std::endl;
        return -1;
      }

      to_fd = open(to_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0640);
      if (to_fd < 0) {
        user_output << "Can't create to_file" << std::endl;
        return -1;
      }
    }
  }

  uint64_t from_file_len = fs.fileSize(from_path);
  ftruncate(to_fd, from_file_len);
  void* to_file_content = mmap64(nullptr, from_file_len, PROT_WRITE, MAP_SHARED, to_fd, 0);

  int bytes_read = fs.readFileContent(from_path, 0, to_file_content, from_file_len);
  if (bytes_read == -1) {
    user_output << "Can't write to app filesystem" << std::endl;

    munmap(to_file_content, from_file_len);
    close(to_fd);
    return -1;
  }

  if ((uint64_t)bytes_read != from_file_len) {
    std::cerr << "possible file corruption" << std::endl;
  }

  munmap(to_file_content, from_file_len);
  close(to_fd);

  user_output << "Ok" << std::endl;
  return 0;
}