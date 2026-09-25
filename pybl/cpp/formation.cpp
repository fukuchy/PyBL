#include "formation.h"

#include <algorithm>
#include <climits>

#include "utils/bitmanip.h"

namespace battleline
{
    namespace
    {
        constexpr uint32_t WINDOW = 0b111u;                 // 連続する3つの数字のマスク
        constexpr int32_t NUM_WINDOWS = NUM_RANKS - 2;      // 連番の始点 (0始まり) の数

        // 連番の始点 low (0始まり) の3枚の合計
        constexpr int32_t window_sum(int32_t low) { return 3 * low + 6; }

        // 部分的に配置されたカードの情報
        struct Partial
        {
            uint32_t rank_mask = 0;
            int32_t sum = 0;
            int32_t color = -1;
            int32_t rank = -1;
            bool same_color = true;
            bool same_rank = true;
            bool distinct = true;

            Partial(const int8_t* cards, int32_t count)
            {
                for (auto i = 0; i < count; i++)
                {
                    auto c = card_color(cards[i]);
                    auto r = card_rank(cards[i]);
                    if (i == 0)
                    {
                        this->color = c;
                        this->rank = r;
                    }
                    this->same_color &= c == this->color;
                    this->same_rank &= r == this->rank;
                    this->distinct &= !(this->rank_mask & (1u << (r - 1)));
                    this->rank_mask |= 1u << (r - 1);
                    this->sum += r;
                }
            }

            // 配置済みのカードの数字が全て始点 low の連番に含まれるか
            bool fits_window(int32_t low) const { return (this->rank_mask & ~(WINDOW << low)) == 0; }
        };

        // 始点 low の連番を, available_ranks に含まれる数字で完成できるか
        bool can_complete_window(const Partial& p, int32_t low, uint32_t available_ranks)
        {
            auto window = WINDOW << low;
            return p.fits_window(low) && ((window & ~p.rank_mask) & ~available_ranks) == 0;
        }

        // ranks に含まれる数字のうち大きいものから n 個の合計
        int32_t top_ranks_sum(uint32_t ranks, int32_t n)
        {
            auto sum = 0;
            for (auto r = NUM_RANKS; r >= 1 && n > 0; r--)
                if (ranks & (1u << (r - 1)))
                {
                    sum += r;
                    n--;
                }
            return sum;
        }

        Strength best_wedge(const Partial& p, CardSet available)
        {
            if (!p.same_color || !p.distinct)
                return NULL_STRENGTH;

            auto best = NULL_STRENGTH;
            auto color_begin = p.color >= 0 ? p.color : 0;
            auto color_end = p.color >= 0 ? p.color + 1 : NUM_COLORS;
            for (auto c = color_begin; c < color_end; c++)
            {
                auto ranks = color_rank_mask(available, c);
                for (auto low = NUM_WINDOWS - 1; low >= 0; low--)
                    if (can_complete_window(p, low, ranks))
                    {
                        best = std::max(best, make_strength(WEDGE, window_sum(low)));
                        break;
                    }
            }
            return best;
        }

        Strength best_phalanx(const Partial& p, int32_t need, CardSet available)
        {
            if (!p.same_rank)
                return NULL_STRENGTH;

            auto rank_begin = p.rank >= 0 ? p.rank : NUM_RANKS;
            auto rank_end = p.rank >= 0 ? p.rank : 1;
            for (auto r = rank_begin; r >= rank_end; r--)
            {
                auto n = 0;
                for (auto c = 0; c < NUM_COLORS; c++)
                    n += has_card(available, make_card(c, r));
                if (n >= need)
                    return make_strength(PHALANX, 3 * r);
            }
            return NULL_STRENGTH;
        }

        Strength best_battalion(const Partial& p, int32_t need, CardSet available)
        {
            if (!p.same_color)
                return NULL_STRENGTH;

            auto best = NULL_STRENGTH;
            auto color_begin = p.color >= 0 ? p.color : 0;
            auto color_end = p.color >= 0 ? p.color + 1 : NUM_COLORS;
            for (auto c = color_begin; c < color_end; c++)
            {
                auto ranks = color_rank_mask(available, c);
                if (popcount(ranks) >= need)
                    best = std::max(best, make_strength(BATTALION, p.sum + top_ranks_sum(ranks, need)));
            }
            return best;
        }

        Strength best_skirmisher(const Partial& p, CardSet available)
        {
            if (!p.distinct)
                return NULL_STRENGTH;

            // 数字が異なるカードは必ず別のカードなので, 色を問わない数字のマスクで判定できる
            auto ranks = any_color_rank_mask(available);
            for (auto low = NUM_WINDOWS - 1; low >= 0; low--)
                if (can_complete_window(p, low, ranks))
                    return make_strength(SKIRMISHER, window_sum(low));
            return NULL_STRENGTH;
        }

