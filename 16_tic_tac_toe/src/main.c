#include "game.h"
#include "safe_utils.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void print_board(const Board *board) {
    printf("\n");
    printf("       1   2   3\n");
    printf("     +---+---+---+\n");
    for (int r = 0; r < BOARD_SIZE; r++) {
        printf("   %d | %c | %c | %c |\n",
               r + 1,
               cell_to_char(board->cells[r][0]),
               cell_to_char(board->cells[r][1]),
               cell_to_char(board->cells[r][2]));
        printf("     +---+---+---+\n");
    }
    printf("\n");
}

static bool prompt_human_move(const Board *board, Cell player, int *out_row, int *out_col) {
    char p_char = cell_to_char(player);

    for (;;) {
        printf("Player '%c' - Enter row [1-3]: ", p_char);
        int r = 0;
        if (!safe_read_int("", &r, 1, 3)) return false;

        printf("Player '%c' - Enter column [1-3]: ", p_char);
        int c = 0;
        if (!safe_read_int("", &c, 1, 3)) return false;

        if (board_is_valid_move(board, r - 1, c - 1)) {
            *out_row = r - 1;
            *out_col = c - 1;
            return true;
        }

        printf("  Cell (%d, %d) is already occupied! Please choose an empty cell.\n", r, c);
    }
}

static void play_game(Scoreboard *score, bool vs_computer, AiDifficulty difficulty) {
    Board board;
    board_init(&board);

    Cell current_player = CELL_X;
    Cell human = CELL_X;
    Cell ai = CELL_O;

    printf("\n=== New Tic-Tac-Toe Game ===\n");
    if (vs_computer) {
        printf("Mode: Human (X) vs Computer (O) [%s]\n",
               difficulty == AI_UNBEATABLE ? "UNBEATABLE MINIMAX" : "EASY CASUAL");
    } else {
        printf("Mode: Two-Player Pass & Play (X vs O)\n");
    }

    while (board_check_status(&board) == GAME_IN_PROGRESS) {
        print_board(&board);

        int r = -1, c = -1;

        if (vs_computer && current_player == ai) {
            printf("Computer (O) is thinking...\n");
            Move ai_move;
            if (difficulty == AI_UNBEATABLE) {
                ai_move = ai_get_unbeatable_move(&board, ai);
            } else {
                ai_move = ai_get_easy_move(&board, NULL, NULL);
            }
            r = ai_move.row;
            c = ai_move.col;
            printf("Computer played row %d, column %d.\n", r + 1, c + 1);
        } else {
            if (!prompt_human_move(&board, current_player, &r, &c)) {
                printf("\nGame aborted due to EOF.\n");
                return;
            }
        }

        board_make_move(&board, r, c, current_player);
        current_player = (current_player == CELL_X) ? CELL_O : CELL_X;
    }

    print_board(&board);
    GameStatus status = board_check_status(&board);
    scoreboard_record_result(score, status);

    printf("===============================================\n");
    if (status == GAME_WIN_X) {
        if (vs_computer && human == CELL_X) {
            printf("  🎉 VICTORY! You defeated the computer!\n");
        } else {
            printf("  🎉 Player 'X' WINS the game!\n");
        }
    } else if (status == GAME_WIN_O) {
        if (vs_computer && ai == CELL_O) {
            printf("  🤖 Computer (O) WINS! Better luck next time.\n");
        } else {
            printf("  🎉 Player 'O' WINS the game!\n");
        }
    } else {
        printf("  🤝 It's a DRAW (Cat's Game)!\n");
    }
    printf("===============================================\n");
}

static void show_scoreboard(const Scoreboard *score) {
    uint32_t total = score->x_wins + score->o_wins + score->draws;
    printf("\n===============================================\n");
    printf("               Tic-Tac-Toe Scoreboard          \n");
    printf("===============================================\n");
    printf("  Total Games Played: %u\n", total);
    printf("  Player 'X' Wins:    %u\n", score->x_wins);
    printf("  Player 'O' Wins:    %u\n", score->o_wins);
    printf("  Draws / Ties:       %u\n", score->draws);
    printf("===============================================\n");
}

int main(void) {
    srand((unsigned int)time(NULL));
    Scoreboard score;
    scoreboard_init(&score);

    bool running = true;
    while (running) {
        safe_clear_screen();
        printf("===============================================\n");
        printf("                 Tic-Tac-Toe                   \n");
        printf("===============================================\n");
        printf("  1. vs Computer (Unbeatable AI - Minimax)\n");
        printf("  2. vs Computer (Easy Casual AI)\n");
        printf("  3. Two-Player Pass & Play\n");
        printf("  4. View Scoreboard\n");
        printf("  5. Reset Scoreboard\n");
        printf("  6. Exit\n");
        printf("===============================================\n");

        int choice = safe_menu_choice(1, 6);
        if (choice == -1 || choice == 6) {
            break;
        }

        switch (choice) {
            case 1: play_game(&score, true, AI_UNBEATABLE); break;
            case 2: play_game(&score, true, AI_EASY); break;
            case 3: play_game(&score, false, AI_EASY); break;
            case 4: show_scoreboard(&score); break;
            case 5: {
                if (safe_confirm("Reset scoreboard statistics to zero?")) {
                    scoreboard_init(&score);
                    printf("  Scoreboard reset.\n");
                }
                break;
            }
            default: break;
        }

        util_press_enter();
    }

    printf("\nThank you for playing Tic-Tac-Toe. Goodbye!\n");
    return 0;
}
