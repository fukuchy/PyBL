# PyBL
[![CI](https://github.com/fukuchy/PyBL/actions/workflows/ci.yml/badge.svg)](https://github.com/fukuchy/PyBL/actions/workflows/ci.yml)

Pythonから利用可能なバトルライン (戦術カード無し) のライブラリです。対局状態の管理、合法手の列挙、フラッグの確保判定をC++で実装し、Cythonを通してPythonから呼び出します。
ルールは [rule/ruleNoTactics.md](rule/ruleNoTactics.md) に従います。

## インストール
以下のコマンドを実行する前に、C++20に対応したC/C++のコンパイラをインストールしてください。

### pipを用いる場合
```
pip install git+https://github.com/fukuchy/PyBL
```

### uvを用いる場合
```
uv add git+https://github.com/fukuchy/PyBL
```

ビルドしたマシン固有の命令セット (`-march=native`) を使わずにビルドする場合は、環境変数 `PYBL_PORTABLE=1` を指定してください。

## 開発

```
uv sync
uv run python setup.py build_ext --inplace
uv run pytest
uv run python benchmarks/bench_playout.py
```

GitHub Actions で Linux / macOS / Windows と Python 3.10 / 3.13 の組み合わせについて、sdist からのビルドとテストを行っています ([.github/workflows/ci.yml](.github/workflows/ci.yml))。

## ディレクトリ構成

```
pybl/
├── pybl.pyx          # Cythonのバインディング
├── pybl.pyi          # 型スタブ
└── cpp/
    ├── constant.h        # 各種定数
    ├── card.h            # カードの表現とCardSet (uint64_t) の操作
    ├── card_iterator.h   # CardSetからカードを列挙
    ├── formation.h/.cpp  # フォーメーションの判定と強さの計算
    ├── flag.h            # フラッグの状態
    ├── judge.h/.cpp      # フラッグの確保判定 (1)(2)(3)
    ├── movegen.h         # 合法手の生成
    ├── observation.h     # プレイヤーから見える情報
    ├── deck.h            # 山札
    ├── move.h            # 着手の表現とundo用の記録
    ├── game_state.h/.cpp # 対局状態 (配置, 判定, 補充, 合法手, undo, determinize)
    └── utils/            # ビット演算, 乱数
tests/                # pytest (tests/reference.py はPythonのみの参照実装)
benchmarks/           # 速度計測
examples/             # 使用例
```

## 使用例

```python
import pybl

state = pybl.GameState(seed=42)
while not state.is_terminal:
    moves = state.get_legal_moves()
    state.play(moves[0])

print(state)
print(pybl.result_to_str(state.result))
```

探索では `play` と `undo` で局面を進めたり戻したりできます。

### プレイヤーから見える情報 (Observation)
`GameState.observe(player)` で、そのプレイヤーから見える情報のみを持つ `Observation` を取り出せます。相手の手札と山札の中身は含まれず、それらを合わせたカードの集合 (`unseen_cards`) と各枚数のみが分かります。AIには `GameState` の代わりに `Observation` を渡すことで、見えないはずの情報を誤って使うことを防げます。

```python
state = pybl.GameState(seed=0)
obs = state.observe(state.side_to_move)

print(obs)                        # 相手の手札は "? x7" のように枚数のみ表示される
obs.get_legal_moves()             # 自分の手番であれば合法手 (相手の手番や終局後は空)
obs.get_hand(obs.player)          # 自分の手札. 相手の手札を指定すると ValueError
```

不完全情報の探索 (ISMCTS など) では、`sample_state` で観測と矛盾しない局面 (相手の手札と山札を無作為に配分した `GameState`) を生成して用います。

```python
def choose_move(obs: pybl.Observation, num_samples: int = 1000) -> int:
    me = obs.player
    win = pybl.FIRST_WIN if me == pybl.FIRST else pybl.SECOND_WIN
    moves = obs.get_legal_moves()
    wins = [0] * len(moves)
    for i in range(num_samples):
        k = i % len(moves)
        sim = obs.sample_state(seed=i)
        sim.play(moves[k])
        wins[k] += sim.random_playout(seed=i) == win
    return moves[max(range(len(moves)), key=lambda k: wins[k])]
```

## 主なAPI

### GameState
| メソッド・プロパティ | 説明 |
|---|---|
| `GameState(seed=None)` / `reset(seed=None)` | 山札をシャッフルして7枚ずつ配り、先手をランダムに決める |
| `set_position(side_to_move, hands, flags, deck=(), owners=None, first_completers=None, first_player=None)` | 任意の局面を設定する。全てのカードが手札・フラッグ・山札のいずれかにちょうど1回ずつ現れる必要がある |
| `side_to_move`, `first_player`, `result`, `winner`, `is_terminal` | 手番、先手、対局結果、勝者、終局したか |
| `is_forced_termination`, `consecutive_pass_count` | 両者連続パスによる強制終局か、連続パス数 |
| `deck_count`, `board_cards`, `move_count` | 山札の枚数、盤面の全カード、undo 可能な手数 |
| `get_hand(p)`, `get_hand_count(p)`, `get_unseen_cards(p)` | 手札、手札の枚数、p から見えないカード (相手の手札 + 山札) |
| `get_flag_cards(i, p)`, `get_flag_owner(i)`, `get_flag_first_completer(i)`, `get_flag_strength(i, p)` | フラッグ i の配置カード、確保者、先に完成させたプレイヤー、フォーメーションの強さ |
| `get_claimed_flags(p)`, `get_placeable_flags(p)` | 確保済み・配置可能なフラッグ (ビットマスク) |
| `get_legal_moves()`, `is_legal(m)` | 合法手 (`numpy.ndarray[int16]`)。配置できなければ `[PASS_MOVE]`、終局後は空 |
| `play(m)` | 配置 → 判定 → 勝利条件の確認 → 補充 の順に1手進め、確保されたフラッグ (ビットマスク) を返す。非合法手は `ValueError` |
| `undo()` | 直前の `play` を取り消す |
| `determinize(p, seed=None)` | p から見えないカードを、相手の手札と山札に無作為に再配分する (undo の履歴は破棄される) |
| `random_playout(seed=None)` | 終局までランダムに着手し、結果を返す (この状態自体が終局まで進む) |
| `observe(player=None)` | player (省略時は手番プレイヤー) から見える情報のみを持つ `Observation` を返す |
| `copy()`, `copy_to(dest)` | 状態のコピー |

### Observation
`GameState.observe` で生成します。盤面に関するプロパティ・メソッド (`side_to_move`, `result`, `deck_count`, `board_cards`, `get_flag_cards`, `get_claimed_flags`, `get_placeable_flags` など) は `GameState` と同じ名前で利用できます。

| メソッド・プロパティ | 説明 |
|---|---|
| `player` | 観測しているプレイヤー |
| `hand`, `get_hand(player=None)` | 自分の手札。相手の手札を指定すると `ValueError` |
| `opponent_hand_count`, `get_hand_count(p)` | 相手の手札の枚数、各プレイヤーの手札の枚数 |
| `unseen_cards`, `get_unseen_cards()` | 見えないカード (相手の手札 + 山札) |
| `is_my_turn` | 自分の手番であり、かつ終局していないか |
| `get_legal_moves()`, `is_legal(m)` | 自分の合法手。相手の手番や終局後は空 |
| `sample_state(seed=None)` | 観測と矛盾しない `GameState` を生成する (見えないカードは無作為に配分) |
| `==` | 観測の比較。見えないカードの配分だけが異なる局面の観測は等しい |

### 関数
| 関数 | 説明 |
|---|---|
| `evaluate_formation(a, b, c)`, `formation_type(a, b, c)` | 3枚のフォーメーションの強さ・種類 |
| `best_completion(partial, available)`, `worst_completion(partial, available)` | partial (0 - 2枚) に available のカードを加えて作れる最強・最弱のフォーメーションの強さ。作れなければ `NULL_STRENGTH` |
| `judge_flag(first_cards, second_cards, board, turn_player, first_completer=None)` | フラッグの確保判定を行い、確保するプレイヤーを返す |
| `make_card`, `card_color`, `card_rank`, `card_to_str`, `parse_card_str` | カードの生成と変換 |
| `make_move`, `move_card`, `move_flag`, `move_to_str`, `parse_move_str` | 着手の生成と変換 |

## 基本的な型と表現

| 型 | 実体 | 説明 |
|---|---|---|
| `Card` | `numpy.int8` | `色 * 10 + (数字 - 1)` で 0 - 59。無効なカードは `NULL_CARD` |
| `CardColor` | `numpy.int8` | `RED`, `YELLOW`, `BLUE`, `GREEN`, `PURPLE`, `ORANGE` |
| `Player` | `numpy.int8` | `FIRST` (先手), `SECOND` (後手), `NULL_PLAYER` |
| `GameResult` | `numpy.int8` | `FIRST_WIN`, `SECOND_WIN`, `DRAW`, `NOT_OVER` |
| `FormationType` | `numpy.int8` | `HOST` < `SKIRMISHER` < `BATTALION` < `PHALANX` < `WEDGE` |
| `Move` | `numpy.int16` | `カード * 9 + フラッグ番号` で 0 - 539。パスは `PASS_MOVE` |

- カードの集合 (手札など) は、カード番号のビットを立てた64bit整数で表現します。`CardIterator` で各カードを列挙できます。
- フラッグの集合 (確保済みのフラッグなど) は、フラッグ番号 (0 - 8) のビットを立てた整数で表現します。
- フォーメーションの強さは `(フォーメーションの種類 << 5) | 合計` の整数で表現し、大小比較がそのまま強弱比較になります。
- 文字列表現では、カードは `"R5"`、着手は `"R5@3"` (フラッグ番号は1始まり) です。

## ルールの補足

- **両者連続パス時の強制終局**: 両プレイヤーが連続でパスした場合、それ以降状態が変化しないため、その時点で対局を終了します。確保したフラッグの数が多いプレイヤーを勝者とし、同数なら引き分けとします (`GameState.is_forced_termination` で判別できます)。
  - 実際の対局でこの状況になるのは、一方の手札と山札が尽き、もう一方が7枚の手札を持ったまま配置できず、残り1本のフラッグが証明で確保できない場合に限られます。このとき他の8本は4本ずつ確保されているため、結果は必ず引き分けになります。
- **(3) 引き分け確定について**: 一方のみフォーメーションが完成しているとき、盤面に出ていないカードは必ず7枚以上あるため、実際の対局で (3) が成立することはありません (成立には盤面に出ていないカードが全て同じ数字である必要があります)。判定自体はルール通り実装しており、`judge_flag` で任意の盤面について確認できます。
