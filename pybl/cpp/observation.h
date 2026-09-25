#pragma once
#include <cstdint>

#include "constant.h"
#include "card.h"
#include "flag.h"
#include "formation.h"
#include "move.h"
#include "movegen.h"
#include "utils/bitmanip.h"

namespace battleline
{
    // あるプレイヤーから見える情報のみからなる局面.
    // 相手の手札と山札の中身は含まず, それらを合わせたカードの集合 (unseen) と各枚数のみを持つ
    struct Observation
    {
        int8_t player;               // 観測しているプレイヤー
        int8_t stm;
        int8_t first;
        int8_t game_result;
        bool forced_termination;
        int8_t consecutive_passes;
        CardSet hand;                // player の手札
        CardSet unseen;              // player から見えないカード (相手の手札 + 山札)
        CardSet board;
        int32_t opponent_hand_count;
        int32_t deck_count;
        uint16_t claimed[NUM_PLAYERS];
        Flag flags[NUM_FLAGS];

        bool operator==(const Observation&) const = default;

        bool is_terminal() const { return this->game_result != NOT_OVER; }
        int8_t winner() const { return result_to_winner(this->game_result); }

        // player の手番であり, かつ終局していないか
        bool is_my_turn() const { return this->stm == this->player && !is_terminal(); }

        int32_t hand_count(int8_t p) const { return p == this->player ? popcount(this->hand) : this->opponent_hand_count; }
        uint16_t claimed_flags(int8_t p) const { return this->claimed[p]; }

        int8_t flag_owner(int32_t flag) const { return this->flags[flag].owner; }
        int8_t flag_first_completer(int32_t flag) const { return this->flags[flag].first_completer; }
        int32_t flag_card_count(int32_t flag, int8_t p) const { return this->flags[flag].count[p]; }
        int8_t flag_card(int32_t flag, int8_t p, int32_t slot) const { return this->flags[flag].cards[p][slot]; }
        Strength flag_strength(int32_t flag, int8_t p) const { return this->flags[flag].strength(p); }

        uint16_t placeable_flags(int8_t p) const { return battleline::placeable_flags(this->flags, p); }

        // player の合法手を out に書き込み, その数を返す. player の手番でない場合や終局後は0を返す
        int32_t get_legal_moves(Move* out) const
        {
            if (!is_my_turn())
                return 0;
            return generate_moves(this->flags, this->hand, this->player, out);
        }
    };
}
