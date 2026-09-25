#pragma once
#include <bit>
#include <cstdint>

#include "../config.h"

namespace battleline
{
    FORCE_INLINE int32_t popcount(uint64_t bits) { return std::popcount(bits); }

    FORCE_INLINE int32_t find_first_set(uint64_t bits) { return std::countr_zero(bits); }

    FORCE_INLINE uint64_t clear_lowest_bit(uint64_t bits) { return bits & (bits - 1); }
}
