#pragma once
#include <cstdint>

#include "constant.h"
#include "card.h"
#include "card_iterator.h"
#include "flag.h"
#include "move.h"

namespace battleline
{
    // player がカードを配置可能なフラッグ (ビットマスク)
    inline uint16_t placeable_flags(const Flag* flags, int8_t player)
    {
        uint16_t mask = 0;
        for (auto i = 0; i < NUM_FLAGS; i++)
            if (flags[i].can_place(player))
                mask |= static_cast<uint16_t>(1u << i);
        return mask;
    }

    // player が手札 hand から指せる手を out に書き込み, その数を返す. out には MAX_LEGAL_MOVES 以上の領域が必要.
    // 配置可能な手が無ければ PASS_MOVE のみを返す
    inline int32_t generate_moves(const Flag* flags, CardSet hand, int8_t player, Move* out)
    {
        auto n = 0;
        auto placeable = placeable_flags(flags, player);
        if (placeable)
        {
            for (CardIterator it(hand); !it.end();)
            {
                auto card = it.next();
                for (auto i = 0; i < NUM_FLAGS; i++)
                    if (placeable & (1u << i))
                        out[n++] = make_move(card, i);
            }
        }

        if (n == 0)
            out[n++] = PASS_MOVE;
        return n;
    }
}
