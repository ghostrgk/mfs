#include <functional>
#include <iostream>
#include <string>
#include <regex>

#include <fs++/filesystem_client.h>

bool check_path(const std::string& string) {
  static const std::regex path_regex("(/\\w+)+");
  std::smatch path_match;
  return std::regex_match(string, path_match, path_regex);
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
  const std::regex exit_regex("^\\s*exit\\s*$");
  const std::regex help_regex("^\\s*help\\s*$");
  const std::regex mkfile_regex(R"(^\s*mkfile\s*([\w/]+)\s*$)");
  const std::regex rmfile_regex(R"(^\s*rmfile\s*([\w/]+)\s*$)");
  const std::regex mkdir_regex(R"(^\s*mkdir\s*([\w/]+)\s*$)");
  const std::regex rmdir_regex(R"(^\s*rmdir\s*([\w/]+)\s*$)");
  const std::regex lsdir_regex(R"(^\s*lsdir\s*([\w/]+)\s*$)");
  const std::regex store_regex(R"(^\s*store\s*([\w/]+)\s*([\w/]+)\s*$)");
  const std::regex load_regex(R"(^\s*load\s*([\w/]+)\s*([\w/]+)\s*$)");

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
    } else if (std::regex_match(input, match, mkfile_regex)) {
      std::cerr << "mkfile command: ";

      const auto& path = match[1].str();
      if (!check_path(path)) {
        std::cout << "Wrong path format" << std::endl;
        std::cerr << "continuing" << std::endl;
        continue;
      }

      if (fs.createFile(path) < 0) {
        std::cerr << "fail" << std::endl;
        continue;
      }

      std::cerr << "success" << std::endl;
    } else if (std::regex_match(input, match, rmfile_regex)) {
      std::cerr << "rmfile command: ";

      const auto& path = match[1].str();
      if (!check_path(path)) {
        std::cout << "Wrong path format" << std::endl;
        std::cerr << "continuing" << std::endl;
        continue;
      }

      if (fs.deleteFile(path) < 0) {
        std::cerr << "fail" << std::endl;
        continue;
      }

      std::cerr << "success" << std::endl;
    } else if (std::regex_match(input, match, mkdir_regex)) {
      std::cerr << "mkdir command: ";

      const auto& path = match[1].str();
      if (!check_path(path)) {
        std::cout << "Wrong path format" << std::endl;
        std::cerr << "continuing" << std::endl;
        continue;
      }

      if (fs.createDir(path) < 0) {
        std::cerr << "fail" << std::endl;
        continue;
      }

      std::cerr << "success" << std::endl;
    } else if (std::regex_match(input, match, rmdir_regex)) {
      std::cerr << "rmdir command: ";

      const auto& path = match[1].str();
      if (!check_path(path)) {
        std::cout << "Wrong path format" << std::endl;
        std::cerr << "continuing" << std::endl;
        continue;
      }

      if (fs.deleteDir(path) < 0) {
        std::cerr << "fail" << std::endl;
        continue;
      }

      std::cerr << "success" << std::endl;
    } else if (std::regex_match(input, match, lsdir_regex)) {
      std::cerr << "lsdir command: ";

      const auto& path = match[1].str();
      if (!check_path(path) && !(path == "/")) {
        std::cout << "Wrong path format" << std::endl;
        std::cerr << "continuing" << std::endl;
        continue;
      }

      std::string output;
      if (fs.listDir(path, output) < 0) {
        std::cerr << "fail" << std::endl;
        continue;
      }
      std::cout << output << std::endl;

      std::cerr << "success" << std::endl;
    } else if (std::regex_match(input, match, store_regex)) {
      std::cerr << "store command: ";

      const auto& from_path = match[1].str();
      const auto& to_path = match[2].str();
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