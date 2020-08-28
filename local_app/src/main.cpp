#include <iostream>
#include <string>
#include <regex>

#include <cassert>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fs++/filesystem_client.h>

bool check_path(const std::string& string) {
  static const std::regex path_regex("(/\\w+)+");
  std::smatch path_match;
  return std::regex_match(string, path_match, path_regex);
}

int mkfile(fspp::FileSystemClient& fs, const std::string& query) {
  static const std::regex full_regex(R"(^\s*mkfile\s+(/|((/[\w.]+)+))\s*$)");
  std::cerr << "mkfile command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (fs.existsDir(path)) {
    std::cout << "Already exists directory with the same name" << std::endl;
    return -1;
  }

  if (fs.existsFile(path)) {
    std::cout << "File already exists" << std::endl;
    return -1;
  }

  if (fs.createFile(path) < 0) {
    return -1;
  }

  return 0;
}

int rmfile(fspp::FileSystemClient& fs, const std::string& query) {
  static const std::regex full_regex(R"(^\s*rmfile\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "rmfile command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (!fs.existsFile(path)) {
    std::cout << "File doesn't exist" << std::endl;
    return -1;
  }

  if (fs.deleteFile(path) < 0) {
    return -1;
  }

  return 0;
}

int mkdir(fspp::FileSystemClient& fs, const std::string& query) {
  static const std::regex full_regex(R"(^\s*mkdir\s+(/|((/[\w.]+)+))\s*$)");
  std::cerr << "mkdir command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (fs.existsDir(path)) {
    std::cout << "Directory already exists" << std::endl;
    return -1;
  }

  if (fs.existsFile(path)) {
    std::cout << "Already exists file with the same name" << std::endl;
    return -1;
  }

  if (fs.createDir(path) < 0) {
    return -1;
  }

  return 0;
}

