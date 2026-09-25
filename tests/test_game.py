import random

import numpy as np
import pytest

import pybl


def popcount(x) -> int:
    return bin(int(x)).count("1")


def test_initial_state():
    state = pybl.GameState(seed=1)
    assert state.side_to_move == state.first_player
    assert state.result == pybl.NOT_OVER
    assert state.deck_count == pybl.NUM_CARDS - 2 * pybl.HAND_SIZE
    for p in (pybl.FIRST, pybl.SECOND):
        assert state.get_hand_count(p) == pybl.HAND_SIZE
    assert int(state.get_hand(pybl.FIRST)) & int(state.get_hand(pybl.SECOND)) == 0
    assert len(state.get_legal_moves()) == pybl.HAND_SIZE * pybl.NUM_FLAGS


def test_same_seed_is_reproducible():
    a, b = pybl.GameState(seed=123), pybl.GameState(seed=123)
    assert str(a) == str(b)
    assert a.random_playout(seed=7) == b.random_playout(seed=7)
    assert str(a) == str(b)


def test_illegal_move_raises():
    state = pybl.GameState(seed=1)
    opp_card = next(iter(pybl.CardIterator(state.get_hand(pybl.to_opponent(state.side_to_move)))))
    with pytest.raises(ValueError):
        state.play(pybl.make_move(opp_card, 0))
    with pytest.raises(ValueError):
        state.play(pybl.PASS_MOVE)


def test_copy_is_independent():
    state = pybl.GameState(seed=1)
    copied = state.copy()
    state.random_playout(seed=1)
    assert copied.move_count == 0
    assert not copied.is_terminal


def check_invariants(state: pybl.GameState):
    hands = [int(state.get_hand(p)) for p in (pybl.FIRST, pybl.SECOND)]
    board = int(state.board_cards)
    assert hands[0] & hands[1] == 0
    assert (hands[0] | hands[1]) & board == 0
    assert popcount(hands[0]) + popcount(hands[1]) + popcount(board) + state.deck_count == pybl.NUM_CARDS
    for p in (pybl.FIRST, pybl.SECOND):
        assert state.get_hand_count(p) <= pybl.HAND_SIZE
        for flag in range(pybl.NUM_FLAGS):
            assert len(state.get_flag_cards(flag, p)) <= pybl.FORMATION_SIZE
    assert state.get_claimed_flags(pybl.FIRST) & state.get_claimed_flags(pybl.SECOND) == 0


@pytest.mark.parametrize("seed", range(200))
def test_random_game(seed):
    rng = random.Random(seed)
    state = pybl.GameState(seed=seed)
    while not state.is_terminal:
        moves = state.get_legal_moves()
        assert len(moves) > 0
        # 配置可能な手がある限りパスはできない
        assert (pybl.PASS_MOVE in moves) == (len(moves) == 1 and moves[0] == pybl.PASS_MOVE)
        state.play(moves[rng.randrange(len(moves))])
        check_invariants(state)

    assert len(state.get_legal_moves()) == 0
    assert state.move_count <= 128

    claimed = [state.get_claimed_flags(p) for p in (pybl.FIRST, pybl.SECOND)]
    if state.is_forced_termination:
        # 両者の連続パスによる強制終局では, 確保したフラッグの数で勝敗を決める
        assert state.consecutive_pass_count == 2
        expected = (pybl.FIRST_WIN if popcount(claimed[0]) > popcount(claimed[1])
                    else pybl.SECOND_WIN if popcount(claimed[1]) > popcount(claimed[0])
                    else pybl.DRAW)
        assert state.result == expected


def snapshot(state: pybl.GameState) -> tuple:
    players = (pybl.FIRST, pybl.SECOND)
    return (
        str(state),
        int(state.side_to_move), int(state.result), state.is_forced_termination,
        state.consecutive_pass_count, state.move_count, state.deck_count, int(state.board_cards),
        tuple(int(state.get_hand(p)) for p in players),
        tuple(state.get_claimed_flags(p) for p in players),
        tuple(int(state.get_flag_first_completer(i)) for i in range(pybl.NUM_FLAGS)),
        tuple(state.get_legal_moves().tolist()),
    )


@pytest.mark.parametrize("seed", range(50))
def test_undo_restores_every_state(seed):
    rng = random.Random(seed)
    state = pybl.GameState(seed=seed)
    history = [snapshot(state)]
    while not state.is_terminal:
        moves = state.get_legal_moves()
        state.play(moves[rng.randrange(len(moves))])
        history.append(snapshot(state))

    history.pop()
    while history:
        state.undo()
        assert snapshot(state) == history.pop()

    with pytest.raises(RuntimeError):
        state.undo()


def test_undo_then_replay_gives_same_draw():
    state = pybl.GameState(seed=3)
    move = state.get_legal_moves()[0]
    state.play(move)
    after = snapshot(state)
    state.undo()
    state.play(move)
    assert snapshot(state) == after


@pytest.mark.parametrize("seed", range(50))
def test_determinize_keeps_visible_information(seed):
    rng = random.Random(seed)
    state = pybl.GameState(seed=seed)
    for _ in range(rng.randrange(40)):
        if state.is_terminal:
            break
        moves = state.get_legal_moves()
        state.play(moves[rng.randrange(len(moves))])

    p = state.side_to_move
    opp = pybl.to_opponent(p)
    visible = (state.get_hand(p), state.get_unseen_cards(p), state.board_cards, state.deck_count,
               state.get_hand_count(opp), state.get_legal_moves().tolist(),
               [state.get_flag_owner(i) for i in range(pybl.NUM_FLAGS)])

    state.determinize(p, seed=seed + 1000)

    assert (state.get_hand(p), state.get_unseen_cards(p), state.board_cards, state.deck_count,
            state.get_hand_count(opp), state.get_legal_moves().tolist(),
            [state.get_flag_owner(i) for i in range(pybl.NUM_FLAGS)]) == visible
    assert int(state.get_hand(opp)) & int(state.get_hand(p)) == 0
    assert state.move_count == 0
    check_invariants(state)

    state.random_playout(seed=seed)
    assert state.is_terminal


def test_determinize_shuffles_unseen_cards():
    state = pybl.GameState(seed=5)
    p = state.side_to_move
    opp = pybl.to_opponent(p)
    hands = set()
    for s in range(20):
        copied = state.copy()
        copied.determinize(p, seed=s)
        hands.add(int(copied.get_hand(opp)))
    assert len(hands) > 1
