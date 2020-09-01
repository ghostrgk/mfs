#include <iostream>
#include <string>
#include <sstream>
#include <regex>

#include <cstdlib>
#include <cstdio>
#include <csignal>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "cmds.h"
#include "support.h"

volatile bool ending = false;

void signal_handler(int signum) {
  (void)signum;
  ending = true;
}

void ignoring_handler(int signum) {
  (void)signum;
}

int init_signal_handling() {
  struct sigaction action {};
  action.sa_handler = signal_handler;
  action.sa_flags = SA_RESTART;
  if (sigaction(SIGTERM, &action, nullptr) < 0) {
    return -1;
  }

  if (sigaction(SIGINT, &action, nullptr) < 0) {
    return -1;
  }

  signal(SIGPIPE, SIG_IGN);

  return 0;
}

int init_socket() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    perror("Can't create socket");
    return -1;
  }

  struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(PORT)};
  address.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("Can't bind socket");
    return -1;
  }

  if (listen(server_fd, SOMAXCONN) < 0) {
    perror("Can't set socket to listen mode");
    return -1;
  }

  return server_fd;
}

int process_input(fspp::FileSystemClient& fs, const std::string& input, std::ostream& user_output) {
  static const std::string help =
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
  static const std::regex exit_regex(R"(^\s*exit\s*$)");
  static const std::regex help_regex(R"(^\s*help\s*$)");
  static const std::regex mkfile_cmd_regex(R"(^\s*mkfile\s+)");
  static const std::regex rmfile_cmd_regex(R"(^\s*rmfile\s+)");
  static const std::regex mkdir_cmd_regex(R"(^\s*mkdir\s+)");
  static const std::regex rmdir_cmd_regex(R"(^\s*rmdir\s+)");
  static const std::regex lsdir_cmd_regex(R"(^\s*lsdir\s+)");
  static const std::regex store_cmd_regex(R"(^\s*store\s+)");
  static const std::regex load_cmd_regex(R"(^\s*load\s+)");

  std::cerr << "(query=" << input << ")" << std::endl;

  std::smatch match;
  if (std::regex_match(input, match, exit_regex)) {
    std::cerr << "exit command: exiting" << std::endl;
    return -1;

  } else if (std::regex_search(input, match, mkfile_cmd_regex)) {
    int result = mkfile(fs, input, user_output);
    std::cerr << (result < 0 ? "fail" : "success") << std::endl;

  } else if (std::regex_search(input, match, rmfile_cmd_regex)) {
    int result = rmfile(fs, input, user_output);
    std::cerr << (result < 0 ? "fail" : "success") << std::endl;

  } else if (std::regex_search(input, match, mkdir_cmd_regex)) {
    int result = mkdir(fs, input, user_output);
    std::cerr << (result < 0 ? "fail" : "success") << std::endl;

  } else if (std::regex_search(input, match, rmdir_cmd_regex)) {
    int result = rmdir(fs, input, user_output);
    std::cerr << (result < 0 ? "fail" : "success") << std::endl;

  } else if (std::regex_search(input, match, lsdir_cmd_regex)) {
    int result = lsdir(fs, input, user_output);
    std::cerr << (result < 0 ? "fail" : "success") << std::endl;

  } else if (std::regex_search(input, match, store_cmd_regex)) {
    int result = store(fs, input, user_output);
    std::cerr << (result < 0 ? "fail" : "success") << std::endl;

  } else if (std::regex_search(input, match, load_cmd_regex)) {
    int result = load(fs, input, user_output);
    std::cerr << (result < 0 ? "fail" : "success") << std::endl;

  } else if (std::regex_match(input, match, help_regex)) {
    std::cerr << "help command" << std::endl;
    user_output << help << std::endl;
  } else {
    std::cerr << "unknown command" << std::endl;
    user_output << "Unknown command" << std::endl;
    user_output << help << std::endl;
  }

  return 0;
}

int process_connection(fspp::FileSystemClient& fs, int socket_fd) {
  int bytes_read;
  char buffer[MAX_QUERY_LEN];
  while (true) {
    if ((bytes_read = read(socket_fd, &buffer, sizeof(buffer))) < 0) {
      close(socket_fd);
      return -1;
    }

    std::string input(buffer, bytes_read);

    std::ostringstream user_output;
    if (process_input(fs, input, user_output) < 0) {
      close(socket_fd);
    }

    std::string response = user_output.str();
    if (write(socket_fd, response.c_str(), response.size()) < 0) {
      return -1;
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <ffile path>" << std::endl;
    return 1;
  }

  // filesystem init
  std::string filesystem_path(argv[1]);
  fspp::FileSystemClient fs(filesystem_path);

  // signal handling init
  if (init_signal_handling() < 0) {
    perror("Can't init signal handling");
    return EXIT_FAILURE;
  }

  // socket init
  int server_fd = init_socket();

  if (server_fd < 0) {
    LOG("Can't create socket");
    return EXIT_FAILURE;
  }

  // epoll init

  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    perror("epoll_create1");
    return EXIT_FAILURE;
  }

  struct epoll_event event = {.events = EPOLLIN};
  event.data.fd = server_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
    perror("Adding server fd failed");
    return EXIT_FAILURE;
  }

  // main loop
  while (!ending) {
    struct epoll_event events[MAX_EPOLL_EVENTS];

    // todo: maybe add concurrency?
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGTERM);

    int event_occurred = 0;
    if ((event_occurred = epoll_pwait(epoll_fd, events, MAX_EPOLL_EVENTS, -1, &mask)) < 0) {
      if (errno == EINTR) {
        continue;
      }

      perror("epoll_pwait");
      return EXIT_FAILURE;
    }

    for (int i = 0; i < event_occurred && !ending; ++i) {
      if (events[i].data.fd != server_fd) {
        LOG("Unexpected fd is in epoll queue");
        return EXIT_FAILURE;
      }

      struct sockaddr_in address {};
      socklen_t addrlen;

      int socket_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
      if (socket_fd < 0) {
        perror("Can't accept connection");
        continue;
      }

      int rc = process_connection(fs, socket_fd);
      if (rc == -1) {
      }
    }
  }

  return 0;
}
