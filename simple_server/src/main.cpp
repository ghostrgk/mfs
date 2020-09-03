#include <iostream>
#include <string>

#include <csignal>

#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include <fs++/filesystem_client.h>
#include <arpa/inet.h>

#include "config.h"
#include "inits.h"
#include "processing.h"
#include "support.h"

// signal handling
volatile bool ending = false;

void signal_handler(int signum) {
  (void)signum;
  ending = true;
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

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " <ffile path>" << std::endl;
    return EXIT_FAILURE;
  }

  // filesystem init
  std::string filesystem_path(argv[1]);
  fspp::FileSystemClient fs(filesystem_path);
  LOG_INFO("filesystem initialized");

  // signal handling init
  if (init_signal_handling() < 0) {
    LOG_ERROR_WITH_ERRNO_MSG("signal handling init failed");
    return EXIT_FAILURE;
  }
  LOG_INFO("signal handling initialized");

  // socket init
  int server_fd = init_socket(PORT);
  if (server_fd < 0) {
    LOG_ERROR("socket init failed");
    return EXIT_FAILURE;
  }
  LOG_INFO("socket initialized");

  // epoll init

  int epoll_fd = init_server_epoll(server_fd);
  if (epoll_fd < 0) {
    LOG_ERROR("epoll init failed");
  }
  LOG_INFO("epoll initialized");

  // main loop
  while (!ending) {
    struct epoll_event events[MAX_EPOLL_EVENTS];

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGTERM);

    int event_occurred;
    if ((event_occurred = epoll_pwait(epoll_fd, events, MAX_EPOLL_EVENTS, -1, &mask)) < 0) {
      if (errno == EINTR) {
        continue;
      }

      LOG_ERROR_WITH_ERRNO_MSG("epoll_pwait failed");
      return EXIT_FAILURE;
    }

    for (int i = 0; i < event_occurred && !ending; ++i) {
      if (events[i].data.fd != server_fd) {
        LOG_ERROR("Unexpected fd is in epoll queue");
        return EXIT_FAILURE;
      }

      struct sockaddr_in address {};
      socklen_t addrlen = sizeof(address);

      int socket_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&address), &addrlen);
      if (socket_fd < 0) {
        LOG_ERROR_WITH_ERRNO_MSG("connection accept failed");
        continue;
      }
      // todo: log address info
      LOG_INFO("connection accepted (address=" + std::string(inet_ntoa(address.sin_addr)) + ")");

      process_connection(fs, socket_fd);

      shutdown(socket_fd, SHUT_RDWR);
      close(socket_fd);
      LOG_INFO("connection finished");
    }
  }

  return EXIT_SUCCESS;
}
