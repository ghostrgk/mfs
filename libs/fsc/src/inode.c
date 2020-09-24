#include <fsc/internal/inode.h>

#include <string.h>

#include <fsc/internal/logging.h>
#include <stdlib.h>

int Inodes_init(struct Inodes* inodes, void* inodes_ptr_start, struct Blocks* blocks, uint64_t* free_inode_num_ptr,
                struct BitSet inodes_bitset) {
  inodes->inodes_ptr_start_ = inodes_ptr_start;
  inodes->blocks_ = blocks;
  inodes->free_inode_num_ptr_ = free_inode_num_ptr;
  inodes->bit_set_ = inodes_bitset;
  return 0;
}

Inode* getInodeById(struct Inodes* inodes, uint64_t inode_id) {
  return &(inodes->inodes_ptr_start_[inode_id]);
}

static uint64_t min(uint64_t lhs, uint64_t rhs) {
  if (lhs < rhs) {
    return lhs;
  }

  return rhs;
}

int Inode_read(struct Inodes* inodes, Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) {
  uint8_t* byte_buffer = buffer;
  uint64_t count_down = count;

  uint64_t buffer_offset = 0;
  uint64_t block_index = offset / BLOCK_SIZE;

  if (offset % BLOCK_SIZE != 0) {
    uint64_t block_offset = offset % BLOCK_SIZE;
    uint64_t start_size = min(count_down, BLOCK_SIZE - block_offset);

    Block* block = getBlockByIndex(inodes, inode_ptr, block_index);
    memcpy(buffer, block->bytes + block_offset, start_size);

    count_down -= start_size;
    offset += start_size;
    buffer_offset += start_size;

    ++block_index;
  }

  if (buffer_offset == count) {
    return buffer_offset;
  }

#ifdef REDUNDANT_CHECKS
  assert(offset % BLOCK_SIZE == 0);
#endif

  for (; block_index < inode_ptr->blocks_count && offset < inode_ptr->file_size && buffer_offset < count;
       ++block_index) {
    Block* block = getBlockByIndex(inodes, inode_ptr, block_index);
    const uint64_t read_size = min(count_down, BLOCK_SIZE);

    memcpy(byte_buffer + buffer_offset, block->bytes, read_size);

    count_down -= read_size;
    offset += read_size;
    buffer_offset += read_size;
  }

  return buffer_offset;
}

int Inode_write(struct Inodes* inodes, Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count) {
  const uint8_t* byte_buffer = buffer;
  uint64_t count_down = count;

  if (offset + count > inode_ptr->file_size) {
    if (extend(inodes, inode_ptr, offset + count) < 0) {
      FSC_LOG("INODE", "can't extend inodes");
      return -1;
    }
  }

#ifdef REDUNDANT_CHECKS
  assert(offset + count <= inode_ptr->file_size);
#endif

  uint64_t buffer_offset = 0;
  uint64_t block_index = offset / BLOCK_SIZE;

  if (offset % BLOCK_SIZE != 0) {
    uint64_t block_offset = offset % BLOCK_SIZE;
    uint64_t start_size = min(count_down, BLOCK_SIZE - block_offset);

    Block* block = getBlockByIndex(inodes, inode_ptr, block_index);
    memcpy(block->bytes + block_offset, buffer, start_size);

    count_down -= start_size;
    offset += start_size;
    buffer_offset += start_size;

    ++block_index;
  }

  if (buffer_offset == count) {
    return buffer_offset;
  }

#ifdef REDUNDANT_CHECKS
  assert(offset % BLOCK_SIZE == 0);
#endif

  for (; block_index < inode_ptr->blocks_count && offset < inode_ptr->file_size && buffer_offset < count;
       ++block_index) {
    Block* block = getBlockByIndex(inodes, inode_ptr, block_index);
    const uint64_t write_size = min(count_down, BLOCK_SIZE);

    memcpy(block->bytes, byte_buffer + buffer_offset, write_size);

    count_down -= write_size;
    offset += write_size;
    buffer_offset += write_size;
  }

  return buffer_offset;
}

