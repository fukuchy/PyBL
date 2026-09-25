"""フラッグの確保判定. ルール文書 (rule/ruleNoTactics.md) の判定例と, 参照実装との突き合わせ"""
import random

import pytest

import pybl
import reference


def cards(*names: str) -> list:
    return [pybl.parse_card_str(s) for s in names]


def board_mask(*names: str) -> int:
    return sum(1 << int(c) for c in cards(*names))


def to_ref_strength(s: int):
    if s == pybl.NULL_STRENGTH:
        return None
    return int(pybl.strength_to_formation_type(s)), pybl.strength_to_sum(s)


def test_example1_wedge_beats_phalanx():
    assert pybl.evaluate_formation(*cards("R3", "R4", "R5")) > pybl.evaluate_formation(*cards("Y8", "B8", "G8"))


def test_example2_phalanx_proves_against_partial():
    mine = pybl.evaluate_formation(*cards("Y2", "R2", "G2"))
    available = int(pybl.ALL_CARDS) & ~board_mask("Y2", "R2", "G2", "R7", "G8")
    best = pybl.best_completion(cards("R7", "G8"), available)
    assert pybl.strength_to_formation_type(best) == pybl.SKIRMISHER
    assert best < mine


def test_example3_wedge_proves_with_cards_elsewhere():
    mine = pybl.evaluate_formation(*cards("R3", "R4", "R5"))
    available = int(pybl.ALL_CARDS) & ~board_mask("R3", "R4", "R5", "B8")
    # 青7, 青10が盤面に無ければ青8を含むウェッジを作られる可能性がある
    assert pybl.strength_to_formation_type(pybl.best_completion(cards("B8"), available)) == pybl.WEDGE

    available &= ~board_mask("B7", "B10")
    best = pybl.best_completion(cards("B8"), available)
    assert pybl.strength_to_formation_type(best) == pybl.PHALANX
    assert best < mine


def test_no_wraparound_in_completion():
    available = board_mask("R10", "R1")
    assert pybl.strength_to_formation_type(pybl.best_completion(cards("R9"), available)) == pybl.BATTALION


def test_cannot_complete():
    assert pybl.best_completion(cards("R1"), board_mask("Y5")) == pybl.NULL_STRENGTH
    assert pybl.worst_completion(cards("R1"), board_mask("Y5")) == pybl.NULL_STRENGTH


def test_completion_of_complete_formation():
    s = pybl.evaluate_formation(*cards("R1", "Y5", "B9"))
    assert pybl.best_completion(cards("R1", "Y5"), board_mask("B9")) == s
    assert pybl.worst_completion(cards("R1", "Y5"), board_mask("B9")) == s


