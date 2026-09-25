#include "game_state.h"

#include <stdexcept>
#include <utility>

#include "card_iterator.h"
#include "judge.h"

namespace battleline
{
    namespace
    {
        // 連続する3つのフラッグ, または5個以上のフラッグを確保していれば勝利条件を満たす
        bool satisfies_victory(uint16_t claimed)
        {
            return (claimed & (claimed >> 1) & (claimed >> 2)) != 0 || popcount(claimed) >= 5;
        }
    }

    void GameState::reset(uint64_t seed)
    {
        Random rng(seed);

        for (auto& flag : this->flags)
            flag.clear();

        this->deck.reset(rng);
        for (auto p = 0; p < NUM_PLAYERS; p++)
        {
            this->hands[p] = 0ULL;
            for (auto i = 0; i < HAND_SIZE; i++)
                this->hands[p] |= card_bit(this->deck.draw());
            this->claimed[p] = 0;
        }

        this->board = 0ULL;
        this->first = static_cast<int8_t>(rng.next_int(NUM_PLAYERS));
        this->stm = this->first;
        this->game_result = NOT_OVER;
        this->consecutive_passes = 0;
        this->forced_termination = false;
        this->history_size = 0;
    }

    void GameState::set_position(const CardSet* hands, const Flag* flags, const int8_t* deck_cards, int32_t deck_count,
                                 int8_t side_to_move, int8_t first_player)
    {
        auto is_player = [](int8_t p) { return p == FIRST || p == SECOND; };
        if (!is_player(side_to_move) || !is_player(first_player))
            throw std::invalid_argument("invalid player");

        // 全てのカードが手札, フラッグ, 山札のいずれかにちょうど1回ずつ現れなければならない
        CardSet used = 0ULL;
        auto use = [&used](int8_t card)
        {
            if (card < 0 || card >= NUM_CARDS || has_card(used, card))
                throw std::invalid_argument("each card must appear exactly once");
            used |= card_bit(card);
        };

        for (auto p = 0; p < NUM_PLAYERS; p++)
        {
            if ((hands[p] & ~ALL_CARDS) || popcount(hands[p]) > HAND_SIZE)
                throw std::invalid_argument("invalid hand");
            for (CardIterator it(hands[p]); !it.end();)
                use(it.next());
        }

        CardSet board = 0ULL;
        for (auto i = 0; i < NUM_FLAGS; i++)
        {
            const auto& f = flags[i];
            for (auto p = 0; p < NUM_PLAYERS; p++)
            {
                if (f.count[p] < 0 || f.count[p] > FORMATION_SIZE)
                    throw std::invalid_argument("invalid number of cards on a flag");
                for (auto j = 0; j < f.count[p]; j++)
                    use(f.cards[p][j]);
            }
            board |= f.card_set(FIRST) | f.card_set(SECOND);

            if (f.owner != NULL_PLAYER && !is_player(f.owner))
                throw std::invalid_argument("invalid flag owner");

            auto full0 = f.is_full(FIRST), full1 = f.is_full(SECOND);
            auto valid_completer = (full0 && full1) ? is_player(f.first_completer)
                                 : full0 ? f.first_completer == FIRST
                                 : full1 ? f.first_completer == SECOND
                                 : f.first_completer == NULL_PLAYER;
            if (!valid_completer)
                throw std::invalid_argument("inconsistent first completer");
        }

        for (auto i = 0; i < deck_count; i++)
            use(deck_cards[i]);

        if (used != ALL_CARDS)
            throw std::invalid_argument("each card must appear exactly once");

        for (auto i = 0; i < NUM_FLAGS; i++)
            this->flags[i] = flags[i];

        this->claimed[FIRST] = this->claimed[SECOND] = 0;
        for (auto i = 0; i < NUM_FLAGS; i++)
            if (this->flags[i].is_claimed())
                this->claimed[this->flags[i].owner] |= static_cast<uint16_t>(1u << i);

        this->hands[FIRST] = hands[FIRST];
        this->hands[SECOND] = hands[SECOND];
        this->board = board;
        this->deck.set(deck_cards, deck_count);
        this->stm = side_to_move;
        this->first = first_player;
        this->game_result = NOT_OVER;
        this->consecutive_passes = 0;
        this->forced_termination = false;
        this->history_size = 0;
        check_victory();
    }

    uint16_t GameState::placeable_flags(int8_t player) const
    {
        uint16_t mask = 0;
        for (auto i = 0; i < NUM_FLAGS; i++)
            if (this->flags[i].can_place(player))
                mask |= static_cast<uint16_t>(1u << i);
        return mask;
    }

    int32_t GameState::get_legal_moves(Move* out) const
    {
        if (is_terminal())
            return 0;

        auto n = 0;
        auto placeable = placeable_flags(this->stm);
        if (placeable)
        {
            for (CardIterator it(this->hands[this->stm]); !it.end();)
            {
                auto card = it.next();
                for (auto i = 0; i < NUM_FLAGS; i++)
                    if (placeable & (1u << i))
                        out[n++] = make_move(card, i);
            }
        }

        if (n == 0)
            out[n++] = PASS_MOVE;
        return n;
    }

    bool GameState::is_legal(Move move) const
    {
        if (is_terminal())
            return false;

        // 配置可能な手がある限りパスはできない
        if (move == PASS_MOVE)
            return this->hands[this->stm] == 0ULL || placeable_flags(this->stm) == 0;

        if (move < 0 || move >= NUM_PLACE_MOVES)
            return false;

        return has_card(this->hands[this->stm], move_card(move)) && this->flags[move_flag(move)].can_place(this->stm);
    }

