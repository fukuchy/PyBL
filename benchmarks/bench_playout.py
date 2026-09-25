"""ランダムプレイアウトの速度を計測する"""
import argparse
import time

import pybl


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--num-games", type=int, default=100000)
    args = parser.parse_args()

    root = pybl.GameState(seed=0)
    state = pybl.GameState(seed=0)

    start = time.perf_counter()
    for i in range(args.num_games):
        root.reset(seed=i)
        root.copy_to(state)
        state.random_playout(seed=i)
    elapsed = time.perf_counter() - start

    print(f"{args.num_games} games in {elapsed:.3f} s ({args.num_games / elapsed:.0f} games/s)")


if __name__ == "__main__":
    main()
