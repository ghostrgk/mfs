#pragma once

#include <string>

#include "block.h"
#include "inode.h"
#include "superblock.h"

namespace fspp::internal {

class FileSystem {
 public:
  explicit FileSystem(const std::string& ffile_path);
  ~FileSystem();

  int createFDE(const std::string& fde_path, bool is_dir);
  int getFDEInodeId(std::string fde_path, uint64_t* result_ptr);
  bool existsFDE(const std::string& fde_path);
  int deleteFDE(const std::string& fde_path, bool is_dir);

  Inode& getInodeById(uint64_t inode_id);

  int createChild(Inode& parent_inode, const std::string& name, bool is_dir);
  int getChildId(Inode& parent_inode, const std::string& name, uint64_t* result_ptr);
  bool existsChild(Inode& parent_inode, const std::string& name);

  int read(Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) const;
  int write(Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count);
  [[maybe_unused]] int append(Inode* inode_ptr, const void* buffer, uint64_t count);

  // maybe need to change interface
  int listDir(const std::string& dir_path, std::string& output);

 private:
  int getFDEInodeParentId(std::string fde_path, uint64_t* result_ptr);
  int deleteInode(uint64_t inode_id);

 private:
  int fd_{-1};
  uint8_t* file_bytes_{nullptr};
  internal::SuperBlock* super_block_ptr_;
  internal::InodeSpace inodes_;
  internal::BlockSpace blocks_;
};

}  // namespace fspp::internal