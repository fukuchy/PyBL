from itertools import combinations

import pytest

import pybl
import reference


def cards(*names: str) -> list:
    return [pybl.parse_card_str(s) for s in names]


@pytest.mark.parametrize("names, expected", [
    (("R4", "R5", "R3"), pybl.WEDGE),
    (("Y8", "R8", "G8"), pybl.PHALANX),
    (("B2", "B7", "B4"), pybl.BATTALION),
    (("Y4", "R6", "G5"), pybl.SKIRMISHER),
    (("Y7", "B2", "G1"), pybl.HOST),
    # 数字の循環は連番としない
    (("R9", "R10", "R1"), pybl.BATTALION),
    (("R9", "Y10", "G1"), pybl.HOST),
])
def test_formation_type(names, expected):
    assert pybl.formation_type(*cards(*names)) == expected


def test_same_formation_compared_by_sum():
    assert pybl.evaluate_formation(*cards("R4", "R6", "R3")) > pybl.evaluate_formation(*cards("B7", "B1", "B3"))


def test_same_formation_same_sum_is_draw():
    assert pybl.evaluate_formation(*cards("Y7", "B2", "G1")) == pybl.evaluate_formation(*cards("Y3", "B3", "G4"))


def test_different_formation_ignores_sum():
    assert pybl.evaluate_formation(*cards("R1", "R2", "R3")) > pybl.evaluate_formation(*cards("Y10", "R10", "G10"))


def test_strength_decomposition():
    s = pybl.evaluate_formation(*cards("B2", "B7", "B4"))
    assert pybl.strength_to_formation_type(s) == pybl.BATTALION
    assert pybl.strength_to_sum(s) == 13


def test_matches_reference_for_all_combinations():
    all_cards = range(pybl.NUM_CARDS)
    combos = list(combinations(all_cards, 3))
    for combo in combos:
        assert pybl.formation_type(*combo) == reference.formation_type(list(combo))

    # 強さの順序関係が参照実装と一致することを確認する
    ordered = sorted(combos, key=lambda c: pybl.evaluate_formation(*c))
    ref = [reference.strength(list(c)) for c in ordered]
    assert ref == sorted(ref)
