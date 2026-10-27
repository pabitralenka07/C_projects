# Project 16: Tic-Tac-Toe

A modern, highly modular implementation of Tic-Tac-Toe featuring two-player local pass & play, casual easy AI, and an unbeatable AI powered by the Minimax algorithm with alpha-beta pruning.

---

## Features

- **Game Modes**:
  1. **vs Unbeatable AI**: Uses recursive Minimax with alpha-beta pruning. It is mathematically impossible for a human player to defeat this AI (results in win for AI or draw).
  2. **vs Easy Casual AI**: Uses an injectable random choice engine for casual play.
  3. **Two-Player Pass & Play**: Local turn-based multiplayer.
- **Architectural Separation**: Pure game state, win evaluation, and Minimax search are completely decoupled in `src/game.c` from terminal input/output, enabling thorough unit testing.
- **Scoreboard**: Tracks game history (Player X wins, Player O wins, and Draws).

---

## Build & Run

```bash
# Compile
make

# Run unit tests (with ASan + UBSan) and integration tests
make test

# Launch interactively
./build/tictactoe
```
