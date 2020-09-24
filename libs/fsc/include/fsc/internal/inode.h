#pragma once

#include <stdint.h>
#include "bitset.h"
#include "block.h"

#ifdef NORMAL_ILIST

#include "ilist.h"
#else
namespace fspp::internal {
class InodesList {
 public:
  [[nodiscard]] uint64_t getBlockIdByIndex(uint64_t index) const {
    assert(index < size_);
    return block_ids_[index];
  }

  [[nodiscard]] uint64_t max_size() const {
    return INODE_MAX_BLOCK_COUNT;
  }

  [[nodiscard]] uint64_t size() const {
    return size_;
  }

  void clear() {
    size_ = 0;
  }

  // aaaaaaa
  void setSize(uint64_t new_size) {
    size_ = new_size;
  }

  int addBlock(uint64_t block_id) {
    if (size_ + 1 == INODE_MAX_BLOCK_COUNT) {
      return -1;
    }

    block_ids_[size_++] = block_id;
    return 0;
  }

 private:
  uint64_t size_{0};
  uint64_t block_ids_[INODE_MAX_BLOCK_COUNT];
};
}  // namespace fspp::internal
#endif

typedef struct Link {
  bool is_alive;
  char name[MAX_LINK_NAME_LEN + 1];
  uint64_t inode_id;
} Link;

typedef struct Inode {
  bool is_dir;
  uint64_t blocks_count;
  uint64_t file_size;
  struct InodesList inodes_list;
} Inode;

/*!
 * Does all work that connected to inodes
 */
struct Inodes {
  struct Inode* inodes_ptr_start_;
  struct Blocks* blocks_;
  uint64_t* free_inode_num_ptr_;
  struct BitSet bit_set_;
};

int Inodes_init(struct Inodes* inodes, void* inodes_ptr_start, struct Blocks* blocks, uint64_t* free_inode_num_ptr,
                struct BitSet inodes_bitset);
Inode* getInodeById(struct Inodes* inodes, uint64_t inode_id);

int Inode_read(struct Inodes* inodes, Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count);
int Inode_write(struct Inodes* inodes, Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count);
int Inode_append(struct Inodes* inodes, Inode* inode_ptr, const void* buffer, uint64_t count);

int addBlockToInode(struct Inodes* inodes, Inode* inode, uint64_t block_id);
uint64_t createInode(struct Inodes* inodes);
void deleteInode(struct Inodes* inodes, uint64_t inode_id);

/*!
 * @note doesn't check possible existence of the entry
 */
int addDirectoryEntry(struct Inodes* inodes, Inode* inode_ptr, const char* name, bool is_dir);

int clearInode(struct Inodes* inodes, Inode* inode_ptr);
Block* getBlockByIndex(struct Inodes* inodes, Inode* inode, uint64_t index);
int extend(struct Inodes* inodes, Inode* inode, uint64_t new_size);
