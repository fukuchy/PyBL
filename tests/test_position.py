"""set_position と両者連続パス時の強制終局"""
import pytest

import pybl

F, S = pybl.FIRST, pybl.SECOND


def cards(*names: str) -> list:
    return [pybl.parse_card_str(s) for s in names]


def build_position(explicit_flags: dict, first_hand: list, second_hand: list, deck: list,
                   owners: list, side_to_move=F) -> dict:
    """explicit_flags 以外のフラッグには, 残りのカードを3枚ずつ両側に配置する (先に完成させたのは先手とする)"""
    used = {int(c) for c in first_hand + second_hand + deck}
    for first_cards, second_cards in explicit_flags.values():
        used |= {int(c) for c in first_cards + second_cards}
    rest = [c for c in range(pybl.NUM_CARDS) if c not in used]

    flags, first_completers = [], []
    for i in range(pybl.NUM_FLAGS):
        if i in explicit_flags:
            flags.append(explicit_flags[i])
            first_completers.append(None)
        else:
            chunk, rest = rest[:6], rest[6:]
            flags.append((chunk[:3], chunk[3:]))
            first_completers.append(F)
    assert not rest

    return dict(side_to_move=side_to_move, hands=(first_hand, second_hand), flags=flags, deck=deck,
                owners=owners, first_completers=first_completers)


def reachable_forced_termination_position() -> dict:
    """実際の対局で到達しうる強制終局の直前の局面.

    先手は手札も山札も無く, 後手は配置可能なフラッグが無い. フラッグ5 (index 4) は後手の G1, G2, G3 (ウェッジ) に対し,
    先手の Y5, Y6 は後手の手札にある Y7 でより強いウェッジを作りうるため, 証明による確保ができない.
    他のフラッグは先手と後手が4本ずつ (3本連続無し) 確保済み
    """
    return build_position(
        explicit_flags={4: (cards("Y5", "Y6"), cards("G1", "G2", "G3"))},
        first_hand=[],
        second_hand=cards("Y7", "R1", "R2", "R3", "R4", "R5", "R6"),
        deck=[],
        owners=[F, F, S, S, None, F, F, S, S])


def test_set_position():
    state = pybl.GameState(seed=0)
    state.set_position(**reachable_forced_termination_position())
    assert state.side_to_move == F
    assert state.get_hand_count(F) == 0
    assert state.get_hand_count(S) == 7
    assert state.deck_count == 0
    assert state.get_claimed_flags(F) == 0b001100011
    assert state.get_claimed_flags(S) == 0b110001100
    assert state.get_flag_first_completer(4) == S
    assert state.get_flag_cards(4, F) == cards("Y5", "Y6")
    assert state.result == pybl.NOT_OVER
    assert state.move_count == 0


def test_set_position_detects_victory():
    position = reachable_forced_termination_position()
    position["owners"] = [F, F, F, S, None, F, F, S, S]  # 先手が3本連続
    state = pybl.GameState(seed=0)
    state.set_position(**position)
    assert state.result == pybl.FIRST_WIN


def move_flag_card_to_second_hand(p: dict):
    """フラッグ1の後手側の3枚目を後手の手札に移し, 後手の手札を8枚にする"""
    first_cards, second_cards = p["flags"][0]
    p["flags"][0] = (first_cards, second_cards[:2])
    p["hands"] = (p["hands"][0], p["hands"][1] + second_cards[2:])


@pytest.mark.parametrize("modify", [
    lambda p: p.update(deck=[p["hands"][1][0]]),                   # カードの重複
    lambda p: p.update(hands=([], p["hands"][1][1:])),             # カードの欠落
    lambda p: p.update(first_completers=[None] * pybl.NUM_FLAGS),  # 双方完成のフラッグで first_completer が無い
    lambda p: p["first_completers"].__setitem__(4, F),             # 完成していない側が first_completer
    move_flag_card_to_second_hand,                                 # 手札が8枚
])
def test_set_position_rejects_invalid(modify):
    position = reachable_forced_termination_position()
    modify(position)
    state = pybl.GameState(seed=0)
    with pytest.raises(ValueError):
        state.set_position(**position)


def test_forced_termination_after_both_players_pass():
    state = pybl.GameState(seed=0)
    state.set_position(**reachable_forced_termination_position())
    before = str(state)

    assert state.get_legal_moves().tolist() == [pybl.PASS_MOVE]
    assert state.play(pybl.PASS_MOVE) == 0
    assert not state.is_terminal

    # 後手は手札があるが配置可能なフラッグが無いためパスする. フラッグ5は証明できず確保されない
    assert state.get_legal_moves().tolist() == [pybl.PASS_MOVE]
    assert state.play(pybl.PASS_MOVE) == 0

    assert state.is_terminal
    assert state.is_forced_termination
    assert state.result == pybl.DRAW  # 確保したフラッグは4本ずつ
    assert len(state.get_legal_moves()) == 0

    state.undo()
    assert not state.is_terminal
    assert not state.is_forced_termination
    state.undo()
    assert str(state) == before


