#pragma once

#include <assert.h>
#include <stdint.h>

#include "block.h"
#include "compiler.h"

struct InodesList {
  uint64_t size_;
  id_t block_ids_[ILIST_ZERO_INDIRECTION];
  id_t level1_id_;
  id_t level2_id_;
};

id_t getBlockIdByIndex(struct InodesList* ilist, struct Blocks* blocks, uint64_t index);
uint64_t maxIListSize(struct InodesList* ilist);
int addBlock(struct InodesList* ilist, struct Blocks* blocks, id_t block_id);





