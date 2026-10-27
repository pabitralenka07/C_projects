#include "game.h"
#include <string.h>
#include <stdlib.h>

void board_init(Board *board) {
    if (!board) return;
    memset(board, 0, sizeof(Board));
}

bool board_is_valid_move(const Board *board, int row, int col) {
    if (!board) return false;
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return false;
    }
    return (board->cells[row][col] == CELL_EMPTY);
}

bool board_make_move(Board *board, int row, int col, Cell player) {
    if (!board_is_valid_move(board, row, col)) return false;
    board->cells[row][col] = player;
    board->move_count++;
    return true;
}

bool board_is_full(const Board *board) {
    if (!board) return true;
    return (board->move_count >= (BOARD_SIZE * BOARD_SIZE));
}

GameStatus board_check_status(const Board *board) {
    if (!board) return GAME_DRAW;

    /* Check Rows */
    for (int r = 0; r < BOARD_SIZE; r++) {
        if (board->cells[r][0] != CELL_EMPTY &&
            board->cells[r][0] == board->cells[r][1] &&
            board->cells[r][1] == board->cells[r][2]) {
            return (board->cells[r][0] == CELL_X) ? GAME_WIN_X : GAME_WIN_O;
        }
    }

    /* Check Columns */
    for (int c = 0; c < BOARD_SIZE; c++) {
        if (board->cells[0][c] != CELL_EMPTY &&
            board->cells[0][c] == board->cells[1][c] &&
            board->cells[1][c] == board->cells[2][c]) {
            return (board->cells[0][c] == CELL_X) ? GAME_WIN_X : GAME_WIN_O;
        }
    }

    /* Check Diagonals */
    if (board->cells[0][0] != CELL_EMPTY &&
        board->cells[0][0] == board->cells[1][1] &&
        board->cells[1][1] == board->cells[2][2]) {
        return (board->cells[0][0] == CELL_X) ? GAME_WIN_X : GAME_WIN_O;
    }

    if (board->cells[0][2] != CELL_EMPTY &&
        board->cells[0][2] == board->cells[1][1] &&
        board->cells[1][1] == board->cells[2][0]) {
        return (board->cells[0][2] == CELL_X) ? GAME_WIN_X : GAME_WIN_O;
    }

    /* Check Draw */
    if (board_is_full(board)) {
        return GAME_DRAW;
    }

    return GAME_IN_PROGRESS;
}

char cell_to_char(Cell cell) {
    switch (cell) {
        case CELL_X: return 'X';
        case CELL_O: return 'O';
        default:     return ' ';
    }
}

/* ------------------------------------------------------------------ */
/*  AI Implementations                                                */
/* ------------------------------------------------------------------ */

static int default_game_rng(int max_val, void *ctx) {
    (void)ctx;
    if (max_val <= 1) return 0;
    return rand() % max_val;
}

Move ai_get_easy_move(const Board *board, GameRngFunc rng, void *rng_ctx) {
    Move moves[BOARD_SIZE * BOARD_SIZE];
    int count = 0;

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board->cells[r][c] == CELL_EMPTY) {
                moves[count].row = r;
                moves[count].col = c;
                count++;
            }
        }
    }

    if (count == 0) {
        Move fallback = {-1, -1};
        return fallback;
    }

    GameRngFunc pick_rng = rng ? rng : default_game_rng;
    int choice = pick_rng(count, rng_ctx);
    if (choice < 0 || choice >= count) choice = 0;

    return moves[choice];
}

/* Recursive Minimax with Alpha-Beta Pruning */
static int minimax(Board *board, int depth, bool is_maximizing, Cell ai_player, int alpha, int beta) {
    GameStatus status = board_check_status(board);
    Cell human_player = (ai_player == CELL_X) ? CELL_O : CELL_X;
    GameStatus ai_win_status = (ai_player == CELL_X) ? GAME_WIN_X : GAME_WIN_O;
    GameStatus human_win_status = (human_player == CELL_X) ? GAME_WIN_X : GAME_WIN_O;

    if (status == ai_win_status) {
        return 10 - depth; /* Prefer faster wins */
    }
    if (status == human_win_status) {
        return depth - 10; /* Prefer delayed losses */
    }
    if (status == GAME_DRAW) {
        return 0;
    }

    if (is_maximizing) {
        int best = -1000;
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (board->cells[r][c] == CELL_EMPTY) {
                    board->cells[r][c] = ai_player;
                    board->move_count++;

                    int score = minimax(board, depth + 1, false, ai_player, alpha, beta);

                    board->cells[r][c] = CELL_EMPTY;
                    board->move_count--;

                    if (score > best) best = score;
                    if (score > alpha) alpha = score;
                    if (beta <= alpha) break;
                }
            }
            if (beta <= alpha) break;
        }
        return best;
    } else {
        int best = 1000;
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (board->cells[r][c] == CELL_EMPTY) {
                    board->cells[r][c] = human_player;
                    board->move_count++;

                    int score = minimax(board, depth + 1, true, ai_player, alpha, beta);

                    board->cells[r][c] = CELL_EMPTY;
                    board->move_count--;

                    if (score < best) best = score;
                    if (score < beta) beta = score;
                    if (beta <= alpha) break;
                }
            }
            if (beta <= alpha) break;
        }
        return best;
    }
}

Move ai_get_unbeatable_move(Board *board, Cell ai_player) {
    Move best_move = {-1, -1};
    int best_score = -1000;

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board->cells[r][c] == CELL_EMPTY) {
                board->cells[r][c] = ai_player;
                board->move_count++;

                int move_score = minimax(board, 0, false, ai_player, -1000, 1000);

                board->cells[r][c] = CELL_EMPTY;
                board->move_count--;

                if (move_score > best_score) {
                    best_score = move_score;
                    best_move.row = r;
                    best_move.col = c;
                }
            }
        }
    }

    return best_move;
}

/* ------------------------------------------------------------------ */
/*  Scoreboard                                                        */
/* ------------------------------------------------------------------ */

void scoreboard_init(Scoreboard *score) {
    if (!score) return;
    memset(score, 0, sizeof(Scoreboard));
}

void scoreboard_record_result(Scoreboard *score, GameStatus status) {
    if (!score) return;
    if (status == GAME_WIN_X) {
        score->x_wins++;
    } else if (status == GAME_WIN_O) {
        score->o_wins++;
    } else if (status == GAME_DRAW) {
        score->draws++;
    }
}