int Inode_append(struct Inodes* inodes, Inode* inode_ptr, const void* buffer, uint64_t count) {
  return Inode_write(inodes, inode_ptr, buffer, inode_ptr->file_size, count);
}

int addBlockToInode(struct Inodes* inodes, Inode* inode, uint64_t block_id) {
  if (addBlock(&inode->inodes_list, inodes->blocks_, block_id) < 0) {
    return -1;
  }

  ++inode->blocks_count;
  return 0;
}

uint64_t createInode(struct Inodes* inodes) {
  assert(*inodes->free_inode_num_ptr_ != 0);
  --(*inodes->free_inode_num_ptr_);

  uint64_t id = findCleanBit(&inodes->bit_set_);
  setBit(&inodes->bit_set_, id);

  clearInode(inodes, getInodeById(inodes, id));

  return id;
}

void deleteInode(struct Inodes* inodes, uint64_t inode_id) {
  assert(getBit(&inodes->bit_set_, inode_id));

  {
    Inode* inode = getInodeById(inodes, inode_id);
    if (inode->is_dir) {
      // delete all files and subdirectories
      for (uint64_t i = 0; i * sizeof(Link) < inode->file_size; ++i) {
        Link link;
        Inode_read(inodes, inode, &link, i * sizeof(Link), sizeof(Link));
        if (link.is_alive) {
          deleteInode(inodes, link.inode_id);
        }
      }
    }
  }

  Inode* inode = getInodeById(inodes, inode_id);
  for (uint64_t i = 0; i < inode->blocks_count; ++i) {
    id_t block_id = getBlockIdByIndex(&inode->inodes_list, inodes->blocks_, i);
    if (deleteBlock(inodes->blocks_, block_id) < 0) {
      abort();
    }
  }

  clearBit(&inodes->bit_set_, inode_id);

  ++(*inodes->free_inode_num_ptr_);
}

int addDirectoryEntry(struct Inodes* inodes, Inode* inode_ptr, const char* name, bool is_dir) {
  if (strlen(name) > MAX_LINK_NAME_LEN) {
    return -1;
  }

  uint64_t new_inode_id = createInode(inodes);
  getInodeById(inodes, new_inode_id)->is_dir = is_dir;

  Link new_link = {.is_alive = true, .inode_id = new_inode_id};
  strcpy(new_link.name, name);

  for (uint64_t i = 0; i * sizeof(Link) < inode_ptr->file_size; ++i) {
    Link link;
    Inode_read(inodes, inode_ptr, &link, i * sizeof(Link), sizeof(Link));
    if (!link.is_alive) {
      if (Inode_write(inodes, inode_ptr, &new_link, i * sizeof(Link), sizeof(Link)) < 0) {
        return -1;
      }

      return 0;
    }
  }

  if (Inode_append(inodes, inode_ptr, &new_link, sizeof(Link)) < 0) {
    return -1;
  }

  return 0;
}

int clearInode(struct Inodes* inodes, Inode* inode_ptr) {
  FSC_UNUSED(inodes);
  inode_ptr->inodes_list.size_ = 0;
  inode_ptr->file_size = 0;
  inode_ptr->blocks_count = 0;

  return 0;
}

Block* getBlockByIndex(struct Inodes* inodes, Inode* inode, uint64_t index) {
  id_t block_id = getBlockIdByIndex(&inode->inodes_list, inodes->blocks_, index);
  return getBlockById(inodes->blocks_, block_id);
}

int extend(struct Inodes* inodes, Inode* inode, uint64_t new_size) {
  uint64_t exact_block_count = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  if (exact_block_count - inode->blocks_count > getFreeBlockNum(inodes->blocks_) ||
      exact_block_count > maxIListSize(&inode->inodes_list)) {
    return -1;
  }

  while (inode->blocks_count != exact_block_count) {
    id_t new_block_id;

    if (createBlock(inodes->blocks_, &new_block_id) < 0 || addBlockToInode(inodes, inode, new_block_id) < 0) {
      return -1;
    }
  }

  inode->file_size = new_size;

  return 0;
}
