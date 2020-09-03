#pragma once

#include <string>
#include <ostream>

#include <fs++/filesystem_client.h>

int process_connection(fspp::FileSystemClient& fs, int socket_fd);
int process_input(fspp::FileSystemClient& fs, const std::string& input, std::ostream& user_output);