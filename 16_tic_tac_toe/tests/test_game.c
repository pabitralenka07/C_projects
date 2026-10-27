#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static void test_win_and_draw_conditions(void) {
    Board b;

    /* Row win */
    board_init(&b);
    board_make_move(&b, 0, 0, CELL_X);
    board_make_move(&b, 0, 1, CELL_X);
    board_make_move(&b, 0, 2, CELL_X);
    assert(board_check_status(&b) == GAME_WIN_X);

    /* Column win */
    board_init(&b);
    board_make_move(&b, 0, 1, CELL_O);
    board_make_move(&b, 1, 1, CELL_O);
    board_make_move(&b, 2, 1, CELL_O);
    assert(board_check_status(&b) == GAME_WIN_O);

    /* Diagonal win */
    board_init(&b);
    board_make_move(&b, 0, 0, CELL_X);
    board_make_move(&b, 1, 1, CELL_X);
    board_make_move(&b, 2, 2, CELL_X);
    assert(board_check_status(&b) == GAME_WIN_X);

    /* Anti-diagonal win */
    board_init(&b);
    board_make_move(&b, 0, 2, CELL_O);
    board_make_move(&b, 1, 1, CELL_O);
    board_make_move(&b, 2, 0, CELL_O);
    assert(board_check_status(&b) == GAME_WIN_O);

    /* Draw */
    /*
     X O X
     X O O
     O X X
    */
    board_init(&b);
    board_make_move(&b, 0, 0, CELL_X);
    board_make_move(&b, 0, 1, CELL_O);
    board_make_move(&b, 0, 2, CELL_X);

    board_make_move(&b, 1, 0, CELL_X);
    board_make_move(&b, 1, 1, CELL_O);
    board_make_move(&b, 1, 2, CELL_O);

    board_make_move(&b, 2, 0, CELL_O);
    board_make_move(&b, 2, 1, CELL_X);
    board_make_move(&b, 2, 2, CELL_X);
    assert(board_check_status(&b) == GAME_DRAW);

    printf("  [PASS] test_win_and_draw_conditions\n");
}

static int mock_first_cell_rng(int max_val, void *ctx) {
    (void)max_val;
    (void)ctx;
    return 0; /* Always pick first available cell */
}

static void test_easy_ai_with_mock_rng(void) {
    Board b;
    board_init(&b);

    board_make_move(&b, 0, 0, CELL_X);
    board_make_move(&b, 0, 1, CELL_O);

    Move m = ai_get_easy_move(&b, mock_first_cell_rng, NULL);
    /* First available cell after (0,0) and (0,1) is (0,2) */
    assert(m.row == 0 && m.col == 2);

    printf("  [PASS] test_easy_ai_with_mock_rng\n");
}

static void test_minimax_winning_and_blocking(void) {
    Board b;

    /* 1. Immediate Winning Move */
    /*
       O | O | .
       X | X | .
       . | . | .
       AI is 'O'. AI must play (0, 2) to win immediately!
    */
    board_init(&b);
    board_make_move(&b, 0, 0, CELL_O);
    board_make_move(&b, 0, 1, CELL_O);
    board_make_move(&b, 1, 0, CELL_X);
    board_make_move(&b, 1, 1, CELL_X);

    Move win_move = ai_get_unbeatable_move(&b, CELL_O);
    assert(win_move.row == 0 && win_move.col == 2);

    /* 2. Immediate Blocking Move */
    /*
       X | X | .
       O | . | .
       . | . | .
       AI is 'O'. Human is 'X'. AI must play (0, 2) to block human!
    */
    board_init(&b);
    board_make_move(&b, 0, 0, CELL_X);
    board_make_move(&b, 0, 1, CELL_X);
    board_make_move(&b, 1, 0, CELL_O);

    Move block_move = ai_get_unbeatable_move(&b, CELL_O);
    assert(block_move.row == 0 && block_move.col == 2);

    printf("  [PASS] test_minimax_winning_and_blocking\n");
}

static void test_minimax_never_loses(void) {
    /*
     * Simulate games where AI (O) plays second against random moves from X.
     * The unbeatable Minimax algorithm must NEVER produce GAME_WIN_X!
     */
    for (int game = 0; game < 50; game++) {
        Board b;
        board_init(&b);
        Cell current = CELL_X;

        while (board_check_status(&b) == GAME_IN_PROGRESS) {
            if (current == CELL_X) {
                /* Random valid human move */
                Move m = ai_get_easy_move(&b, NULL, NULL);
                board_make_move(&b, m.row, m.col, CELL_X);
                current = CELL_O;
            } else {
                /* Unbeatable AI move */
                Move m = ai_get_unbeatable_move(&b, CELL_O);
                assert(board_is_valid_move(&b, m.row, m.col));
                board_make_move(&b, m.row, m.col, CELL_O);
                current = CELL_X;
            }
        }

        GameStatus status = board_check_status(&b);
        /* Minimax AI must NEVER lose */
        assert(status == GAME_WIN_O || status == GAME_DRAW);
    }

    printf("  [PASS] test_minimax_never_loses (50 automated games verified)\n");
}

static void test_scoreboard(void) {
    Scoreboard s;
    scoreboard_init(&s);

    scoreboard_record_result(&s, GAME_WIN_X);
    scoreboard_record_result(&s, GAME_WIN_O);
    scoreboard_record_result(&s, GAME_DRAW);

    assert(s.x_wins == 1);
    assert(s.o_wins == 1);
    assert(s.draws == 1);

    printf("  [PASS] test_scoreboard\n");
}

int main(void) {
    printf("=== Running Tic-Tac-Toe Unit Tests ===\n");
    test_win_and_draw_conditions();
    test_easy_ai_with_mock_rng();
    test_minimax_winning_and_blocking();
    test_minimax_never_loses();
    test_scoreboard();
    printf("All tic-tac-toe unit tests passed successfully!\n");
    return 0;
}
