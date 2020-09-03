#include "cmds.h"

#include <iostream>
#include <regex>

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

int store(int socket_fd, fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
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

  if (!fs.existsFile(to_path)) {
    if (fs.createFile(to_path) < 0) {
      user_output << "Can't create file in app filesystem" << std::endl;
      return -1;
    }
  }

  char intermediate_ok[] = "IOK";
  write(socket_fd, intermediate_ok, sizeof(intermediate_ok));

  // host/network problems may happened
  // there is no default hton64
  // todo: fix
  uint64_t file_len = 0;
  int bytes_read;

  if ((bytes_read = read(socket_fd, &file_len, sizeof(file_len))) < 0 || bytes_read != sizeof(file_len)) {
    user_output << "Can't receive file len" << std::endl;
    return -1;
  }

  for (uint64_t bytes_written = 0; bytes_written < file_len;) {
    char buffer[4096];
    if ((bytes_read = read(socket_fd, buffer, sizeof(buffer))) < 0) {
      // maybe add more smart way to have unfilled files
      fs.deleteFile(to_path);
      user_output << "Can't receive file content" << std::endl;
      return -1;
    }

    uint64_t current_write_len = std::min((uint64_t)bytes_read, file_len - bytes_written);
    if (fs.writeFileContent(to_path, bytes_written, buffer, current_write_len) < 0) {
      user_output << "Writing to file failed" << std::endl;
      return -1;
    }
    bytes_written += current_write_len;
  }

  user_output << "Ok" << std::endl;
  return 0;
}

int load(int socket_fd, fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output) {
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

  char intermediate_ok[] = "IOK";
  write(socket_fd, intermediate_ok, sizeof(intermediate_ok));

  // todo: make hton64?
  uint64_t file_len = fs.fileSize(from_path);
  std::cerr << "(file_len=" << file_len << ") ";

  int bytes_written;
  if ((bytes_written = write(socket_fd, &file_len, sizeof(file_len))) < 0 || bytes_written != sizeof(file_len)) {
    user_output << "Can't send file len" << std::endl;
    return -1;
  }

  for (uint64_t bytes_sent = 0; bytes_sent < file_len;) {
    char buffer[4096];
    uint64_t current_read_len = std::min((uint64_t)sizeof(buffer), file_len - bytes_sent);
    std::cerr << "(current_read_len=" << current_read_len << ") ";
    if (fs.readFileContent(from_path, bytes_sent, buffer, current_read_len) < 0) {
      user_output << "Reading of file failed" << std::endl;
      return -1;
    }

    if (write(socket_fd, buffer, current_read_len) < 0) {
      user_output << "Can't send file content" << std::endl;
    }
    bytes_sent += current_read_len;
  }

  user_output << "Ok" << std::endl;
  return 0;
}