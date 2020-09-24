#pragma once

#include "internal/filesystem.h"

struct FileSystemClient {
  struct FileSystem fs_;
};

int FileSystemClient_init(struct FileSystemClient* fsc, const char* ffile_path);

bool existsDir(struct FileSystemClient* fsc, const char* dir_path);
int createDir(struct FileSystemClient* fsc, const char* dir_path);
int deleteDir(struct FileSystemClient* fsc, const char* dir_path);

/*
 * You must free *_output_
 */
int listDir(struct FileSystemClient* fsc, const char* dir_path, char** output);

bool existsFile(struct FileSystemClient* fsc, const char* file_path);
int createFile(struct FileSystemClient* fsc, const char* file_path);
int deleteFile(struct FileSystemClient* fsc, const char* file_path);

// one must use only after successful existsFile
uint64_t fileSize(struct FileSystemClient* fsc, const char* file_path);

int readFileContent(struct FileSystemClient* fsc, const char* file_path, uint64_t offset, void* buffer, uint64_t size);
int writeFileContent(struct FileSystemClient* fsc, const char* file_path, uint64_t offset, const void* buffer,
                     uint64_t size);
