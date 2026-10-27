#include "rps.h"
#include "safe_utils.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static void play_match(RpsGame *game, int best_of_n) {
    RpsMatch match;
    rps_match_init(&match, best_of_n);

    printf("\n=== Starting Best-of-%d Match ===\n", match.total_rounds_target);
    printf("First to %d round wins takes the match!\n\n", (match.total_rounds_target / 2) + 1);

    while (!rps_match_is_over(&match)) {
        printf("--- Round %d (Score: You %d - %d Computer | Draws: %d) ---\n",
               match.current_round, match.player_wins, match.computer_wins, match.draws);
        printf("  1. Rock (r)\n");
        printf("  2. Paper (p)\n");
        printf("  3. Scissors (s)\n");

        char input[16];
        if (!safe_read_string("Choose your move: ", input, sizeof(input), false)) {
            printf("\nMatch aborted due to EOF.\n");
            return;
        }

        RpsMove p_move = rps_parse_move(input);
        if (p_move == MOVE_INVALID) {
            printf("  Invalid move! Enter 1/r, 2/p, or 3/s.\n\n");
            continue;
        }

        RpsMove c_move = MOVE_ROCK;
        RpsOutcome outcome = rps_play_single_round(game, p_move, rps_default_rng, NULL, &c_move);

        printf("\n  You played:      %s\n", rps_move_name(p_move));
        printf("  Computer played: %s\n", rps_move_name(c_move));

        if (outcome == OUTCOME_PLAYER_WIN) {
            printf("  => Result: YOU WIN this round! 🎉\n\n");
            match.player_wins++;
        } else if (outcome == OUTCOME_COMPUTER_WIN) {
            printf("  => Result: COMPUTER WINS this round. 🤖\n\n");
            match.computer_wins++;
        } else {
            printf("  => Result: It's a DRAW! 🤝\n\n");
            match.draws++;
        }

        match.current_round++;
    }

    int winner = rps_match_winner(&match);
    printf("===============================================\n");
    if (winner == 1) {
        printf("  🏆 MATCH VICTORY! You won the Best-of-%d match (%d to %d)!\n",
               match.total_rounds_target, match.player_wins, match.computer_wins);
    } else {
        printf("  💀 MATCH DEFEAT. Computer won the Best-of-%d match (%d to %d).\n",
               match.total_rounds_target, match.computer_wins, match.player_wins);
    }
    printf("===============================================\n");

    game->stats.games_played++;
    rps_game_save(game);
}

static void show_stats(const RpsGame *game) {
    const RpsStats *s = &game->stats;
    uint32_t total_rounds = s->rounds_won + s->rounds_lost + s->rounds_drawn;
    double win_rate = total_rounds > 0 ? ((double)s->rounds_won / total_rounds) * 100.0 : 0.0;

    printf("\n===============================================\n");
    printf("            RPS Lifetime Statistics            \n");
    printf("===============================================\n");
    printf("  Matches Completed:   %u\n", s->games_played);
    printf("  Total Rounds Played: %u\n", total_rounds);
    printf("  Rounds Won:          %u\n", s->rounds_won);
    printf("  Rounds Lost:         %u\n", s->rounds_lost);
    printf("  Rounds Drawn:        %u\n", s->rounds_drawn);
    printf("  Win Rate:            %.1f%%\n", win_rate);
    printf("  Current Win Streak:  %u\n", s->current_streak);
    printf("  Best Win Streak:     %u\n", s->best_streak);
    printf("===============================================\n");
}

int main(void) {
    RpsGame game;
    rps_game_init(&game);
    rps_game_load(&game);

    bool running = true;
    while (running) {
        safe_clear_screen();
        printf("===============================================\n");
        printf("             Rock Paper Scissors               \n");
        printf("===============================================\n");
        printf("  1. Quick Match (1 Round)\n");
        printf("  2. Best-of-3 Match\n");
        printf("  3. Best-of-5 Match\n");
        printf("  4. Custom Best-of-N Match\n");
        printf("  5. View Statistics & Streaks\n");
        printf("  6. Reset Statistics\n");
        printf("  7. Save & Exit\n");
        printf("===============================================\n");

        int choice = safe_menu_choice(1, 7);
        if (choice == -1 || choice == 7) {
            break;
        }

        switch (choice) {
            case 1: play_match(&game, 1); break;
            case 2: play_match(&game, 3); break;
            case 3: play_match(&game, 5); break;
            case 4: {
                int n = 3;
                if (safe_read_int("Enter odd number of rounds (1-99): ", &n, 1, 99)) {
                    play_match(&game, n);
                }
                break;
            }
            case 5: show_stats(&game); break;
            case 6: {
                if (safe_confirm("Are you sure you want to reset all lifetime statistics?")) {
                    rps_game_reset_stats(&game);
                    rps_game_save(&game);
                    printf("  Statistics reset to zero.\n");
                }
                break;
            }
            default: break;
        }

        util_press_enter();
    }

    rps_game_save(&game);
    printf("\nThank you for playing Rock Paper Scissors. Goodbye!\n");
    return 0;
}
