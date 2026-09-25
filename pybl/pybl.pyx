# distutils: language = c++
# cython: language_level = 3

from libc.stdint cimport int8_t, int16_t, int32_t, uint16_t, uint64_t
from libcpp cimport bool as cbool

import os

import numpy as np


cdef extern from "cpp/constant.h":
    const int32_t c_NUM_COLORS "battleline::NUM_COLORS"
    const int32_t c_NUM_RANKS "battleline::NUM_RANKS"
    const int32_t c_NUM_CARDS "battleline::NUM_CARDS"
    const int32_t c_NUM_FLAGS "battleline::NUM_FLAGS"
    const int32_t c_FORMATION_SIZE "battleline::FORMATION_SIZE"
    const int32_t c_HAND_SIZE "battleline::HAND_SIZE"
    const int32_t c_MAX_LEGAL_MOVES "battleline::MAX_LEGAL_MOVES"

    const int8_t c_RED "battleline::RED"
    const int8_t c_YELLOW "battleline::YELLOW"
    const int8_t c_BLUE "battleline::BLUE"
    const int8_t c_GREEN "battleline::GREEN"
    const int8_t c_PURPLE "battleline::PURPLE"
    const int8_t c_ORANGE "battleline::ORANGE"
    const int8_t c_NULL_COLOR "battleline::NULL_COLOR"

    const int8_t c_NULL_CARD "battleline::NULL_CARD"

    const int8_t c_FIRST "battleline::FIRST"
    const int8_t c_SECOND "battleline::SECOND"
    const int8_t c_NULL_PLAYER "battleline::NULL_PLAYER"

    const int8_t c_FIRST_WIN "battleline::FIRST_WIN"
    const int8_t c_SECOND_WIN "battleline::SECOND_WIN"
    const int8_t c_DRAW "battleline::DRAW"
    const int8_t c_NOT_OVER "battleline::NOT_OVER"

    const int8_t c_HOST "battleline::HOST"
    const int8_t c_SKIRMISHER "battleline::SKIRMISHER"
    const int8_t c_BATTALION "battleline::BATTALION"
    const int8_t c_PHALANX "battleline::PHALANX"
    const int8_t c_WEDGE "battleline::WEDGE"
    const int8_t c_NULL_FORMATION "battleline::NULL_FORMATION"

    const int16_t c_PASS_MOVE "battleline::PASS_MOVE"
    const int16_t c_NULL_MOVE "battleline::NULL_MOVE"

    const uint64_t c_ALL_CARDS "battleline::ALL_CARDS"
    const uint16_t c_ALL_FLAGS "battleline::ALL_FLAGS"


cdef extern from "cpp/card.h":
    int8_t c_make_card "battleline::make_card"(int32_t color, int32_t rank)
    int32_t c_card_color "battleline::card_color"(int8_t card)
    int32_t c_card_rank "battleline::card_rank"(int8_t card)


cdef extern from "cpp/move.h":
    int16_t c_make_move "battleline::make_move"(int8_t card, int32_t flag)
    int8_t c_move_card "battleline::move_card"(int16_t move)
    int32_t c_move_flag "battleline::move_flag"(int16_t move)


cdef extern from "cpp/formation.h":
    const int32_t c_NULL_STRENGTH "battleline::NULL_STRENGTH"
    int32_t c_strength_formation "battleline::strength_formation"(int32_t s)
    int32_t c_strength_sum "battleline::strength_sum"(int32_t s)
    int32_t c_formation_type "battleline::formation_type"(int8_t a, int8_t b, int8_t c)
    int32_t c_evaluate_formation "battleline::evaluate_formation"(int8_t a, int8_t b, int8_t c)
    int32_t c_best_completion "battleline::best_completion"(const int8_t* partial, int32_t count, uint64_t available) except +
    int32_t c_worst_completion "battleline::worst_completion"(const int8_t* partial, int32_t count, uint64_t available) except +


cdef extern from "cpp/flag.h":
    cdef cppclass CFlag "battleline::Flag":
        int8_t owner
        int8_t first_completer
        void clear()
        void place(int8_t player, int8_t card)
        cbool is_full(int8_t player) const