int rmdir(fspp::FileSystemClient& fs, const std::string& query) {
  static const std::regex full_regex(R"(^\s*rmdir\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "rmdir command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (!fs.existsDir(path)) {
    std::cout << "Directory doesn't exist" << std::endl;
    return -1;
  }

  if (path == "/") {
    std::cout << "You can't remove root directory" << std::endl;
    return -1;
  }

  if (fs.deleteDir(path) < 0) {
    return -1;
  }

  return 0;
}

int lsdir(fspp::FileSystemClient& fs, const std::string& query) {
  static const std::regex full_regex(R"(^\s*lsdir\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "lsdir command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong path format" << std::endl;
    return -1;
  }

  const std::string& path = match[1];
  std::cerr << "(path=" << path << ") ";

  if (!fs.existsDir(path)) {
    std::cout << "Directory doesn't exist" << std::endl;
    return -1;
  }

  std::string output;
  if (fs.listDir(path, output) < 0) {
    return -1;
  }

  std::cout << output << std::endl;

  return 0;
}

int store(fspp::FileSystemClient& fs, const std::string& query) {
  static const std::regex full_regex(R"(^\s*store\s+(/|(/[\w.]+)+)\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "store command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong from_path or to_path format" << std::endl;
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
    std::cout << "Can't open from_file" << std::endl;
    return -1;
  }

  struct stat stat_buf{};
  if (fstat(from_fd, &stat_buf) < 0) {
    std::cout << "Can't read from_file length" << std::endl;

    close(from_fd);
    return -1;
  }

  if (S_ISDIR(stat_buf.st_mode)) {
    std::cout << "You can't store directory. Only storing files is supporting" << std::endl;

    close(from_fd);
    return -1;
  }

  uint64_t from_file_len = stat_buf.st_size;
  void* from_file_content = mmap64(nullptr, from_file_len, PROT_READ, MAP_PRIVATE, from_fd, 0);

  if (!fs.existsFile(to_path)) {
    if (fs.createFile(to_path) < 0) {
      std::cout << "Can't create file in app filesystem" << std::endl;

      munmap(from_file_content, from_file_len);
      close(from_fd);
      return -1;
    }
  }

  int bytes_written = fs.writeFileContent(to_path, 0, from_file_content, from_file_len);
  if ((uint64_t)bytes_written != from_file_len) {
    std::cerr << "possible file corruption" << std::endl;
  }

  munmap(from_file_content, from_file_len);
  close(from_fd);

  return 0;
}

template <typename T>
void printSizes(T& os) {
  os << "Inode size: " << sizeof(fspp::internal::Inode) << std::endl;
  os << "Block size: " << sizeof(fspp::internal::Block) << std::endl;
  os << "SuperBlock size: " << sizeof(fspp::internal::SuperBlock) << std::endl;
  os << "Link size: " << sizeof(fspp::internal::Link) << std::endl;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <path>" << std::endl;
    return 1;
  }

  printSizes(std::cerr);

  // filesystem init
  std::string filesystem_path(argv[1]);
  fspp::FileSystemClient fs(filesystem_path);

  // help const
  const std::string help =
      "Commands:\n"
      "\texit\n\t\texit app\n"
      "\thelp\n\t\tshow this message\n"
      "\tmkfile <filepath>\n\t\tcreate file\n"
      "\trmfile <filepath>\n\t\tdelete file\n"
      "\tmkdir <dirpath>\n\t\tcreate directory\n"
      "\trmdir <dirpath>\n\t\tdelete directory\n"
      "\tstore <from_path> <to_path>\n\t\tstore from outer filesystem to app filesystem\n"
      "\tload <from_path> <to_path>\n\t\tload to outer filesystem from app filesystem";

  // regexes init
  const std::regex exit_regex(R"(^\s*exit\s*$)");
  const std::regex help_regex(R"(^\s*help\s*$)");
  const std::regex mkfile_cmd_regex(R"(^\s*mkfile\s+)");
  const std::regex rmfile_cmd_regex(R"(^\s*rmfile\s+)");
  const std::regex mkdir_cmd_regex(R"(^\s*mkdir\s+)");
  const std::regex rmdir_cmd_regex(R"(^\s*rmdir\s+)");
  const std::regex lsdir_cmd_regex(R"(^\s*lsdir\s+)");
  const std::regex store_cmd_regex(R"(^\s*store\s+)");
  const std::regex load_regex(R"(^\s*load\s*([\w/]+)\s+([\w/]+)\s*$)");

  // main loop
  while (true) {
    std::string input;
    if (!std::getline(std::cin, input)) {
      std::cerr << "EOF: exiting" << std::endl;
      break;
    }

    std::smatch match;
    if (std::regex_match(input, match, exit_regex)) {
      std::cerr << "exit command: exiting" << std::endl;
      break;

    } else if (std::regex_search(input, match, mkfile_cmd_regex)) {
      int result = mkfile(fs, input);
      std::cerr << (result < 0 ? "fail" : "success") << std::endl;

    } else if (std::regex_search(input, match, rmfile_cmd_regex)) {
      int result = rmfile(fs, input);
      std::cerr << (result < 0 ? "fail" : "success") << std::endl;

    } else if (std::regex_search(input, match, mkdir_cmd_regex)) {
      int result = mkdir(fs, input);
      std::cerr << (result < 0 ? "fail" : "success") << std::endl;

    } else if (std::regex_search(input, match, rmdir_cmd_regex)) {
      int result = rmdir(fs, input);
      std::cerr << (result < 0 ? "fail" : "success") << std::endl;

    } else if (std::regex_search(input, match, lsdir_cmd_regex)) {
      int result = lsdir(fs, input);
      std::cerr << (result < 0 ? "fail" : "success") << std::endl;

    } else if (std::regex_search(input, match, store_cmd_regex)) {
      int result = store(fs, input);
      std::cerr << (result < 0 ? "fail" : "success") << std::endl;


    } else if (std::regex_match(input, match, load_regex)) {
      std::cerr << "load command: ";

      const auto& from_path = match[1].str();
      const auto& to_path = match[2].str();
      if (!check_path(from_path) || !check_path(to_path)) {
        std::cout << "Wrong from or to path format" << std::endl;
        std::cerr << "continuing" << std::endl;
        continue;
      }

      std::cerr << "success" << std::endl;
    } else if (std::regex_match(input, match, help_regex)) {
      std::cerr << "help command" << std::endl;
      std::cout << help << std::endl;
    } else {
      std::cerr << "unknown command" << std::endl;
      std::cout << "Unknown command" << std::endl;
      std::cout << help << std::endl;
    }
  }

  return 0;
}