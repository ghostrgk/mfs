#pragma once

#include <cstdint>
#include "bitset.h"
#include "block.h"

#ifndef NORMAL_ILIST
#include <cassert>
#endif

namespace fspp::internal {

#ifdef NORMAL_ILIST
class InodesList {
 public:
  uint64_t getBlockIdByIndex(uint64_t index);
  uint64_t size() const;

 private:
  class IndirectionLevel1 {};

  class IndirectionLevel2 {};

  uint64_t block_ids_[10];
  IndirectionLevel1 level1_;
  IndirectionLevel2 level2_;
};
#else
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
#endif

struct Link {
  bool is_alive{true};
  char name[MAX_LINK_NAME_LEN + 1]{};
  uint64_t inode_id{0};
};

struct Inode {
  bool is_dir{false};
  uint64_t blocks_count{0};
  uint64_t file_size{0};
  InodesList inodes_list;
};

class InodeSpace {
 public:
  InodeSpace() = default;
  InodeSpace(uint64_t* inode_num_ptr, uint64_t* free_inode_num_ptr, uint8_t* bit_set_bytes, BlockSpace* blocks,
             Inode* inode_bytes);

  Inode& getInodeById(uint64_t inode_id);

  /*!
   * attempts to read up to _count_ bytes from file associated with _inode_ at _offset_ (in bytes) into _buffer_
   * @param inode
   * @param buffer
   * @param offset
   * @param count
   * @return on success, the number of bytes read is returned. on error, -1 is returned.
   */
  int read(Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) const;

  /*!
   * attempts to write up to _count_ bytes to file associated with inode at _offset_ (in bytes) from _buffer_
   * @param buffer
   * @param offset
   * @param count
   * @return on success, the number of bytes written is returned. on error, -1 is returned.
   */
  int write(Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count);
  int append(Inode* inode_ptr, const void* buffer, uint64_t count);

 public:
  static int addBlockToInode(Inode& inode, uint64_t block_id);
  uint64_t createInode();
  void deleteInode(uint64_t inode_id);

  /*!
   * @note doesn't check possible existence of the entry
   */
  int addDirectoryEntry(Inode* inode_ptr, const char* name, bool is_dir);

  int gcLaterRename(uint64_t inode_id);

 private:
  int clearInode(Inode* inode_ptr);
  Block& getBlockByIndex(Inode& inode, uint64_t index) const;
  int extend(Inode& inode, uint64_t new_size);

 private:
  uint64_t* inode_num_ptr_{nullptr};
  uint64_t* free_inode_num_ptr_{nullptr};
  BitSet bit_set_{};

  BlockSpace* blocks_{nullptr};
  Inode* inodes_{nullptr};
};

}  // namespace fspp::internal