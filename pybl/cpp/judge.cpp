#include "judge.h"

#include "formation.h"
#include "utils/bitmanip.h"

namespace battleline
{
    int8_t judge_flag(const Flag& flag, CardSet board, int8_t turn_player)
    {
        auto full0 = flag.is_full(FIRST);
        auto full1 = flag.is_full(SECOND);

        // (1) 両側の比較. 引き分けなら先に3枚目を置いたプレイヤーが確保する
        if (full0 && full1)
        {
            auto s0 = flag.strength(FIRST);
            auto s1 = flag.strength(SECOND);
            if (s0 > s1)
                return FIRST;
            if (s1 > s0)
                return SECOND;
            return flag.first_completer;
        }

        if (!full0 && !full1)
            return NULL_PLAYER;

        // 以降は一方のみフォーメーションが完成している場合.
        // 証明には盤面情報のみを用いるため, 盤面に出ていない全てのカードを未完成側が使える可能性がある
        auto complete = full0 ? FIRST : SECOND;
        auto incomplete = to_opponent(complete);
        auto strength = flag.strength(complete);
        board |= flag.card_set(FIRST) | flag.card_set(SECOND);
        auto available = ALL_CARDS & ~board;

        // (3) が成立するには, どのカードを選んでも合計が等しくなる必要があり, 盤面に出ていないカードは
        // 全て同じ数字 (高々 NUM_COLORS 枚) でなければならない. よって (2) の対象外でこれを超える場合は判定を省略する
        if (complete != turn_player && popcount(available) > NUM_COLORS)
            return NULL_PLAYER;

        auto best = best_completion(flag.cards[incomplete], flag.count[incomplete], available);

        // (2) 証明による確保 (手番プレイヤーのみ).
        //     同じ強さになっても先に完成させた側が確保するため, 相手の最強が自分以下なら確保できる.
        //     相手がフォーメーションを完成できない (best == NULL_STRENGTH) 場合も確保できる
        if (complete == turn_player && best <= strength)
            return complete;

        // (3) 引き分け確定. どう配置しても同じ強さにしかならない場合, 先に完成させた側が確保する
        if (best == strength && worst_completion(flag.cards[incomplete], flag.count[incomplete], available) == strength)
            return flag.first_completer;

        return NULL_PLAYER;
    }
}