        Strength best_host(const Partial& p, int32_t need, CardSet available)
        {
            auto sum = p.sum;
            for (auto r = NUM_RANKS; r >= 1 && need > 0; r--)
            {
                for (auto c = 0; c < NUM_COLORS && need > 0; c++)
                    if (has_card(available, make_card(c, r)))
                    {
                        sum += r;
                        need--;
                    }
            }
            return make_strength(HOST, sum);
        }

        // candidates (数字の昇順) から残りのカードを選び, 最弱のフォーメーションを探索する.
        // どのフォーメーションの強さも make_strength(HOST, 合計) 以上であることを用いて枝刈りする
        void search_worst(const int8_t* candidates, int32_t num_candidates, int32_t start,
                          int8_t* cards, int32_t filled, int32_t sum, Strength& worst)
        {
            if (filled == FORMATION_SIZE)
            {
                worst = std::min(worst, evaluate_formation(cards[0], cards[1], cards[2]));
                return;
            }

            auto remaining = FORMATION_SIZE - filled;
            for (auto i = start; i <= num_candidates - remaining; i++)
            {
                auto bound = sum;
                for (auto j = 0; j < remaining; j++)
                    bound += card_rank(candidates[i + j]);
                if (make_strength(HOST, bound) >= worst)
                    break;

                cards[filled] = candidates[i];
                search_worst(candidates, num_candidates, i + 1, cards, filled + 1, sum + card_rank(candidates[i]), worst);
            }
        }

        CardSet partial_set(const int8_t* partial, int32_t count)
        {
            CardSet set = 0ULL;
            for (auto i = 0; i < count; i++)
                set |= card_bit(partial[i]);
            return set;
        }
    }

    int32_t formation_type(int8_t a, int8_t b, int8_t c)
    {
        auto ra = card_rank(a), rb = card_rank(b), rc = card_rank(c);
        auto same_color = card_color(a) == card_color(b) && card_color(b) == card_color(c);
        auto same_rank = ra == rb && rb == rc;
        auto distinct = ra != rb && rb != rc && ra != rc;
        // 数字の循環 (9, 10, 1 など) は連番としない
        auto consecutive = distinct && std::max({ra, rb, rc}) - std::min({ra, rb, rc}) == 2;

        if (same_color && consecutive)
            return WEDGE;
        if (same_rank)
            return PHALANX;
        if (same_color)
            return BATTALION;
        if (consecutive)
            return SKIRMISHER;
        return HOST;
    }

    Strength evaluate_formation(int8_t a, int8_t b, int8_t c)
    {
        return make_strength(formation_type(a, b, c), card_rank(a) + card_rank(b) + card_rank(c));
    }

    Strength best_completion(const int8_t* partial, int32_t count, CardSet available)
    {
        if (count >= FORMATION_SIZE)
            return evaluate_formation(partial[0], partial[1], partial[2]);

        available &= ALL_CARDS & ~partial_set(partial, count);
        auto need = FORMATION_SIZE - count;
        if (popcount(available) < need)
            return NULL_STRENGTH;

        // 強いフォーメーションから順に成立可否を調べる.
        // 各段階では, それより強いフォーメーションが成立しないことが確定しているため,
        // 選んだカードが意図せず強いフォーメーションになることはない
        Partial p(partial, count);
        Strength s;
        if ((s = best_wedge(p, available)) != NULL_STRENGTH)
            return s;
        if ((s = best_phalanx(p, need, available)) != NULL_STRENGTH)
            return s;
        if ((s = best_battalion(p, need, available)) != NULL_STRENGTH)
            return s;
        if ((s = best_skirmisher(p, available)) != NULL_STRENGTH)
            return s;
        return best_host(p, need, available);
    }

    Strength worst_completion(const int8_t* partial, int32_t count, CardSet available)
    {
        if (count >= FORMATION_SIZE)
            return evaluate_formation(partial[0], partial[1], partial[2]);

        available &= ALL_CARDS & ~partial_set(partial, count);
        if (popcount(available) < FORMATION_SIZE - count)
            return NULL_STRENGTH;

        int8_t candidates[NUM_CARDS];
        auto num_candidates = 0;
        for (auto r = 1; r <= NUM_RANKS; r++)
            for (auto c = 0; c < NUM_COLORS; c++)
                if (has_card(available, make_card(c, r)))
                    candidates[num_candidates++] = make_card(c, r);

        int8_t cards[FORMATION_SIZE];
        auto sum = 0;
        for (auto i = 0; i < count; i++)
        {
            cards[i] = partial[i];
            sum += card_rank(partial[i]);
        }

        Strength worst = INT_MAX;
        search_worst(candidates, num_candidates, 0, cards, count, sum, worst);
        return worst;
    }
}
