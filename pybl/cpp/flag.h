#pragma once
#include <cstdint>

#include "config.h"
#include "constant.h"
#include "card.h"
#include "formation.h"

namespace battleline
{
    // 1つのフラッグとその両側に配置された部隊カード
    struct Flag
    {
        int8_t cards[NUM_PLAYERS][FORMATION_SIZE];
        int8_t count[NUM_PLAYERS];
        int8_t owner;            // フラッグを確保したプレイヤー. 未確保なら NULL_PLAYER
        int8_t first_completer;  // 先に3枚目を置いてフォーメーションを完成させたプレイヤー

        void clear()
        {
            for (auto p = 0; p < NUM_PLAYERS; p++)
            {
                for (auto i = 0; i < FORMATION_SIZE; i++)
                    this->cards[p][i] = NULL_CARD;
                this->count[p] = 0;
            }
            this->owner = NULL_PLAYER;
            this->first_completer = NULL_PLAYER;
        }

        FORCE_INLINE bool is_claimed() const { return this->owner != NULL_PLAYER; }

        FORCE_INLINE bool is_full(int8_t player) const { return this->count[player] == FORMATION_SIZE; }

        FORCE_INLINE bool can_place(int8_t player) const { return !is_claimed() && !is_full(player); }

        FORCE_INLINE void place(int8_t player, int8_t card)
        {
            this->cards[player][this->count[player]++] = card;
            if (is_full(player) && this->first_completer == NULL_PLAYER)
                this->first_completer = player;
        }

        CardSet card_set(int8_t player) const
        {
            CardSet set = 0ULL;
            for (auto i = 0; i < this->count[player]; i++)
                set |= card_bit(this->cards[player][i]);
            return set;
        }

        // フォーメーションが未完成なら NULL_STRENGTH を返す
        Strength strength(int8_t player) const
        {
            if (!is_full(player))
                return NULL_STRENGTH;
            auto c = this->cards[player];
            return evaluate_formation(c[0], c[1], c[2]);
        }
    };
}
