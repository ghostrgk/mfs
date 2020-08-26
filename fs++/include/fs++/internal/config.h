#pragma once

#include <cstdint>

// should be multiple of 8
const uint64_t DEFAULT_BLOCK_COUNT = 32 * 1;
const uint64_t DEFAULT_INODE_COUNT = 32 * 1;
const uint64_t BLOCK_SIZE = 16;
const uint64_t INODE_MAX_BLOCK_COUNT = 6;
const uint64_t MAX_LINK_NAME_LEN = 63;

static_assert(sizeof(char) == sizeof(uint8_t), "char should be 1 byte");
static_assert(DEFAULT_BLOCK_COUNT % 8 == 0);
static_assert(DEFAULT_INODE_COUNT % 8 == 0);
static_assert(DEFAULT_BLOCK_COUNT % 32 == 0);
// static_assert(DEFAULT_INODE_COUNT % 32 == 0);