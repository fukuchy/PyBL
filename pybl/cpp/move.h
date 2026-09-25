#pragma once
#include <cstdint>

#include "config.h"
#include "constant.h"

namespace battleline
{
    using Move = int16_t;

    FORCE_INLINE constexpr Move make_move(int8_t card, int32_t flag)
    {
        return static_cast<Move>(card * NUM_FLAGS + flag);
    }

    FORCE_INLINE constexpr int8_t move_card(Move move) { return static_cast<int8_t>(move / NUM_FLAGS); }

    FORCE_INLINE constexpr int32_t move_flag(Move move) { return move % NUM_FLAGS; }

    // undo のために1手ごとに記録する情報
    struct MoveRecord
    {
        Move move;
        int8_t drawn_card;               // 補充したカード. 補充しなかった場合は NULL_CARD
        uint16_t claimed_flags;          // この手で確保されたフラッグ (ビットマスク)
        int8_t prev_result;
        int8_t prev_consecutive_passes;
        bool prev_forced_termination;
    };
}
