#include <iostream>
#include <regex>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

int proxy_command(int socket_fd, const std::string& query) {
  int bytes_sent = write(socket_fd, query.c_str(), query.size());
  if (bytes_sent < 0) {
    perror("Query sending failed");
  }

  char buffer[4096];
  int bytes_received = read(socket_fd, &buffer, sizeof(buffer));
  if (bytes_received < 0) {
    perror("Response receiving failed");
    return -1;
  }

  std::cout << std::string(buffer, bytes_received) << std::endl;

  return 0;
}

// int mkfile(int socket_fd, const std::string& query) {
//  int bytes_sent = write(socket_fd, query.c_str(), query.size());
//  if (bytes_sent < 0) {
//    perror("Query sending failed");
//  }
//
//  char buffer[4096];
//  int bytes_received = read(socket_fd, &buffer, sizeof(buffer));
//  if (bytes_received < 0) {
//    perror("Response receiving failed");
//    return -1;
//  }
//
//  return 0;
//}
//
// int rmfile(int socket_fd, const std::string& query) {
//  static const std::regex full_regex(R"(^\s*rmfile\s+(/|(/[\w.]+)+)\s*$)");
//  std::cerr << "rmfile command: ";
//
//  std::smatch match;
//  if (!std::regex_match(query, match, full_regex)) {
//    std::cout << "Wrong path format" << std::endl;
//    return -1;
//  }
//
//  const std::string& path = match[1];
//  std::cerr << "(path=" << path << ") ";
//
//  if (!fs.existsFile(path)) {
//    std::cout << "File doesn't exist" << std::endl;
//    return -1;
//  }
//
//  if (fs.deleteFile(path) < 0) {
//    return -1;
//  }
//
//  return 0;
//}
//
// int mkdir(int socket_fd, const std::string& query) {
//  static const std::regex full_regex(R"(^\s*mkdir\s+(/|((/[\w.]+)+))\s*$)");
//  std::cerr << "mkdir command: ";
//
//  std::smatch match;
//  if (!std::regex_match(query, match, full_regex)) {
//    std::cout << "Wrong path format" << std::endl;
//    return -1;
//  }
//
//  const std::string& path = match[1];
//  std::cerr << "(path=" << path << ") ";
//
//  if (fs.existsDir(path)) {
//    std::cout << "Directory already exists" << std::endl;
//    return -1;
//  }
//
//  if (fs.existsFile(path)) {
//    std::cout << "Already exists file with the same name" << std::endl;
//    return -1;
//  }
//
//  if (fs.createDir(path) < 0) {
//    return -1;
//  }
//
//  return 0;
//}
//
// int rmdir(int socket_fd, const std::string& query) {
//  static const std::regex full_regex(R"(^\s*rmdir\s+(/|(/[\w.]+)+)\s*$)");
//  std::cerr << "rmdir command: ";
//
//  std::smatch match;
//  if (!std::regex_match(query, match, full_regex)) {
//    std::cout << "Wrong path format" << std::endl;
//    return -1;
//  }
//
//  const std::string& path = match[1];
//  std::cerr << "(path=" << path << ") ";
//
//  if (!fs.existsDir(path)) {
//    std::cout << "Directory doesn't exist" << std::endl;
//    return -1;
//  }
//
//  if (path == "/") {
//    std::cout << "You can't remove root directory" << std::endl;
//    return -1;
//  }
//
//  if (fs.deleteDir(path) < 0) {
//    return -1;
//  }
//
//  return 0;
//}
//
// int lsdir(int socket_fd, const std::string& query) {
//  static const std::regex full_regex(R"(^\s*lsdir\s+(/|(/[\w.]+)+)\s*$)");
//  std::cerr << "lsdir command: ";
//
//  std::smatch match;
//  if (!std::regex_match(query, match, full_regex)) {
//    std::cout << "Wrong path format" << std::endl;
//    return -1;
//  }
//
//  const std::string& path = match[1];
//  std::cerr << "(path=" << path << ") ";
//
//  if (!fs.existsDir(path)) {
//    std::cout << "Directory doesn't exist" << std::endl;
//    return -1;
//  }
//
//  std::string output;
//  if (fs.listDir(path, output) < 0) {
//    return -1;
//  }
//
//  std::cout << output << std::endl;
//
//  return 0;
//}
//
// int store(int socket_fd, const std::string& query) {
//  static const std::regex full_regex(R"(^\s*store\s+(/|(/[-\d\w.]+)+)\s+(/|(/[-\d\w.]+)+)\s*$)");
//  std::cerr << "store command: ";
//
//  std::smatch match;
//  if (!std::regex_match(query, match, full_regex)) {
//    std::cout << "Wrong from_path or to_path format" << std::endl;
//    return -1;
//  }
//
//  const std::string& from_path = match[1];
//  std::string to_path = match[3];
//  std::cerr << "(from_path=" << from_path << ") ";
//  std::cerr << "(to_path=" << to_path << ") ";
//
//  const char* from_basename_start = strrchr(from_path.c_str(), '/') + 1;
//  assert(from_basename_start != nullptr);
//  std::string from_basename(from_basename_start);
//
//  if (fs.existsDir(to_path)) {
//    to_path += from_basename;
//  }
//
//  int from_fd = open(from_path.c_str(), O_RDONLY);
//  if (from_fd < 0) {
//    std::cout << "Can't open from_file" << std::endl;
//    return -1;
//  }
//
//  struct stat stat_buf {};
//  if (fstat(from_fd, &stat_buf) < 0) {
//    std::cout << "Can't read from_file length" << std::endl;
//
//    close(from_fd);
//    return -1;
//  }
//
//  if (S_ISDIR(stat_buf.st_mode)) {
//    std::cout << "You can't store directory. Only storing files is supporting" << std::endl;
//
//    close(from_fd);
//    return -1;
//  }
//
//  uint64_t from_file_len = stat_buf.st_size;
//  void* from_file_content = mmap64(nullptr, from_file_len, PROT_READ, MAP_PRIVATE, from_fd, 0);
//
//  if (!fs.existsFile(to_path)) {
//    if (fs.createFile(to_path) < 0) {
//      std::cout << "Can't create file in app filesystem" << std::endl;
//
//      munmap(from_file_content, from_file_len);
//      close(from_fd);
//      return -1;
//    }
//  }
//
//  int bytes_written = fs.writeFileContent(to_path, 0, from_file_content, from_file_len);
//  if (bytes_written == -1) {
//    std::cout << "Can't write to app filesystem" << std::endl;
//
//    munmap(from_file_content, from_file_len);
//    close(from_fd);
//    return -1;
//  }
//
//  if ((uint64_t)bytes_written != from_file_len) {
//    std::cerr << "possible file corruption" << std::endl;
//  }
//
//  munmap(from_file_content, from_file_len);
//  close(from_fd);
//
//  return 0;
//}
//
// int load(int socket_fd, const std::string& query) {
//  static const std::regex full_regex(R"(^\s*load\s+(/|(/[\w.]+)+)\s+(/|(/[\w.]+)+)\s*$)");
//  std::cerr << "load command: ";
//
//  std::smatch match;
//  if (!std::regex_match(query, match, full_regex)) {
//    std::cout << "Wrong from_path or to_path format" << std::endl;
//    return -1;
//  }
//
//  const std::string& from_path = match[1];
//  std::string to_path = match[3];
//  std::cerr << "(from_path=" << from_path << ") ";
//  std::cerr << "(to_path=" << to_path << ") ";
//
//  if (!fs.existsFile(from_path)) {
//    std::cout << "Requested file doesn't exist" << std::endl;
//    return -1;
//  }
//
//  const char* from_basename_start = strrchr(from_path.c_str(), '/') + 1;
//  assert(from_basename_start != nullptr);
//  std::string from_basename(from_basename_start);
//
//  int to_fd = open(to_path.c_str(), O_RDWR);
//  if (to_fd < 0) {
//    if (errno != ENOENT) {
//      std::cout << "Can't open to_file/to_directory" << std::endl;
//      return -1;
//    }
//
//    to_fd = open(to_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0640);
//    if (to_fd < 0) {
//      std::cout << "Can't create to_file" << std::endl;
//      return -1;
//    }
//  }
//
//  struct stat stat_buf {};
//  if (fstat(to_fd, &stat_buf) < 0) {
//    std::cout << "Can't read to_file stat" << std::endl;
//
//    close(to_fd);
//    return -1;
//  }
//
//  if (S_ISDIR(stat_buf.st_mode)) {
//    to_path += from_basename;
//    close(to_fd);
//    to_fd = open(to_path.c_str(), O_RDWR);
//    if (to_fd < 0) {
//      if (errno != ENOENT) {
//        std::cout << "Can't open to_file" << std::endl;
//        return -1;
//      }
//
//      to_fd = open(to_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0640);
//      if (to_fd < 0) {
//        std::cout << "Can't create to_file" << std::endl;
//        return -1;
//      }
//    }
//  }
//
//  uint64_t from_file_len = fs.fileSize(from_path);
//  ftruncate(to_fd, from_file_len);
//  void* to_file_content = mmap64(nullptr, from_file_len, PROT_WRITE, MAP_SHARED, to_fd, 0);
//
//  int bytes_read = fs.readFileContent(from_path, 0, to_file_content, from_file_len);
//  if (bytes_read == -1) {
//    std::cout << "Can't write to app filesystem" << std::endl;
//
//    munmap(to_file_content, from_file_len);
//    close(to_fd);
//    return -1;
//  }
//
//  if ((uint64_t)bytes_read != from_file_len) {
//    std::cerr << "possible file corruption" << std::endl;
//  }
//
//  munmap(to_file_content, from_file_len);
//  close(to_fd);
//
//  return 0;
//}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <address> <port>" << std::endl;
    return EXIT_FAILURE;
  }

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
  const std::regex load_cmd_regex(R"(^\s*load\s+)");

  // args parse
  char* ip_address = argv[1];
  char* parse_end = nullptr;
  long parsed_port = strtol(argv[2], &parse_end, 10);
  if (*parse_end != '\0' || parsed_port < 0 || parsed_port > std::numeric_limits<uint16_t>::max()) {
    std::cerr << "Wrong port format" << std::endl;
    return EXIT_FAILURE;
  }
  uint16_t port = parsed_port;

  // socket init
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("Can't create socket");
    return EXIT_FAILURE;
  }

  struct sockaddr_in server_address = {.sin_family = AF_INET, .sin_port = htons(port)};
  if (inet_pton(AF_INET, ip_address, &server_address.sin_addr) <= 0) {
    std::cerr << "Wrong address format" << std::endl;
    return EXIT_FAILURE;
  }

  if (connect(socket_fd, reinterpret_cast<const sockaddr*>(&server_address), sizeof(server_address)) < 0) {
    perror("Can't connect to the server");
    return EXIT_FAILURE;
  }

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

    } else if (std::regex_search(input, match, mkfile_cmd_regex) || std::regex_search(input, match, rmfile_cmd_regex) ||
               std::regex_search(input, match, mkdir_cmd_regex) || std::regex_search(input, match, rmdir_cmd_regex) ||
               std::regex_search(input, match, lsdir_cmd_regex)) {
      if (proxy_command(socket_fd, input) < 0) {
        break;
      }

    } else if (std::regex_search(input, match, rmfile_cmd_regex)) {
      if (proxy_command(socket_fd, input) < 0) {
        break;
      }

    } else if (std::regex_search(input, match, mkdir_cmd_regex)) {
      if (proxy_command(socket_fd, input) < 0) {
        break;
      }

    } else if (std::regex_search(input, match, rmdir_cmd_regex)) {
      if (proxy_command(socket_fd, input) < 0) {
        break;
      }

    } else if (std::regex_search(input, match, lsdir_cmd_regex)) {
      if (proxy_command(socket_fd, input) < 0) {
        break;
      }

      // todo: implement load and store

      //    } else if (std::regex_search(input, match, store_cmd_regex)) {
      //      if (store(socket_fd, input) < 0) {
      //        break;
      //      }
      //
      //    } else if (std::regex_search(input, match, load_cmd_regex)) {
      //      if (load(socket_fd, input) < 0) {
      //        break;
      //      }

    } else if (std::regex_match(input, match, help_regex)) {
      std::cerr << "help command" << std::endl;
      std::cout << help << std::endl;
    } else {
      std::cerr << "unknown command" << std::endl;
      std::cout << "Unknown command" << std::endl;
      std::cout << help << std::endl;
    }
  }

  shutdown(socket_fd, SHUT_RDWR);
  close(socket_fd);

  return 0;
}