#pragma once
#include <cstdint>

#include "config.h"
#include "constant.h"

namespace battleline
{
    // カードの集合. card番目のビットが1ならそのカードを含む
    using CardSet = uint64_t;

    FORCE_INLINE constexpr int8_t make_card(int32_t color, int32_t rank)
    {
        return static_cast<int8_t>(color * NUM_RANKS + rank - 1);
    }

    FORCE_INLINE constexpr int32_t card_color(int8_t card) { return card / NUM_RANKS; }

    FORCE_INLINE constexpr int32_t card_rank(int8_t card) { return card % NUM_RANKS + 1; }

    FORCE_INLINE constexpr CardSet card_bit(int8_t card) { return 1ULL << card; }

    FORCE_INLINE constexpr bool has_card(CardSet set, int8_t card) { return (set >> card) & 1ULL; }

    // 指定した色のカードについて, (rank - 1)番目のビットが1である10bitマスクを返す
    FORCE_INLINE constexpr uint32_t color_rank_mask(CardSet set, int32_t color)
    {
        return static_cast<uint32_t>(set >> (color * NUM_RANKS)) & 0x3FFu;
    }

    // 色を問わず, (rank - 1)番目のビットが1である10bitマスクを返す
    FORCE_INLINE constexpr uint32_t any_color_rank_mask(CardSet set)
    {
        uint32_t mask = 0;
        for (int32_t color = 0; color < NUM_COLORS; color++)
            mask |= color_rank_mask(set, color);
        return mask;
    }
}