    uint16_t GameState::play(Move move)
    {
        if (!is_legal(move))
            throw std::invalid_argument("illegal move");

        auto player = this->stm;
        auto& record = this->history[this->history_size++];
        record.move = move;
        record.drawn_card = NULL_CARD;
        record.prev_result = this->game_result;
        record.prev_consecutive_passes = this->consecutive_passes;
        record.prev_forced_termination = this->forced_termination;

        if (move == PASS_MOVE)
        {
            this->consecutive_passes++;
        }
        else
        {
            this->consecutive_passes = 0;
            auto card = move_card(move);
            this->hands[player] &= ~card_bit(card);
            this->board |= card_bit(card);
            this->flags[move_flag(move)].place(player, card);
        }

        // 配置直後 (パスした場合はパスした時点) で, 補充より前に判定する
        auto claimed_now = judge_flags(player);
        record.claimed_flags = claimed_now;
        check_victory();

        // ルール外の保護: 両者が連続でパスした場合, 以降状態が変化しないため強制的に終局させる
        if (!is_terminal() && this->consecutive_passes >= NUM_PLAYERS)
            force_terminate();

        // パスした場合は補充しない
        if (move != PASS_MOVE && !is_terminal())
        {
            auto drawn = this->deck.draw();
            if (drawn != NULL_CARD)
                this->hands[player] |= card_bit(drawn);
            record.drawn_card = drawn;
        }

        this->stm = to_opponent(player);
        return claimed_now;
    }

    uint16_t GameState::judge_flags(int8_t turn_player)
    {
        uint16_t claimed_now = 0;
        for (auto i = 0; i < NUM_FLAGS; i++)
        {
            if (this->flags[i].is_claimed())
                continue;

            auto owner = judge_flag(this->flags[i], this->board, turn_player);
            if (owner == NULL_PLAYER)
                continue;

            this->flags[i].owner = owner;
            this->claimed[owner] |= static_cast<uint16_t>(1u << i);
            claimed_now |= static_cast<uint16_t>(1u << i);
        }
        return claimed_now;
    }

    void GameState::check_victory()
    {
        auto first_wins = satisfies_victory(this->claimed[FIRST]);
        auto second_wins = satisfies_victory(this->claimed[SECOND]);

        if (first_wins && second_wins)
            this->game_result = DRAW;
        else if (first_wins)
            this->game_result = FIRST_WIN;
        else if (second_wins)
            this->game_result = SECOND_WIN;
    }

    void GameState::force_terminate()
    {
        // 確保したフラッグの数が多い方を勝者とし, 同数なら引き分けとする
        auto c0 = popcount(this->claimed[FIRST]);
        auto c1 = popcount(this->claimed[SECOND]);

        if (c0 > c1)
            this->game_result = FIRST_WIN;
        else if (c1 > c0)
            this->game_result = SECOND_WIN;
        else
            this->game_result = DRAW;

        this->forced_termination = true;
    }

    void GameState::undo()
    {
        if (this->history_size == 0)
            throw std::logic_error("no move to undo");

        const auto& record = this->history[--this->history_size];
        auto player = to_opponent(this->stm);
        this->stm = player;

        if (record.drawn_card != NULL_CARD)
        {
            this->hands[player] &= ~card_bit(record.drawn_card);
            this->deck.undo_draw();
        }

        for (auto i = 0; i < NUM_FLAGS; i++)
        {
            if (!(record.claimed_flags & (1u << i)))
                continue;
            auto& f = this->flags[i];
            this->claimed[f.owner] &= static_cast<uint16_t>(~(1u << i));
            f.owner = NULL_PLAYER;
        }

        if (record.move != PASS_MOVE)
        {
            auto card = move_card(record.move);
            auto& f = this->flags[move_flag(record.move)];
            // この手で3枚目を置いた場合, 先に完成させたのはこの手による
            if (f.is_full(player) && f.first_completer == player)
                f.first_completer = NULL_PLAYER;
            f.cards[player][--f.count[player]] = NULL_CARD;
            this->hands[player] |= card_bit(card);
            this->board &= ~card_bit(card);
        }

        this->game_result = record.prev_result;
        this->consecutive_passes = record.prev_consecutive_passes;
        this->forced_termination = record.prev_forced_termination;
    }

    void GameState::determinize(int8_t player, uint64_t seed)
    {
        Random rng(seed);
        auto opponent = to_opponent(player);
        auto unseen = unseen_cards(player);

        int8_t cards[NUM_CARDS];
        auto n = 0;
        for (CardIterator it(unseen); !it.end();)
            cards[n++] = it.next();

        for (auto i = n - 1; i > 0; i--)
            std::swap(cards[i], cards[rng.next_int(i + 1)]);

        // 相手の手札の枚数と山札の枚数は変えずに再配分する
        auto hand_count = popcount(this->hands[opponent]);
        this->hands[opponent] = 0ULL;
        for (auto i = 0; i < hand_count; i++)
            this->hands[opponent] |= card_bit(cards[i]);
        this->deck.set_remaining(cards + hand_count, n - hand_count);

        this->history_size = 0;
    }

    int8_t GameState::random_playout(uint64_t seed)
    {
        Random rng(seed);
        Move moves[MAX_LEGAL_MOVES];
        while (!is_terminal())
        {
            auto n = get_legal_moves(moves);
            play(moves[rng.next_int(n)]);
        }
        return this->game_result;
    }
}
