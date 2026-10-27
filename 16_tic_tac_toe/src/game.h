#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define BOARD_SIZE 3

typedef enum {
    CELL_EMPTY = 0,
    CELL_X     = 1,
    CELL_O     = 2
} Cell;

typedef enum {
    GAME_IN_PROGRESS = 0,
    GAME_WIN_X       = 1,
    GAME_WIN_O       = 2,
    GAME_DRAW        = 3
} GameStatus;

typedef enum {
    AI_EASY       = 1,  /* Random valid move */
    AI_UNBEATABLE = 2   /* Minimax optimal solver */
} AiDifficulty;

/* Injectable RNG signature for easy AI: returns a number in [0, max_val - 1] */
typedef int (*GameRngFunc)(int max_val, void *ctx);

typedef struct {
    Cell cells[BOARD_SIZE][BOARD_SIZE];
    int  move_count;
} Board;

typedef struct {
    int row;
    int col;
} Move;

typedef struct {
    uint32_t x_wins;
    uint32_t o_wins;
    uint32_t draws;
} Scoreboard;

/* Board Operations (Pure Logic) */
void board_init(Board *board);
bool board_is_valid_move(const Board *board, int row, int col);
bool board_make_move(Board *board, int row, int col, Cell player);
GameStatus board_check_status(const Board *board);
bool board_is_full(const Board *board);

/* AI Moves (Decoupled, Unit-Testable) */
Move ai_get_easy_move(const Board *board, GameRngFunc rng, void *rng_ctx);
Move ai_get_unbeatable_move(Board *board, Cell ai_player);

/* Scoreboard */
void scoreboard_init(Scoreboard *score);
void scoreboard_record_result(Scoreboard *score, GameStatus status);

/* Display */
char cell_to_char(Cell cell);

#endif /* GAME_H */
