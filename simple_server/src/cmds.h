#pragma once

#include <ostream>

#include <fs++/filesystem_client.h>

int mkfile(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output);
int rmfile(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output);
int mkdir(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output);
int rmdir(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output);
int lsdir(fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output);
int store(int socket_fd, fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output);
int load(int socket_fd, fspp::FileSystemClient& fs, const std::string& query, std::ostream& user_output);
