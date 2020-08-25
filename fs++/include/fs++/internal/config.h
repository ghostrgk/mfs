#pragma once

#include <cstdint>


// should be multiple of 8
const uint64_t DEFAULT_BLOCK_COUNT = 8 * 128;
const uint64_t DEFAULT_INODE_COUNT = 8 * 32;
const uint64_t BLOCK_SIZE = 4096;

static_assert(sizeof(char) == sizeof(uint8_t), "char should be 1 byte");
static_assert(DEFAULT_BLOCK_COUNT % 8 == 0);
static_assert(DEFAULT_INODE_COUNT % 8 == 0);
