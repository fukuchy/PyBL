import pybl


def test_card_encoding_roundtrip():
    for color in range(pybl.NUM_COLORS):
        for rank in range(1, pybl.NUM_RANKS + 1):
            card = pybl.make_card(color, rank)
            assert 0 <= card < pybl.NUM_CARDS
            assert pybl.card_color(card) == color
            assert pybl.card_rank(card) == rank
            assert pybl.parse_card_str(pybl.card_to_str(card)) == card


def test_card_str():
    assert pybl.card_to_str(pybl.make_card(pybl.RED, 5)) == "R5"
    assert pybl.card_to_str(pybl.make_card(pybl.ORANGE, 10)) == "O10"
    assert pybl.parse_card_str("y1") == pybl.make_card(pybl.YELLOW, 1)
    assert pybl.parse_card_str("R11") == pybl.NULL_CARD
    assert pybl.parse_card_str("X3") == pybl.NULL_CARD


def test_move_encoding_roundtrip():
    for card in range(pybl.NUM_CARDS):
        for flag in range(pybl.NUM_FLAGS):
            move = pybl.make_move(card, flag)
            assert 0 <= move < pybl.PASS_MOVE
            assert pybl.move_card(move) == card
            assert pybl.move_flag(move) == flag
            assert pybl.parse_move_str(pybl.move_to_str(move)) == move


def test_move_str():
    move = pybl.make_move(pybl.parse_card_str("R5"), 2)
    assert pybl.move_to_str(move) == "R5@3"
    assert pybl.parse_move_str("pass") == pybl.PASS_MOVE
    assert pybl.move_to_str(pybl.PASS_MOVE) == "pass"
    assert pybl.parse_move_str("R5@10") == pybl.NULL_MOVE


def test_card_iterator():
    cards = [pybl.parse_card_str(s) for s in ("R1", "B7", "O10")]
    bits = sum(1 << int(c) for c in cards)
    assert list(pybl.CardIterator(bits)) == sorted(cards)
    assert len(pybl.CardIterator(bits)) == 3