cdef extern from "cpp/judge.h":
    int8_t c_judge_flag "battleline::judge_flag"(const CFlag& flag, uint64_t board, int8_t turn_player)


cdef extern from "cpp/card_iterator.h":
    cdef cppclass CCardIterator "battleline::CardIterator":
        CCardIterator()
        CCardIterator(uint64_t bits)
        cbool end() const
        int8_t next()
        int32_t size() const


cdef extern from "cpp/observation.h":
    cdef cppclass CObservation "battleline::Observation":
        int8_t player
        int8_t stm
        int8_t first
        int8_t game_result
        cbool forced_termination
        int8_t consecutive_passes
        uint64_t hand
        uint64_t unseen
        uint64_t board
        int32_t opponent_hand_count
        int32_t deck_count
        cbool operator==(const CObservation&) const
        cbool is_terminal() const
        int8_t winner() const
        cbool is_my_turn() const
        int32_t hand_count(int8_t player) const
        uint16_t claimed_flags(int8_t player) const
        int8_t flag_owner(int32_t flag) const
        int8_t flag_first_completer(int32_t flag) const
        int32_t flag_card_count(int32_t flag, int8_t player) const
        int8_t flag_card(int32_t flag, int8_t player, int32_t slot) const
        int32_t flag_strength(int32_t flag, int8_t player) const
        uint16_t placeable_flags(int8_t player) const
        int32_t get_legal_moves(int16_t* out) const


cdef extern from "cpp/game_state.h":
    cdef cppclass CGameState "battleline::GameState":
        CGameState() except +
        void reset(uint64_t seed)
        void set_position(const uint64_t* hands, const CFlag* flags, const int8_t* deck_cards, int32_t deck_count,
                          int8_t side_to_move, int8_t first_player) except +
        int8_t side_to_move() const
        int8_t first_player() const
        int8_t result() const
        cbool is_terminal() const
        int8_t winner() const
        cbool is_forced_termination() const
        int32_t consecutive_pass_count() const
        int32_t move_count() const
        uint64_t hand(int8_t player) const
        int32_t hand_count(int8_t player) const
        int32_t deck_count() const
        uint64_t board_cards() const
        uint64_t unseen_cards(int8_t player) const
        uint16_t claimed_flags(int8_t player) const
        int8_t flag_owner(int32_t flag) const
        int8_t flag_first_completer(int32_t flag) const
        int32_t flag_card_count(int32_t flag, int8_t player) const
        int8_t flag_card(int32_t flag, int8_t player, int32_t slot) const
        int32_t flag_strength(int32_t flag, int8_t player) const
        uint16_t placeable_flags(int8_t player) const
        int32_t get_legal_moves(int16_t* out) const
        cbool is_legal(int16_t move) const
        uint16_t play(int16_t move) except +
        void undo() except +
        void determinize(int8_t player, uint64_t seed) except +
        int8_t random_playout(uint64_t seed) except +
        CObservation observe(int8_t player) const
        void sample_from_observation(const CObservation& obs, uint64_t seed) except +


Card = np.int8
CardColor = np.int8
Player = np.int8
GameResult = np.int8
FormationType = np.int8
Move = np.int16

NUM_COLORS = c_NUM_COLORS
NUM_RANKS = c_NUM_RANKS
NUM_CARDS = c_NUM_CARDS
NUM_FLAGS = c_NUM_FLAGS
FORMATION_SIZE = c_FORMATION_SIZE
HAND_SIZE = c_HAND_SIZE
MAX_LEGAL_MOVES = c_MAX_LEGAL_MOVES

RED = CardColor(c_RED)
YELLOW = CardColor(c_YELLOW)
BLUE = CardColor(c_BLUE)
GREEN = CardColor(c_GREEN)
PURPLE = CardColor(c_PURPLE)
ORANGE = CardColor(c_ORANGE)
NULL_COLOR = CardColor(c_NULL_COLOR)

NULL_CARD = Card(c_NULL_CARD)

FIRST = Player(c_FIRST)
SECOND = Player(c_SECOND)
NULL_PLAYER = Player(c_NULL_PLAYER)

