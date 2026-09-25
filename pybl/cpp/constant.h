#pragma once
#include <cstdint>

namespace battleline
{
    constexpr int32_t NUM_COLORS = 6;
    constexpr int32_t NUM_RANKS = 10;
    constexpr int32_t NUM_CARDS = NUM_COLORS * NUM_RANKS;
    constexpr int32_t NUM_FLAGS = 9;
    constexpr int32_t FORMATION_SIZE = 3;
    constexpr int32_t HAND_SIZE = 7;
    constexpr int32_t NUM_PLAYERS = 2;

    // 部隊カードの色
    constexpr int8_t RED = 0;
    constexpr int8_t YELLOW = 1;
    constexpr int8_t BLUE = 2;
    constexpr int8_t GREEN = 3;
    constexpr int8_t PURPLE = 4;
    constexpr int8_t ORANGE = 5;
    constexpr int8_t NULL_COLOR = 6;

    // カードは color * NUM_RANKS + (rank - 1) で 0 - 59 に割り当てる
    constexpr int8_t NULL_CARD = static_cast<int8_t>(NUM_CARDS);

    // プレイヤー
    constexpr int8_t FIRST = 0;
    constexpr int8_t SECOND = 1;
    constexpr int8_t NULL_PLAYER = 2;

    // 対局結果
    constexpr int8_t FIRST_WIN = 0;
    constexpr int8_t SECOND_WIN = 1;
    constexpr int8_t DRAW = 2;
    constexpr int8_t NOT_OVER = 3;

    // フォーメーション (値が大きいほど強い)
    constexpr int8_t HOST = 0;
    constexpr int8_t SKIRMISHER = 1;
    constexpr int8_t BATTALION = 2;
    constexpr int8_t PHALANX = 3;
    constexpr int8_t WEDGE = 4;
    constexpr int8_t NULL_FORMATION = 5;

    // 着手は card * NUM_FLAGS + flag で 0 - 539 に割り当てる
    constexpr int16_t NUM_PLACE_MOVES = static_cast<int16_t>(NUM_CARDS * NUM_FLAGS);
    constexpr int16_t PASS_MOVE = NUM_PLACE_MOVES;
    constexpr int16_t NULL_MOVE = NUM_PLACE_MOVES + 1;
    constexpr int32_t MAX_LEGAL_MOVES = HAND_SIZE * NUM_FLAGS;

    // 1対局の最大手数 (配置は最大54手, パスは配置の間に高々1回ずつ)
    constexpr int32_t MAX_GAME_LENGTH = 128;

    constexpr uint64_t ALL_CARDS = (1ULL << NUM_CARDS) - 1;
    constexpr uint16_t ALL_FLAGS = static_cast<uint16_t>((1u << NUM_FLAGS) - 1);

    constexpr int8_t to_opponent(int8_t player) { return player ^ 1; }
}
