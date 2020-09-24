#pragma once

#include "block.h"
#include "inode.h"
#include "superblock.h"

struct FileSystem {
  int fd_;
  uint8_t* file_bytes_;
  struct SuperBlock* super_block_ptr_;
  struct Inodes inodes_;
  struct Blocks blocks_;
};

int FileSystem_init(struct FileSystem* fs, const char* ffile_path);
void FileSystem_free(struct FileSystem* fs);

int createFDE(struct FileSystem* fs, const char* fde_path, bool is_dir);
int getFDEInodeId(struct FileSystem* fs, const char* fde_path, uint64_t* result_ptr);
bool existsFDE(struct FileSystem* fs, const char* fde_path);
int deleteFDE(struct FileSystem* fs, const char* fde_path, bool is_dir);

Inode* FS_getInodeById(struct FileSystem* fs, uint64_t inode_id);

int createChild(struct FileSystem* fs, Inode* parent_inode, const char* name, bool is_dir);
int getChildId(struct FileSystem* fs, Inode* parent_inode, const char* name, uint64_t* result_ptr);
bool existsChild(struct FileSystem* fs, Inode* parent_inode, const char* name);

int FS_read(struct FileSystem* fs, Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count);
int FS_write(struct FileSystem* fs, Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count);
int FS_append(struct FileSystem* fs, Inode* inode_ptr, const void* buffer, uint64_t count);

// maybe need to change interface
int FS_listDir(struct FileSystem* fs, const char* dir_path, char** output);

int getFDEInodeParentId(struct FileSystem* fs, const char* fde_path, uint64_t* result_ptr);
int FS_deleteInode(struct FileSystem* fs, uint64_t inode_id);

