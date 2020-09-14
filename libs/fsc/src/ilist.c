#include <fsc/internal/ilist.h>

#ifdef NORMAL_ILIST

static id_t* resolveIndirection(struct Blocks* blocks, uint64_t block_id, uint64_t index) {
  Block* bytes = (Block*)getBlockById(blocks, block_id)->bytes;
  return &(((uint64_t*)bytes)[index]);
}

id_t getBlockIdByIndex(struct InodesList* ilist, struct Blocks* blocks, uint64_t index) {
  assert(index < ilist->size_);
  if (index < ILIST_ZERO_INDIRECTION) {
    return ilist->block_ids_[index];
  }

  index -= ILIST_ZERO_INDIRECTION;

  if (index < IDS_IN_BLOCK_COUNT) {
    return *resolveIndirection(blocks, ilist->level1_id_, index);
  }

  index -= IDS_IN_BLOCK_COUNT;

  assert(index < IDS_IN_BLOCK_COUNT * IDS_IN_BLOCK_COUNT);

  id_t resolved_level1_id = *resolveIndirection(blocks, ilist->level2_id_, index / IDS_IN_BLOCK_COUNT);
  index %= IDS_IN_BLOCK_COUNT;

  return *resolveIndirection(blocks, resolved_level1_id, index);
}

int addBlock(struct InodesList* ilist, struct Blocks* blocks, id_t block_id) {
  if (ilist->size_ + 1 == INODE_MAX_BLOCK_COUNT) {
    return -1;
  }

  uint64_t index = ilist->size_;
  ++ilist->size_;

  if (index < ILIST_ZERO_INDIRECTION) {
    ilist->block_ids_[index] = block_id;
    return 0;
  }

  index -= ILIST_ZERO_INDIRECTION;

  if (index == 0) {
    if (createBlock(blocks, &ilist->level1_id_) < 0) {
      return -1;
    }
  }

  if (index < IDS_IN_BLOCK_COUNT) {
    *resolveIndirection(blocks, ilist->level1_id_, index) = block_id;
    return 0;
  }

  index -= IDS_IN_BLOCK_COUNT;

  assert(index < IDS_IN_BLOCK_COUNT * IDS_IN_BLOCK_COUNT);

  if (index == 0) {
    if (createBlock(blocks, &ilist->level2_id_) < 0) {
      return -1;
    }
  }

  id_t* resolved_level1_id = resolveIndirection(blocks, ilist->level2_id_, index / IDS_IN_BLOCK_COUNT);
  if (index % IDS_IN_BLOCK_COUNT == 0) {
    if (createBlock(blocks, resolved_level1_id) < 0) {
      return -1;
    }
  }

  index %= IDS_IN_BLOCK_COUNT;

  *resolveIndirection(blocks, *resolved_level1_id, index) = block_id;
  return 0;
}

uint64_t maxIListSize(struct InodesList* ilist) {
  FSC_UNUSED(ilist);
  return INODE_MAX_BLOCK_COUNT;
}

#endif