FIRST_WIN = GameResult(c_FIRST_WIN)
SECOND_WIN = GameResult(c_SECOND_WIN)
DRAW = GameResult(c_DRAW)
NOT_OVER = GameResult(c_NOT_OVER)

HOST = FormationType(c_HOST)
SKIRMISHER = FormationType(c_SKIRMISHER)
BATTALION = FormationType(c_BATTALION)
PHALANX = FormationType(c_PHALANX)
WEDGE = FormationType(c_WEDGE)
NULL_FORMATION = FormationType(c_NULL_FORMATION)

PASS_MOVE = Move(c_PASS_MOVE)
NULL_MOVE = Move(c_NULL_MOVE)

NULL_STRENGTH = c_NULL_STRENGTH

ALL_CARDS = np.uint64(c_ALL_CARDS)
ALL_FLAGS = c_ALL_FLAGS

_COLOR_CHARS = "RYBGPO"
_PLAYER_NAMES = ("First", "Second", "Null")
_RESULT_NAMES = ("FirstWin", "SecondWin", "Draw", "NotOver")
_FORMATION_NAMES = ("Host", "Skirmisher", "Battalion", "Phalanx", "Wedge", "Null")


cdef inline void _check_card(card) except *:
    if not 0 <= card < c_NUM_CARDS:
        raise ValueError(f"invalid card: {card}")


cdef inline void _check_flag(flag) except *:
    if not 0 <= flag < c_NUM_FLAGS:
        raise IndexError(f"invalid flag index: {flag}")


cdef inline void _check_player(player) except *:
    if player != c_FIRST and player != c_SECOND:
        raise ValueError(f"invalid player: {player}")


def _resolve_seed(seed) -> int:
    if seed is None:
        return int.from_bytes(os.urandom(8), "little")
    return int(seed) & 0xFFFFFFFFFFFFFFFF


def to_opponent(player: Player) -> Player:
    return Player(player ^ c_SECOND)


def player_to_str(player: Player) -> str:
    return _PLAYER_NAMES[player] if 0 <= player <= c_NULL_PLAYER else "Invalid"


def result_to_str(result: GameResult) -> str:
    return _RESULT_NAMES[result] if 0 <= result <= c_NOT_OVER else "Invalid"


def formation_type_to_str(formation: FormationType) -> str:
    return _FORMATION_NAMES[formation] if 0 <= formation <= c_NULL_FORMATION else "Invalid"


def make_card(color: CardColor, rank: int) -> Card:
    if not 0 <= color < c_NUM_COLORS or not 1 <= rank <= c_NUM_RANKS:
        raise ValueError(f"invalid color or rank: color={color}, rank={rank}")
    return Card(c_make_card(color, rank))


def card_color(card: Card) -> CardColor:
    _check_card(card)
    return CardColor(c_card_color(card))


def card_rank(card: Card) -> int:
    _check_card(card)
    return c_card_rank(card)


def card_to_str(card: Card) -> str:
    if card == c_NULL_CARD:
        return "Null"
    _check_card(card)
    return f"{_COLOR_CHARS[c_card_color(card)]}{c_card_rank(card)}"


def parse_card_str(s: str) -> Card:
    """"R5", "o10" のような文字列をカードに変換する. 変換できなければ NULL_CARD を返す"""
    s = s.strip().upper()
    if len(s) < 2 or s[0] not in _COLOR_CHARS or not s[1:].isdigit():
        return NULL_CARD
    rank = int(s[1:])
    if not 1 <= rank <= c_NUM_RANKS:
        return NULL_CARD
    return Card(c_make_card(_COLOR_CHARS.index(s[0]), rank))


def make_move(card: Card, flag: int) -> Move:
    _check_card(card)
    _check_flag(flag)
    return Move(c_make_move(card, flag))


def move_card(move: Move) -> Card:
    if not 0 <= move < c_PASS_MOVE:
        return NULL_CARD
    return Card(c_move_card(move))


def move_flag(move: Move) -> int:
    if not 0 <= move < c_PASS_MOVE:
        return -1
    return c_move_flag(move)


def move_to_str(move: Move) -> str:
    """着手を "R5@3" (フラッグ番号は1始まり) または "pass" に変換する"""
    if move == c_PASS_MOVE:
        return "pass"
    if not 0 <= move < c_PASS_MOVE:
        return "Null"
    return f"{card_to_str(c_move_card(move))}@{c_move_flag(move) + 1}"


