#pragma once
#include <cstdint>

namespace battleline
{
    // xorshift64* による軽量な擬似乱数生成器
    class Random
    {
    public:
        explicit Random(uint64_t seed = 0) { set_seed(seed); }

        void set_seed(uint64_t seed)
        {
            this->state = splitmix64(seed);
            if (this->state == 0)
                this->state = 0x9E3779B97F4A7C15ULL;
        }

        uint64_t next()
        {
            this->state ^= this->state >> 12;
            this->state ^= this->state << 25;
            this->state ^= this->state >> 27;
            return this->state * 0x2545F4914F6CDD1DULL;
        }

        // [0, n) の一様乱数
        int32_t next_int(int32_t n)
        {
            return static_cast<int32_t>(((next() >> 32) * static_cast<uint64_t>(n)) >> 32);
        }

    private:
        uint64_t state;

        static uint64_t splitmix64(uint64_t x)
        {
            x += 0x9E3779B97F4A7C15ULL;
            x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
            x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
            return x ^ (x >> 31);
        }
    };
}
