#pragma once
#include <cstdint>
#include <utility>

#include "constant.h"
#include "card.h"
#include "utils/random.h"

namespace battleline
{
    // 部隊カードの山札. 60枚全てをシャッフルして保持し, top より後ろが未使用のカード
    class Deck
    {
    public:
        void reset(Random& rng)
        {
            for (auto i = 0; i < NUM_CARDS; i++)
                this->cards[i] = static_cast<int8_t>(i);

            for (auto i = NUM_CARDS - 1; i > 0; i--)
                std::swap(this->cards[i], this->cards[rng.next_int(i + 1)]);

            this->top = 0;
        }

        // 山札を cards (先頭から引かれる順) で置き換える
        void set(const int8_t* cards, int32_t n)
        {
            this->top = NUM_CARDS - n;
            set_remaining(cards, n);
        }

        bool empty() const { return this->top == NUM_CARDS; }

        int32_t count() const { return NUM_CARDS - this->top; }

        // 山札が空なら NULL_CARD を返す
        int8_t draw() { return empty() ? NULL_CARD : this->cards[this->top++]; }

        void undo_draw() { this->top--; }

        // 未使用のカードを cards で置き換える. n は count() と等しくなければならない
        void set_remaining(const int8_t* cards, int32_t n)
        {
            for (auto i = 0; i < n; i++)
                this->cards[this->top + i] = cards[i];
        }

        CardSet remaining() const
        {
            CardSet set = 0ULL;
            for (auto i = this->top; i < NUM_CARDS; i++)
                set |= card_bit(this->cards[i]);
            return set;
        }

    private:
        int8_t cards[NUM_CARDS];
        int32_t top;
    };
}