def parse_move_str(s: str) -> Move:
    """"R5@3", "pass" のような文字列を着手に変換する. 変換できなければ NULL_MOVE を返す"""
    s = s.strip().lower()
    if s == "pass":
        return PASS_MOVE

    card_str, sep, flag_str = s.partition("@")
    card = parse_card_str(card_str)
    if not sep or card == c_NULL_CARD or not flag_str.isdigit():
        return NULL_MOVE

    flag = int(flag_str) - 1
    if not 0 <= flag < c_NUM_FLAGS:
        return NULL_MOVE
    return Move(c_make_move(card, flag))


def evaluate_formation(a: Card, b: Card, c: Card) -> int:
    """3枚のカードからなるフォーメーションの強さを返す. 値が大きいほど強い"""
    for card in (a, b, c):
        _check_card(card)
    return c_evaluate_formation(a, b, c)


def formation_type(a: Card, b: Card, c: Card) -> FormationType:
    for card in (a, b, c):
        _check_card(card)
    return FormationType(c_formation_type(a, b, c))


def strength_to_formation_type(strength: int) -> FormationType:
    if strength == c_NULL_STRENGTH:
        return NULL_FORMATION
    return FormationType(c_strength_formation(strength))


def strength_to_sum(strength: int) -> int:
    if strength == c_NULL_STRENGTH:
        return 0
    return c_strength_sum(strength)


def best_completion(partial, available) -> int:
    """partial (0 - 2枚) に available のカードを加えて作れる最強のフォーメーションの強さを返す"""
    cdef int8_t buf[3]
    cdef int32_t n = len(partial)
    if n >= c_FORMATION_SIZE:
        raise ValueError("partial must contain at most 2 cards")
    for i, card in enumerate(partial):
        _check_card(card)
        buf[i] = card
    return c_best_completion(buf, n, available)


def worst_completion(partial, available) -> int:
    """partial (0 - 2枚) に available のカードを加えて作れる最弱のフォーメーションの強さを返す"""
    cdef int8_t buf[3]
    cdef int32_t n = len(partial)
    if n >= c_FORMATION_SIZE:
        raise ValueError("partial must contain at most 2 cards")
    for i, card in enumerate(partial):
        _check_card(card)
        buf[i] = card
    return c_worst_completion(buf, n, available)


cdef void _build_flag(CFlag* flag, first_cards, second_cards, first_completer) except *:
    flag.clear()
    for player, cards in ((c_FIRST, first_cards), (c_SECOND, second_cards)):
        cards = list(cards)
        if len(cards) > c_FORMATION_SIZE:
            raise ValueError("at most 3 cards can be placed on each side of a flag")
        for card in cards:
            _check_card(card)
            flag.place(player, card)

    if first_completer is None:
        if flag.is_full(c_FIRST) and flag.is_full(c_SECOND):
            raise ValueError("first_completer is required when both formations are complete")
    else:
        flag.first_completer = first_completer


def judge_flag(first_cards, second_cards, board, turn_player: Player, first_completer: Player | None = None) -> Player:
    """フラッグの確保判定 ((1) 両側の比較, (2) 証明による確保, (3) 引き分け確定) を行い, 確保するプレイヤーを返す.
    確保されない場合は NULL_PLAYER を返す. board は盤面に出ている全てのカード (64bit整数).
    first_completer は双方のフォーメーションが完成している場合にのみ必須"""
    cdef CFlag flag
    _check_player(turn_player)
    _build_flag(&flag, first_cards, second_cards, first_completer)
    return Player(c_judge_flag(flag, board, turn_player))