@pytest.mark.parametrize("seed", range(300))
def test_completion_matches_reference(seed):
    rng = random.Random(seed)
    count = rng.choice([0, 1, 1, 2, 2, 2])
    # 部分的な配置は同じ色や連番になりやすいように偏らせる
    color = rng.randrange(pybl.NUM_COLORS)
    pool = [c for c in range(pybl.NUM_CARDS) if rng.random() < 0.3 or c // 10 == color]
    partial = rng.sample(pool, count)
    num_available = rng.randrange(0, 24 if count == 0 else 58)
    available = set(rng.sample([c for c in range(pybl.NUM_CARDS) if c not in partial], num_available))
    mask = sum(1 << c for c in available)

    assert to_ref_strength(pybl.best_completion(partial, mask)) == reference.best_completion(partial, available)
    assert to_ref_strength(pybl.worst_completion(partial, mask)) == reference.worst_completion(partial, available)


@pytest.mark.parametrize("seed", range(15))
def test_claims_match_reference_in_random_games(seed):
    """ランダム対局の各手で, 確保されたフラッグが参照実装の判定と一致することを確認する"""
    rng = random.Random(seed)
    state = pybl.GameState(seed=seed)
    first_completer = [None] * pybl.NUM_FLAGS

    while not state.is_terminal:
        player = int(state.side_to_move)
        moves = state.get_legal_moves()
        move = moves[rng.randrange(len(moves))]
        before = [int(state.get_flag_owner(i)) for i in range(pybl.NUM_FLAGS)]
        claimed = state.play(move)

        if move != pybl.PASS_MOVE:
            flag = pybl.move_flag(move)
            if first_completer[flag] is None and len(state.get_flag_cards(flag, player)) == 3:
                first_completer[flag] = player
            assert state.get_flag_first_completer(flag) == (
                pybl.NULL_PLAYER if first_completer[flag] is None else first_completer[flag])

        board = {int(c) for c in pybl.CardIterator(state.board_cards)}
        for i in range(pybl.NUM_FLAGS):
            if before[i] != pybl.NULL_PLAYER:
                continue
            flag_cards = [[int(c) for c in state.get_flag_cards(i, p)] for p in (pybl.FIRST, pybl.SECOND)]
            expected = reference.judge_flag(flag_cards, first_completer[i], board, player)
            assert bool(claimed >> i & 1) == (expected is not None)
            if expected is not None:
                assert state.get_flag_owner(i) == expected


def test_judge_flag_rule3_draw_is_certain():
    # 後手の R1, Y2, B4 (ホスト, 合計7) に対し, 先手の G1, P2 は盤面に無い4のカードでしか完成できず必ず合計7のホストになる
    first, second = cards("G1", "P2"), cards("R1", "Y2", "B4")
    off_board = {int(c) for c in cards("Y4", "G4", "P4", "O4")}
    board = int(pybl.ALL_CARDS) & ~sum(1 << c for c in off_board)
    # (2) は手番プレイヤーのみなので先手の手番では判定されず, (3) で先に完成させた後手が確保する
    assert pybl.judge_flag(first, second, board, pybl.FIRST) == pybl.SECOND

    # 5のカードが盤面に無ければ合計が変わりうるので確保されない
    board &= ~(1 << int(pybl.parse_card_str("R5")))
    assert pybl.judge_flag(first, second, board, pybl.FIRST) == pybl.NULL_PLAYER


def test_judge_flag_rule2_only_for_turn_player():
    first, second = cards("Y2", "R2", "G2"), cards("R7", "G8")
    board = board_mask("Y2", "R2", "G2", "R7", "G8")
    assert pybl.judge_flag(first, second, board, pybl.FIRST) == pybl.FIRST
    assert pybl.judge_flag(first, second, board, pybl.SECOND) == pybl.NULL_PLAYER


def test_judge_flag_rule1_tie_goes_to_first_completer():
    first, second = cards("Y7", "B2", "G1"), cards("Y3", "B3", "G4")
    board = board_mask("Y7", "B2", "G1", "Y3", "B3", "G4")
    assert pybl.judge_flag(first, second, board, pybl.FIRST, first_completer=pybl.SECOND) == pybl.SECOND
    with pytest.raises(ValueError):
        pybl.judge_flag(first, second, board, pybl.FIRST)


@pytest.mark.parametrize("seed", range(400))
def test_judge_flag_matches_reference(seed):
    """盤面に出ていないカードが少ない局面も含めて, 参照実装の判定と一致することを確認する"""
    rng = random.Random(seed)
    deck = list(range(pybl.NUM_CARDS))
    rng.shuffle(deck)
    counts = [3, rng.randrange(3)] if rng.random() < 0.8 else [3, 3]
    if rng.random() < 0.5:
        counts.reverse()
    first, second = deck[:counts[0]], deck[counts[0]:counts[0] + counts[1]]
    rest = deck[counts[0] + counts[1]:]
    num_off_board = rng.choice([1, 2, 3, 4, 6, 10, 20]) if 0 in counts or min(counts) < 3 else 10
    board = set(first) | set(second) | set(rest[num_off_board:])
    turn_player = rng.choice([pybl.FIRST, pybl.SECOND])
    first_completer = rng.choice([pybl.FIRST, pybl.SECOND]) if counts == [3, 3] else None

    ref_completer = first_completer
    if ref_completer is None and 3 in counts:
        ref_completer = pybl.FIRST if counts[0] == 3 else pybl.SECOND
    expected = reference.judge_flag([first, second], ref_completer, board, int(turn_player))

    actual = pybl.judge_flag(first, second, sum(1 << c for c in board), turn_player, first_completer)
    assert actual == (pybl.NULL_PLAYER if expected is None else expected)
