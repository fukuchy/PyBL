#pragma once
#include <cstdint>

#include "config.h"
#include "constant.h"
#include "card.h"
#include "deck.h"
#include "flag.h"
#include "formation.h"
#include "move.h"
#include "utils/bitmanip.h"
#include "utils/random.h"

namespace battleline
{
    // 対局全体の状態 (両者の手札と山札を含む完全情報)
    class GameState
    {
    public:
        GameState() { reset(0); }
        explicit GameState(uint64_t seed) { reset(seed); }

        // 山札をシャッフルして7枚ずつ配り, 先手をランダムに決める
        void reset(uint64_t seed);

        // 任意の局面を設定する. hands は2要素, flags は NUM_FLAGS 要素, deck_cards は先頭から引かれる順.
        // 全てのカードが手札, フラッグ, 山札のいずれかにちょうど1回ずつ現れなければならず,
        // 不正な局面の場合は std::invalid_argument を送出する. undo の履歴は破棄される
        void set_position(const CardSet* hands, const Flag* flags, const int8_t* deck_cards, int32_t deck_count,
                          int8_t side_to_move, int8_t first_player);

        int8_t side_to_move() const { return this->stm; }
        int8_t first_player() const { return this->first; }
        int8_t result() const { return this->game_result; }
        bool is_terminal() const { return this->game_result != NOT_OVER; }

        // 勝者. 引き分けまたは未終局なら NULL_PLAYER
        int8_t winner() const
        {
            if (this->game_result == FIRST_WIN)
                return FIRST;
            if (this->game_result == SECOND_WIN)
                return SECOND;
            return NULL_PLAYER;
        }

        // 両者の連続パスにより強制終局したかどうか
        bool is_forced_termination() const { return this->forced_termination; }
        int32_t consecutive_pass_count() const { return this->consecutive_passes; }
        int32_t move_count() const { return this->history_size; }

        CardSet hand(int8_t player) const { return this->hands[player]; }
        int32_t hand_count(int8_t player) const { return popcount(this->hands[player]); }
        int32_t deck_count() const { return this->deck.count(); }
        CardSet board_cards() const { return this->board; }

        // player から見えないカード (相手の手札 + 山札)
        CardSet unseen_cards(int8_t player) const { return ALL_CARDS & ~this->board & ~this->hands[player]; }

        uint16_t claimed_flags(int8_t player) const { return this->claimed[player]; }

        int8_t flag_owner(int32_t flag) const { return this->flags[flag].owner; }
        int8_t flag_first_completer(int32_t flag) const { return this->flags[flag].first_completer; }
        int32_t flag_card_count(int32_t flag, int8_t player) const { return this->flags[flag].count[player]; }
        int8_t flag_card(int32_t flag, int8_t player, int32_t slot) const { return this->flags[flag].cards[player][slot]; }
        Strength flag_strength(int32_t flag, int8_t player) const { return this->flags[flag].strength(player); }

        // player がカードを配置可能なフラッグ (ビットマスク)
        uint16_t placeable_flags(int8_t player) const;

        // 合法手を out に書き込み, その数を返す. out には MAX_LEGAL_MOVES 以上の領域が必要.
        // 配置可能な手が無ければ PASS_MOVE のみを返す. 終局後は0を返す
        int32_t get_legal_moves(Move* out) const;

        bool is_legal(Move move) const;

        // 配置 -> フラッグの判定 -> 勝利条件の確認 -> 補充 の順に1手進め, この手で確保されたフラッグを返す.
        // 非合法手の場合は std::invalid_argument を送出する
        uint16_t play(Move move);

        // 直前の play を取り消す. 取り消せる手が無い場合は std::logic_error を送出する
        void undo();

        // player から見えないカード (相手の手札と山札) をランダムに再配分する.
        // 過去の補充の記録と整合しなくなるため, undo の履歴は破棄される
        void determinize(int8_t player, uint64_t seed);

        // 終局までランダムに着手し, 対局結果を返す (この状態自体を終局まで進める)
        int8_t random_playout(uint64_t seed);

    private:
        Flag flags[NUM_FLAGS];
        CardSet hands[NUM_PLAYERS];
        CardSet board;
        Deck deck;
        uint16_t claimed[NUM_PLAYERS];
        int8_t stm;
        int8_t first;
        int8_t game_result;
        int8_t consecutive_passes;
        bool forced_termination;
        MoveRecord history[MAX_GAME_LENGTH];
        int32_t history_size;

        uint16_t judge_flags(int8_t turn_player);
        void check_victory();
        void force_terminate();
    };
}