def _format_position(state, hand_lines) -> str:
    """GameState / Observation の盤面を文字列にする"""
    def cards_str(cards):
        return " ".join(card_to_str(c) for c in cards)

    def owner_str(flag):
        owner = state.get_flag_owner(flag)
        return {c_FIRST: "<F", c_SECOND: "S>"}.get(owner, "  ")

    lines = [
        f"side to move: {player_to_str(state.side_to_move)}  "
        f"deck: {state.deck_count}  result: {result_to_str(state.result)}",
        f"{'First':>12} |flag| {'Second':<12}",
    ]
    for i in range(c_NUM_FLAGS):
        first = cards_str(state.get_flag_cards(i, FIRST))
        second = cards_str(state.get_flag_cards(i, SECOND))
        lines.append(f"{first:>12} |{i + 1}{owner_str(i)}| {second:<12}")
    for p, hand in hand_lines:
        lines.append(f"hand {player_to_str(p):<6}: {hand}")
    return "\n".join(lines)


def _cards_str(bits) -> str:
    return " ".join([card_to_str(c) for c in CardIterator(bits)])


cdef class CardIterator:
    """64bit整数で表現されたカードの集合から, カードを番号の小さい順に列挙する"""
    cdef CCardIterator _it

    def __cinit__(self, bits):
        self._it = CCardIterator(bits)

    def __iter__(self):
        return self

    def __next__(self):
        if self._it.end():
            raise StopIteration()
        return Card(self._it.next())

    def __len__(self):
        return self._it.size()


