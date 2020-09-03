#include "processing.h"

#include <regex>

#include <unistd.h>

#include <support/files.h>

#include "cmds.h"
#include "config.h"
#include "support.h"


int process_connection(fspp::FileSystemClient& fs, int socket_fd) {
  static const std::regex exit_regex(R"(^\s*exit\s*$)");
  static const std::regex store_cmd_regex(R"(^\s*store\s+)");
  static const std::regex load_cmd_regex(R"(^\s*load\s+)");

  int bytes_read;
  char buffer[MAX_QUERY_LEN];

  // main connection loop
  while (true) {
    if ((bytes_read = read(socket_fd, &buffer, sizeof(buffer))) < 0) {
      return -1;
    }

    std::string input(buffer, bytes_read);

    std::ostringstream user_output;

    // connection dependent commands
    // looks bad
    // todo: fix
    std::smatch match;
    if (std::regex_match(input, match, exit_regex)) {
      std::cerr << "exit command: exiting" << std::endl;
      return 0;

    } else if (std::regex_search(input, match, store_cmd_regex)) {
      int result = store(socket_fd, fs, input, user_output);
      std::cerr << (result < 0 ? "fail" : "success") << std::endl;

    } else if (std::regex_search(input, match, load_cmd_regex)) {
      int result = load(socket_fd, fs, input, user_output);
      std::cerr << (result < 0 ? "fail" : "success") << std::endl;
    } else {
      // process other commands
      if (process_input(fs, input, user_output) < 0) {
        return -1;
      }
    }

    // writing output
    std::string response = user_output.str();
    if (writeall(socket_fd, response.c_str(), response.size()) < 0) {
      return -1;
    }
  }
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
