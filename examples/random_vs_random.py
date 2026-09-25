"""ランダムプレイヤー同士で1局対戦し, 盤面を表示する"""
import random

import pybl


def main():
    rng = random.Random()
    state = pybl.GameState()
    print(state, end="\n\n")

    while not state.is_terminal:
        player = state.side_to_move
        moves = state.get_legal_moves()
        move = moves[rng.randrange(len(moves))]
        claimed = state.play(move)
        print(f"{pybl.player_to_str(player)}: {pybl.move_to_str(move)}", end="")
        if claimed:
            flags = [str(i + 1) for i in range(pybl.NUM_FLAGS) if claimed >> i & 1]
            print(f"  (claimed flag {', '.join(flags)})", end="")
        print()

    print()
    print(state)
    print(f"result: {pybl.result_to_str(state.result)}")


if __name__ == "__main__":
    main()