cdef class GameState:
    """バトルライン (戦術カード無し) の対局状態"""
    cdef CGameState _state

    def __cinit__(self, seed=None):
        self._state.reset(_resolve_seed(seed))

    def reset(self, seed=None):
        self._state.reset(_resolve_seed(seed))

    def set_position(self, side_to_move: Player, hands, flags, deck=(), owners=None, first_completers=None,
                     first_player: Player | None = None):
        """任意の局面を設定する.

        hands: (先手の手札, 後手の手札). それぞれカードの列
        flags: NUM_FLAGS 個の (先手側のカード列, 後手側のカード列). カードは配置した順に並べる
        deck: 山札のカード列. 先頭から順に引かれる
        owners: 各フラッグを確保したプレイヤー (None なら全て未確保)
        first_completers: 各フラッグで先に3枚目を置いたプレイヤー.
            None の要素は一方のみ完成している場合はそのプレイヤー, どちらも未完成なら NULL_PLAYER と推定する
        first_player: 先手のプレイヤー (None なら FIRST)

        全てのカードが手札, フラッグ, 山札のいずれかにちょうど1回ずつ現れなければならない.
        不正な局面の場合は ValueError を送出する. undo の履歴は破棄される"""
        cdef uint64_t c_hands[2]
        cdef CFlag c_flags[9]
        cdef int8_t c_deck[60]

        _check_player(side_to_move)
        first_player = FIRST if first_player is None else first_player
        _check_player(first_player)

        if len(hands) != 2:
            raise ValueError("hands must contain 2 elements")
        for p in range(2):
            cards = list(hands[p])
            for card in cards:
                _check_card(card)
            if len(set(int(c) for c in cards)) != len(cards):
                raise ValueError("each card must appear exactly once")
            c_hands[p] = sum(1 << int(c) for c in cards)

        flags = list(flags)
        owners = [None] * c_NUM_FLAGS if owners is None else list(owners)
        first_completers = [None] * c_NUM_FLAGS if first_completers is None else list(first_completers)
        if len(flags) != c_NUM_FLAGS or len(owners) != c_NUM_FLAGS or len(first_completers) != c_NUM_FLAGS:
            raise ValueError(f"flags, owners and first_completers must contain {c_NUM_FLAGS} elements")
        for i in range(c_NUM_FLAGS):
            first_cards, second_cards = flags[i]
            _build_flag(&c_flags[i], first_cards, second_cards, first_completers[i])
            c_flags[i].owner = c_NULL_PLAYER if owners[i] is None else owners[i]

        deck = list(deck)
        if len(deck) > c_NUM_CARDS:
            raise ValueError("too many cards in the deck")
        for i, card in enumerate(deck):
            _check_card(card)
            c_deck[i] = card

        self._state.set_position(c_hands, c_flags, c_deck, len(deck), side_to_move, first_player)

    @property
    def side_to_move(self) -> Player:
        return Player(self._state.side_to_move())

    @property
    def first_player(self) -> Player:
        return Player(self._state.first_player())

    @property
    def result(self) -> GameResult:
        return GameResult(self._state.result())

    @property
    def winner(self) -> Player:
        return Player(self._state.winner())

    @property
    def is_terminal(self) -> bool:
        return self._state.is_terminal()

    @property
    def is_forced_termination(self) -> bool:
        return self._state.is_forced_termination()

    @property
    def consecutive_pass_count(self) -> int:
        return self._state.consecutive_pass_count()

    @property
    def move_count(self) -> int:
        return self._state.move_count()

    @property
    def deck_count(self) -> int:
        return self._state.deck_count()

    @property
    def board_cards(self) -> np.uint64:
        return np.uint64(self._state.board_cards())

    def get_hand(self, player: Player) -> np.uint64:
        _check_player(player)
        return np.uint64(self._state.hand(player))

    def get_hand_count(self, player: Player) -> int:
        _check_player(player)
        return self._state.hand_count(player)

    def get_unseen_cards(self, player: Player) -> np.uint64:
        _check_player(player)
        return np.uint64(self._state.unseen_cards(player))

    def get_claimed_flags(self, player: Player) -> int:
        _check_player(player)
        return self._state.claimed_flags(player)

    def get_placeable_flags(self, player: Player) -> int:
        _check_player(player)
        return self._state.placeable_flags(player)

    def get_flag_owner(self, flag: int) -> Player:
        _check_flag(flag)
        return Player(self._state.flag_owner(flag))

    def get_flag_first_completer(self, flag: int) -> Player:
        _check_flag(flag)
        return Player(self._state.flag_first_completer(flag))

    def get_flag_cards(self, flag: int, player: Player) -> list:
        _check_flag(flag)
        _check_player(player)
        return [Card(self._state.flag_card(flag, player, i)) for i in range(self._state.flag_card_count(flag, player))]

    def get_flag_strength(self, flag: int, player: Player) -> int:
        _check_flag(flag)
        _check_player(player)
        return self._state.flag_strength(flag, player)

    def get_legal_moves(self) -> np.ndarray:
        cdef int16_t[::1] view
        out = np.empty(c_MAX_LEGAL_MOVES, dtype=np.int16)
        view = out
        n = self._state.get_legal_moves(&view[0])
        return out[:n]

    def is_legal(self, move: Move) -> bool:
        if not 0 <= move <= c_PASS_MOVE:
            return False
        return self._state.is_legal(move)

    def play(self, move: Move) -> int:
        """1手進め, この手で確保されたフラッグをビットマスクで返す. 非合法手なら ValueError を送出する"""
        if not 0 <= move <= c_PASS_MOVE:
            raise ValueError(f"invalid move: {move}")
        return self._state.play(move)

    def undo(self):
        self._state.undo()

    def determinize(self, player: Player, seed=None):
        _check_player(player)
        self._state.determinize(player, _resolve_seed(seed))

    def random_playout(self, seed=None) -> GameResult:
        """終局までランダムに着手し, 対局結果を返す. この状態自体が終局まで進む"""
        return GameResult(self._state.random_playout(_resolve_seed(seed)))

    def copy_to(self, GameState dest):
        dest._state = self._state

    def copy(self) -> GameState:
        cdef GameState state = GameState.__new__(GameState, 0)
        self.copy_to(state)
        return state

    def observe(self, player: Player | None = None) -> Observation:
        """player (省略時は手番プレイヤー) から見える情報のみを取り出す"""
        player = self.side_to_move if player is None else player
        _check_player(player)
        cdef Observation obs = Observation.__new__(Observation)
        obs._obs = self._state.observe(player)
        return obs

    def __str__(self) -> str:
        return _format_position(self, [(p, _cards_str(self._state.hand(p))) for p in (FIRST, SECOND)])


