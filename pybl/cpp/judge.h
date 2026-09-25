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
}
