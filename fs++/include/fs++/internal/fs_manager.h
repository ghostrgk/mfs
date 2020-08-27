#pragma once

#include <string>

#include "block.h"
#include "inode.h"
#include "superblock.h"

namespace fspp::internal {

class FSManager {
 public:
  explicit FSManager(const std::string& ffile_path);
  ~FSManager();

  int createFDE(const std::string& fde_path, bool is_dir);

  int getFDEInodeId(std::string fde_path, uint64_t* result_ptr);

  bool existsFDE(const std::string& fde_path);

  int deleteFDE(const std::string& fde_path, bool is_dir);

  int deleteInode(uint64_t inode_id);
  int getChildId(Inode& parent_inode, const std::string& name, uint64_t* result_ptr);
  bool existsChild(Inode& parent_inode, const std::string& name);
  int createChild(Inode& parent_inode, const std::string& name, bool is_dir);

  int read(Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) const;
  int write(Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count);
  int append(Inode* inode_ptr, const void* buffer, uint64_t count);

  Inode& getInodeById(uint64_t inode_id);

 private:
  int getFDEInodeParentId(std::string fde_path, uint64_t* result_ptr) ;

 private:
  uint8_t* file_bytes_{nullptr};
  internal::SuperBlock* super_block_ptr_;
  internal::InodeSpace inodes_;
  internal::BlockSpace blocks_;
};

}  // namespace fspp::internal