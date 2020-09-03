#include <iostream>
#include <regex>

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

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

int store(int socket_fd, const std::string& query) {
  static const std::regex full_regex(R"(^\s*store\s+(/|(/[-\d\w.]+)+)\s+(/|(/[-\d\w.]+)+)\s*$)");
  std::cerr << "store command" << std::endl;

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong from_path or to_path format" << std::endl;
    return -1;
  }

  const std::string& from_path = match[1];
  std::string to_path = match[3];
  std::cerr << "(from_path=" << from_path << ")" << std::endl;
  std::cerr << "(to_path=" << to_path << ")" << std::endl;

  int from_fd = open(from_path.c_str(), O_RDONLY);
  if (from_fd < 0) {
    std::cout << "Can't open from_file" << std::endl;
    return -1;
  }
  std::cerr << "file opened" << std::endl;

  struct stat stat_buf {};
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
  std::cerr << "file len read: (from_file_len=" << from_file_len << ")" << std::endl;

  std::string remote_query = query;
  if (write(socket_fd, remote_query.c_str(), remote_query.size()) < 0) {
    close(from_fd);
    return -1;
  }

  char query_correctness_response[4096];
  int query_correctness_response_len = read(socket_fd, query_correctness_response, sizeof(query_correctness_response));
  if (query_correctness_response_len < 0) {
    close(from_fd);
    return -1;
  }
  std::cerr << "(query_correctness_response=" << std::string(query_correctness_response, query_correctness_response_len)
            << std::endl;

  if (write(socket_fd, &from_file_len, sizeof(from_file_len)) < 0) {
    close(from_fd);
    return -1;
  }

  for (uint64_t bytes_sent = 0; bytes_sent < from_file_len;) {
    char buffer[4096];
    uint64_t current_read_len = std::min(sizeof(buffer), from_file_len - bytes_sent);
    int bytes_read = read(from_fd, buffer, current_read_len);
    if (bytes_read < 0) {
      // todo: what should program do?
      close(from_fd);
      return -1;
    }

    if (write(socket_fd, buffer, bytes_read) < 0) {
      close(from_fd);
      return -1;
    }

    bytes_sent += bytes_read;
  }

  close(from_fd);

  return 0;
}

int load(int socket_fd, const std::string& query) {
  static const std::regex full_regex(R"(^\s*load\s+(/|(/[\w.]+)+)\s+(/|(/[\w.]+)+)\s*$)");
  std::cerr << "load command: ";

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong from_path or to_path format" << std::endl;
    return -1;
  }

  const std::string& from_path = match[1];
  std::string to_path = match[3];
  std::cerr << "(from_path=" << from_path << ") ";
  std::cerr << "(to_path=" << to_path << ") ";

  (void)socket_fd;

  const char* from_basename_start = strrchr(from_path.c_str(), '/') + 1;
  std::string from_basename(from_basename_start);

  int to_fd = open(to_path.c_str(), O_RDWR);
  if (to_fd < 0) {
    if (errno != ENOENT) {
      std::cout << "Can't open to_file/to_directory" << std::endl;
      return -1;
    }

    to_fd = open(to_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0640);
    if (to_fd < 0) {
      std::cout << "Can't create to_file" << std::endl;
      return -1;
    }
  }

  struct stat stat_buf {};
  if (fstat(to_fd, &stat_buf) < 0) {
    std::cout << "Can't read to_file stat" << std::endl;

    close(to_fd);
    return -1;
  }

  if (S_ISDIR(stat_buf.st_mode)) {
    to_path += from_basename;
    close(to_fd);
    to_fd = open(to_path.c_str(), O_RDWR);
    if (to_fd < 0) {
      if (errno != ENOENT) {
        std::cout << "Can't open to_file" << std::endl;
        return -1;
      }

      to_fd = open(to_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0640);
      if (to_fd < 0) {
        std::cout << "Can't create to_file" << std::endl;
        return -1;
      }
    }
  }

  std::string remote_query = query;
  if (write(socket_fd, remote_query.c_str(), remote_query.size()) < 0) {
    close(to_fd);
    return -1;
  }

  char query_correctness_response[4096];
  int query_correctness_response_len = read(socket_fd, query_correctness_response, sizeof(query_correctness_response));
  if (query_correctness_response_len < 0) {
    close(to_fd);
    return -1;
  }
  std::cerr << "(query_correctness_response=" << std::string(query_correctness_response, query_correctness_response_len)
            << std::endl;

  uint64_t from_file_len;

  int bytes_read;
  if ((bytes_read = read(socket_fd, &from_file_len, sizeof(from_file_len))) < 0 ||
      bytes_read != sizeof(from_file_len)) {
    close(to_fd);
    return -1;
  }

  for (uint64_t bytes_written = 0; bytes_written < from_file_len;) {
    char buffer[4096];
    if ((bytes_read = read(socket_fd, buffer, sizeof(buffer))) < 0) {
      // maybe need to delete file
      std::cout << "Can't receive file content" << std::endl;
      return -1;
    }

    uint64_t current_write_len = std::min((uint64_t)bytes_read, from_file_len - bytes_written);
    if (write(to_fd, buffer, current_write_len) < 0) {
      std::cout << "Writing to file failed" << std::endl;
      return -1;
    }
    bytes_written += current_write_len;
  }

  return 0;
}

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

    } else if (std::regex_search(input, match, store_cmd_regex)) {
      if (store(socket_fd, input) < 0) {
        break;
      }

    } else if (std::regex_search(input, match, load_cmd_regex)) {
      if (load(socket_fd, input) < 0) {
        break;
      }

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