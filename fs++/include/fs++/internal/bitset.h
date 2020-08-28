#pragma once

#include <cstdint>

namespace fspp::internal {

class BitSet {
 public:
  BitSet();
  explicit BitSet(uint64_t element_num);
  BitSet(uint64_t element_num, uint8_t* bytes);
  ~BitSet();

  BitSet(const BitSet& other) = delete;
  BitSet& operator=(const BitSet& other) = delete;
  BitSet(BitSet&& other) noexcept;
  BitSet& operator=(BitSet&& other) noexcept;

  void setBit(uint64_t index);
  void clearBit(uint64_t index);
  uint8_t getBit(uint64_t index);
  uint64_t findCleanBit();

 private:
  uint8_t* getByteByIndex(uint64_t index);
  static uint8_t bitmaskByBitIndex(uint8_t bit_index);

 private:
  bool has_ownership_{false};
  uint64_t element_num_{0};
  uint8_t* bytes_{nullptr};
};

}  // namespace fspp::internal
