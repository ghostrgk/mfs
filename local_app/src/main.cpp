#include <iostream>
#include <string>
#include <regex>

#include <fs++/filesystem_client.h>

bool check_path(const std::string& string) {
  static const std::regex path_regex("(/\\w+)+");
  std::smatch path_match;
  return std::regex_match(string, path_match, path_regex);
}

int mkfile(fspp::FileSystemClient& fs, const std::string& query) {
  static const std::regex full_regex(R"(^\s*mkfile\s+(/|((/\w+)+))\s*$)");
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
  static const std::regex full_regex(R"(^\s*rmfile\s+(/|(/\w+)+)\s*$)");
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
  static const std::regex full_regex(R"(^\s*mkdir\s+(/|((/\w+)+))\s*$)");
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
  static const std::regex full_regex(R"(^\s*rmdir\s+(/|(/\w+)+)\s*$)");
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
  static const std::regex full_regex(R"(^\s*lsdir\s+(/|(/\w+)+)\s*$)");
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
  const std::regex store_regex(R"(^\s*store\s*([\w/]+)\s+([\w/]+)\s*$)");
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

    } else if (std::regex_match(input, match, store_regex)) {
      std::cerr << "store command: ";

      const std::string& from_path = match[1];
      const std::string& to_path = match[2];

      if (!check_path(from_path) || !check_path(to_path)) {
        std::cout << "Wrong from or to path format" << std::endl;
        std::cerr << "continuing" << std::endl;
        continue;
      }

      std::cerr << "success" << std::endl;
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