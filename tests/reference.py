"""C++実装との突き合わせに用いる, Pythonのみで書かれた素朴な参照実装"""
from itertools import combinations

NUM_RANKS = 10
NUM_CARDS = 60

FIRST, SECOND = 0, 1

HOST, SKIRMISHER, BATTALION, PHALANX, WEDGE = range(5)


def color_of(card: int) -> int:
    return card // NUM_RANKS


def rank_of(card: int) -> int:
    return card % NUM_RANKS + 1


def formation_type(cards: list[int]) -> int:
    colors = {color_of(c) for c in cards}
    ranks = sorted(rank_of(c) for c in cards)
    same_color = len(colors) == 1
    same_rank = len(set(ranks)) == 1
    consecutive = ranks == list(range(ranks[0], ranks[0] + len(ranks)))

    if same_color and consecutive:
        return WEDGE
    if same_rank:
        return PHALANX
    if same_color:
        return BATTALION
    if consecutive:
        return SKIRMISHER
    return HOST


def strength(cards: list[int]) -> tuple[int, int]:
    """(フォーメーションの種類, 合計) のタプル. タプルの大小比較で強弱を判定できる"""
    return formation_type(cards), sum(rank_of(c) for c in cards)


def completions(partial: list[int], available: set[int]):
    candidates = sorted(available - set(partial))
    for extra in combinations(candidates, 3 - len(partial)):
        yield list(partial) + list(extra)


def best_completion(partial: list[int], available: set[int]):
    """全ての完成形を列挙して最強の強さを返す. 完成できなければ None"""
    return max((strength(c) for c in completions(partial, available)), default=None)


def worst_completion(partial: list[int], available: set[int]):
    return min((strength(c) for c in completions(partial, available)), default=None)


def judge_flag(cards: list[list[int]], first_completer, board: set[int], turn_player: int):
    """フラッグを確保するプレイヤーを返す. 確保されなければ None"""
    full = [len(cards[p]) == 3 for p in (FIRST, SECOND)]

    # (1) 両側の比較
    if all(full):
        s0, s1 = strength(cards[FIRST]), strength(cards[SECOND])
        if s0 != s1:
            return FIRST if s0 > s1 else SECOND
        return first_completer

    if not any(full):
        return None

    complete = FIRST if full[FIRST] else SECOND
    incomplete = 1 - complete
    mine = strength(cards[complete])
    available = set(range(NUM_CARDS)) - board
    outcomes = {strength(c) for c in completions(cards[incomplete], available)}

    # (2) 証明による確保: 相手がどう完成させても自分に勝てない
    if complete == turn_player and all(s <= mine for s in outcomes):
        return complete

    # (3) 引き分け確定: どう完成させても引き分けにしかならない
    if outcomes == {mine}:
        return first_completer

    return None