cdef class Observation:
    """あるプレイヤーから見える情報のみからなる局面. GameState.observe で生成する.

    相手の手札と山札の中身は含まず, それらを合わせたカードの集合 (unseen_cards) と各枚数のみを持つ.
    盤面に関するメソッドは GameState と同じ名前で利用できる"""
    cdef CObservation _obs

    def __init__(self):
        raise TypeError("Observation cannot be created directly; use GameState.observe()")

    @property
    def player(self) -> Player:
        """観測しているプレイヤー"""
        return Player(self._obs.player)

    @property
    def side_to_move(self) -> Player:
        return Player(self._obs.stm)

    @property
    def first_player(self) -> Player:
        return Player(self._obs.first)

    @property
    def result(self) -> GameResult:
        return GameResult(self._obs.game_result)

    @property
    def winner(self) -> Player:
        return Player(self._obs.winner())

    @property
    def is_terminal(self) -> bool:
        return self._obs.is_terminal()

    @property
    def is_my_turn(self) -> bool:
        """観測しているプレイヤーの手番であり, かつ終局していないか"""
        return self._obs.is_my_turn()

    @property
    def is_forced_termination(self) -> bool:
        return self._obs.forced_termination

    @property
    def consecutive_pass_count(self) -> int:
        return self._obs.consecutive_passes

    @property
    def deck_count(self) -> int:
        return self._obs.deck_count

    @property
    def board_cards(self) -> np.uint64:
        return np.uint64(self._obs.board)

    @property
    def hand(self) -> np.uint64:
        """観測しているプレイヤーの手札"""
        return np.uint64(self._obs.hand)

    @property
    def opponent_hand_count(self) -> int:
        return self._obs.opponent_hand_count

    @property
    def unseen_cards(self) -> np.uint64:
        """観測しているプレイヤーから見えないカード (相手の手札 + 山札)"""
        return np.uint64(self._obs.unseen)

    def get_hand(self, player: Player | None = None) -> np.uint64:
        """観測しているプレイヤーの手札. 相手の手札は見えないため ValueError を送出する"""
        if player is not None and player != self._obs.player:
            raise ValueError("the opponent's hand is not visible")
        return np.uint64(self._obs.hand)

    def get_hand_count(self, player: Player) -> int:
        _check_player(player)
        return self._obs.hand_count(player)

    def get_unseen_cards(self) -> np.uint64:
        return np.uint64(self._obs.unseen)

    def get_claimed_flags(self, player: Player) -> int:
        _check_player(player)
        return self._obs.claimed_flags(player)

    def get_placeable_flags(self, player: Player) -> int:
        _check_player(player)
        return self._obs.placeable_flags(player)

    def get_flag_owner(self, flag: int) -> Player:
        _check_flag(flag)
        return Player(self._obs.flag_owner(flag))

    def get_flag_first_completer(self, flag: int) -> Player:
        _check_flag(flag)
        return Player(self._obs.flag_first_completer(flag))

    def get_flag_cards(self, flag: int, player: Player) -> list:
        _check_flag(flag)
        _check_player(player)
        return [Card(self._obs.flag_card(flag, player, i)) for i in range(self._obs.flag_card_count(flag, player))]

    def get_flag_strength(self, flag: int, player: Player) -> int:
        _check_flag(flag)
        _check_player(player)
        return self._obs.flag_strength(flag, player)

    def get_legal_moves(self) -> np.ndarray:
        """観測しているプレイヤーの合法手. 相手の手番や終局後は空の配列を返す"""
        cdef int16_t[::1] view
        out = np.empty(c_MAX_LEGAL_MOVES, dtype=np.int16)
        view = out
        n = self._obs.get_legal_moves(&view[0])
        return out[:n]

    def is_legal(self, move: Move) -> bool:
        return bool(np.any(self.get_legal_moves() == move))

    def sample_state(self, seed=None) -> GameState:
        """この観測と矛盾しない局面を生成する. 見えないカードは相手の手札と山札にランダムに配分される.
        生成した局面の undo の履歴は空になる"""
        cdef GameState state = GameState.__new__(GameState, 0)
        state._state.sample_from_observation(self._obs, _resolve_seed(seed))
        return state

    def __eq__(self, other) -> bool:
        if not isinstance(other, Observation):
            return NotImplemented
        return self._obs == (<Observation>other)._obs

    __hash__ = None

    def __str__(self) -> str:
        me = self._obs.player
        opp = me ^ c_SECOND
        hands = {me: _cards_str(self._obs.hand), opp: f"? x{self._obs.opponent_hand_count}"}
        return _format_position(self, [(p, hands[p]) for p in (FIRST, SECOND)])