def test_forced_termination_winner_is_player_with_more_flags():
    # フラッグ5, 7 (index 4, 6) が未確保で, 先手 3本, 後手 4本を確保済み.
    # 山札の O10 により先手はフラッグ7で O8, O9, O10 のウェッジを作りうるため, 後手は証明による確保ができない
    state = pybl.GameState(seed=0)
    state.set_position(**build_position(
        explicit_flags={4: (cards("Y5", "Y6"), cards("G1", "G2", "G3")),
                        6: (cards("O8", "O9"), cards("P1", "P2", "P3"))},
        first_hand=[],
        second_hand=cards("Y7", "R1", "R2", "R3", "R4", "R5", "R6"),
        deck=cards("O10"),
        owners=[F, F, S, S, None, F, None, S, S]))

    state.play(pybl.PASS_MOVE)
    assert not state.is_terminal
    state.play(pybl.PASS_MOVE)
    assert state.is_forced_termination
    assert state.result == pybl.SECOND_WIN
    assert state.winner == S


def test_placement_after_set_position():
    state = pybl.GameState(seed=0)
    state.set_position(**build_position(
        explicit_flags={4: (cards("Y5", "Y6"), cards("G1", "G2", "G3"))},
        first_hand=cards("Y4"),
        second_hand=cards("Y7", "R1", "R2", "R3", "R4", "R5"),
        deck=[],
        owners=[F, F, S, S, None, F, F, S, S]))

    assert state.get_legal_moves().tolist() == [pybl.make_move(pybl.parse_card_str("Y4"), 4)]
    assert state.play(pybl.parse_move_str("Y4@5")) == 1 << 4
    # 先手の Y4, Y5, Y6 のウェッジ (合計15) が後手の G1, G2, G3 (合計6) に勝ち, 先手は5本目を確保する
    assert state.get_flag_owner(4) == F
    assert state.result == pybl.FIRST_WIN
    assert not state.is_forced_termination


def comparison_win_before_proof_position() -> dict:
    """先手が B4 をフラッグ6 (index 5) に置くと, (1) で後手がフラッグ6 を確保して 5-6-7 の連続3本で勝利する.

    同じ配置の後, 先手はフラッグ1 (index 0) を (2) 証明で確保でき, 確保すれば 1-2-3 の連続3本になるが,
    (1) の段階で後手の勝利が決まるため, (2) の判定は行われない
    """
    explicit_flags = {0: (cards("Y8", "Y9", "Y10"), cards("G9")),
                      5: (cards("R1", "O2"), cards("P5", "P6", "P7"))}
    first_hand, second_hand = cards("B4"), cards("B5", "B6")
    used = {int(c) for c in first_hand + second_hand}
    for first_cards, second_cards in explicit_flags.values():
        used |= {int(c) for c in first_cards + second_cards}
    deck = [c for c in range(pybl.NUM_CARDS) if c not in used][-6:]     # 他のフラッグ7本に配れない6枚は山札に置く
    return build_position(explicit_flags=explicit_flags, first_hand=first_hand, second_hand=second_hand,
                          deck=deck, owners=[None, F, F, S, S, None, S, F, F])


def test_comparison_win_ends_game_before_proof():
    state = pybl.GameState(seed=0)
    state.set_position(**comparison_win_before_proof_position())
    assert not state.is_terminal
    claimed = state.play(pybl.make_move(pybl.parse_card_str("B4"), 5))
    assert state.result == pybl.SECOND_WIN
    assert claimed == 1 << 5
    assert state.get_flag_owner(0) == pybl.NULL_PLAYER
    state.undo()
    assert not state.is_terminal and state.get_flag_owner(5) == pybl.NULL_PLAYER


def test_proof_still_claimed_when_no_one_wins_by_comparison():
    """(1) で勝利条件が成立しなければ, 同じ手で (2) の確保と勝利の確認が行われる"""
    position = comparison_win_before_proof_position()
    position["owners"] = [None, F, F, S, F, None, S, F, S]      # 後手はフラッグ6 を確保しても連続3本にならない
    state = pybl.GameState(seed=0)
    state.set_position(**position)
    claimed = state.play(pybl.make_move(pybl.parse_card_str("B4"), 5))
    assert claimed == (1 << 5) | (1 << 0)
    assert state.result == pybl.FIRST_WIN
