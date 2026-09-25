#pragma once
#include <cstdint>

#include "card.h"
#include "utils/bitmanip.h"

namespace battleline
{
    // CardSetに含まれるカードを番号の小さい順に列挙する
    class CardIterator
    {
    public:
        CardIterator() : bits(0ULL) { }
        CardIterator(const CardSet bits) : bits(bits) { }

        bool end() const { return !this->bits; }

        int8_t next()
        {
            auto card = static_cast<int8_t>(find_first_set(this->bits));
            this->bits = clear_lowest_bit(this->bits);
            return card;
        }

        int32_t size() const { return popcount(this->bits); }

    private:
        CardSet bits;
    };
}
