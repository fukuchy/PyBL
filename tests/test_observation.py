"""プレイヤーから見える情報のみを取り出す Observation"""
import random

import pytest

import pybl

PLAYERS = (pybl.FIRST, pybl.SECOND)


def random_states(num_games: int, seed: int = 0):
    """ランダム対局の途中局面 (終局も含む) を順に返す"""
    rng = random.Random(seed)
    for game in range(num_games):
        state = pybl.GameState(seed=seed * 1000 + game)
        yield state
        while not state.is_terminal:
            moves = state.get_legal_moves()
            state.play(moves[rng.randrange(len(moves))])
            yield state


def test_observation_matches_visible_state():
    for state in random_states(20):
        for p in PLAYERS:
            obs = state.observe(p)
            opp = pybl.to_opponent(p)
            assert obs.player == p
            assert obs.side_to_move == state.side_to_move
            assert obs.first_player == state.first_player
            assert obs.result == state.result
            assert obs.winner == state.winner
            assert obs.is_terminal == state.is_terminal
            assert obs.is_forced_termination == state.is_forced_termination
            assert obs.consecutive_pass_count == state.consecutive_pass_count
            assert obs.deck_count == state.deck_count
            assert obs.board_cards == state.board_cards
            assert obs.hand == obs.get_hand() == obs.get_hand(p) == state.get_hand(p)
            assert obs.unseen_cards == obs.get_unseen_cards() == state.get_unseen_cards(p)
            assert obs.opponent_hand_count == state.get_hand_count(opp)
            for q in PLAYERS:
                assert obs.get_hand_count(q) == state.get_hand_count(q)
                assert obs.get_claimed_flags(q) == state.get_claimed_flags(q)
                assert obs.get_placeable_flags(q) == state.get_placeable_flags(q)
            for i in range(pybl.NUM_FLAGS):
                assert obs.get_flag_owner(i) == state.get_flag_owner(i)
                assert obs.get_flag_first_completer(i) == state.get_flag_first_completer(i)
                for q in PLAYERS:
                    assert obs.get_flag_cards(i, q) == state.get_flag_cards(i, q)
                    assert obs.get_flag_strength(i, q) == state.get_flag_strength(i, q)


def test_default_observer_is_side_to_move():
    state = pybl.GameState(seed=0)
    assert state.observe().player == state.side_to_move


def test_legal_moves_only_on_my_turn():
    for state in random_states(20):
        for p in PLAYERS:
            obs = state.observe(p)
            if p == state.side_to_move and not state.is_terminal:
                assert obs.is_my_turn
                assert obs.get_legal_moves().tolist() == state.get_legal_moves().tolist()
                assert all(obs.is_legal(m) for m in state.get_legal_moves())
            else:
                assert not obs.is_my_turn
                assert len(obs.get_legal_moves()) == 0


def test_opponent_hand_is_hidden():
    """見えないカードの配分だけが異なる局面は, 同じ観測になる"""
    for state in random_states(10):
        for p in PLAYERS:
            obs = state.observe(p)
            for s in range(3):
                other = state.copy()
                other.determinize(p, seed=s)
                assert other.observe(p) == obs


def test_observation_differs_between_players():
    state = pybl.GameState(seed=0)
    assert state.observe(pybl.FIRST) != state.observe(pybl.SECOND)


def test_observation_changes_after_move():
    state = pybl.GameState(seed=0)
    before = state.observe(pybl.FIRST)
    state.play(state.get_legal_moves()[0])
    assert state.observe(pybl.FIRST) != before


def test_observation_is_snapshot():
    state = pybl.GameState(seed=0)
    obs = state.observe(pybl.FIRST)
    copied = state.copy().observe(pybl.FIRST)
    state.random_playout(seed=0)
    assert obs == copied
    assert not obs.is_terminal


def test_opponent_hand_cannot_be_read():
    state = pybl.GameState(seed=0)
    p = state.side_to_move
    obs = state.observe(p)
    with pytest.raises(ValueError):
        obs.get_hand(pybl.to_opponent(p))

    # 文字列表現にも相手の手札のカードは現れない
    tokens = set(str(obs).split())
    for card in pybl.CardIterator(state.get_hand(pybl.to_opponent(p))):
        assert pybl.card_to_str(card) not in tokens
    for card in pybl.CardIterator(state.get_hand(p)):
        assert pybl.card_to_str(card) in tokens
    assert f"x{pybl.HAND_SIZE}" in tokens


def test_cannot_create_directly():
    with pytest.raises(TypeError):
        pybl.Observation()


def test_sample_state_is_consistent_with_observation():
    for state in random_states(10, seed=1):
        for p in PLAYERS:
            obs = state.observe(p)
            opp = pybl.to_opponent(p)
            opponent_hands = set()
            for s in range(5):
                sample = obs.sample_state(seed=s)
                assert sample.observe(p) == obs
                assert sample.move_count == 0
                opponent_hands.add(int(sample.get_hand(opp)))
                if state.side_to_move == p:
                    assert sample.get_legal_moves().tolist() == state.get_legal_moves().tolist()
                if not sample.is_terminal:
                    sample.random_playout(seed=s)
                    assert sample.is_terminal
            # 見えないカードが相手の手札の枚数より多ければ, 配分は乱数によって変わる
            if popcount(int(obs.unseen_cards)) > obs.opponent_hand_count > 0:
                assert len(opponent_hands) > 1


def test_sample_state_is_reproducible():
    obs = pybl.GameState(seed=3).observe(pybl.FIRST)
    assert str(obs.sample_state(seed=7)) == str(obs.sample_state(seed=7))


def test_ismcts_style_usage():
    """観測のみから局面を生成してプレイアウトする (情報集合モンテカルロ木探索などの使い方)"""
    state = pybl.GameState(seed=0)
    obs = state.observe()
    wins = {m: 0 for m in obs.get_legal_moves().tolist()}
    for i, move in enumerate(list(wins) * 3):
        sample = obs.sample_state(seed=i)
        sample.play(move)
        wins[move] += sample.random_playout(seed=i) == (pybl.FIRST_WIN if obs.player == pybl.FIRST else pybl.SECOND_WIN)
    assert sum(wins.values()) > 0


def popcount(x: int) -> int:
    return bin(x).count("1")
