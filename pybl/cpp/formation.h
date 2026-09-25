#pragma once
#include <cstdint>

#include "config.h"
#include "constant.h"
#include "card.h"

namespace battleline
{
    // フォーメーションの強さ. (フォーメーションの種類 << 5) | カード3枚の合計 で表現し,
    // 値の大小比較がそのままフォーメーションの強弱比較になる (合計は最大27なので5bitに収まる)
    using Strength = int32_t;

    constexpr Strength NULL_STRENGTH = -1;
    constexpr int32_t STRENGTH_TYPE_SHIFT = 5;

    FORCE_INLINE constexpr Strength make_strength(int32_t formation, int32_t sum)
    {
        return (formation << STRENGTH_TYPE_SHIFT) | sum;
    }

    FORCE_INLINE constexpr int32_t strength_formation(Strength s) { return s >> STRENGTH_TYPE_SHIFT; }

    FORCE_INLINE constexpr int32_t strength_sum(Strength s) { return s & ((1 << STRENGTH_TYPE_SHIFT) - 1); }

    int32_t formation_type(int8_t a, int8_t b, int8_t c);

    Strength evaluate_formation(int8_t a, int8_t b, int8_t c);

    // partial (count枚, 0 - 2枚) に available のカードを加えて完成させたときの最強のフォーメーションの強さ.
    // 完成させられない場合は NULL_STRENGTH を返す
    Strength best_completion(const int8_t* partial, int32_t count, CardSet available);

    // best_completion の最弱版. 引き分け確定 (判定(3)) に用いる
    Strength worst_completion(const int8_t* partial, int32_t count, CardSet available);
}
