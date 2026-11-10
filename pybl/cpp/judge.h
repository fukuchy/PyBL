#pragma once
#include <cstdint>

#include "constant.h"
#include "card.h"
#include "flag.h"

namespace battleline
{
    // (1) 両側の比較, (2) 証明による確保 (turn_player のみ), (3) 引き分け確定 の順に判定し,
    // フラッグを確保するプレイヤーを返す. 確保されない場合は NULL_PLAYER を返す.
    // board には盤面に出ている全てのカードを与える (flag のカードは含まれていなくてもよい)
    int8_t judge_flag(const Flag& flag, CardSet board, int8_t turn_player);

    // 上と同じ判定を行い, 確保される場合は stage に判定の番号 (1, 2, 3) を入れる. 確保されない場合は 0
    int8_t judge_flag(const Flag& flag, CardSet board, int8_t turn_player, int8_t& stage);
}
