#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "rps.h"
#include "safe_utils.h"
#include "file_ops.h"

/* Mock RNG contexts for deterministic testing */
typedef struct {
    RpsMove sequence[8];
    int     index;
    int     total;
} MockRngContext;

static RpsMove mock_cyclic_rng(void *ctx) {
    MockRngContext *m = (MockRngContext *)ctx;
    RpsMove move = m->sequence[m->index % m->total];
    m->index++;
    return move;
}

static void test_rules_and_evaluation(void) {
    /* Draws */
    assert(rps_evaluate_round(MOVE_ROCK, MOVE_ROCK) == OUTCOME_DRAW);
    assert(rps_evaluate_round(MOVE_PAPER, MOVE_PAPER) == OUTCOME_DRAW);
    assert(rps_evaluate_round(MOVE_SCISSORS, MOVE_SCISSORS) == OUTCOME_DRAW);

    /* Player wins */
    assert(rps_evaluate_round(MOVE_ROCK, MOVE_SCISSORS) == OUTCOME_PLAYER_WIN);
    assert(rps_evaluate_round(MOVE_PAPER, MOVE_ROCK) == OUTCOME_PLAYER_WIN);
    assert(rps_evaluate_round(MOVE_SCISSORS, MOVE_PAPER) == OUTCOME_PLAYER_WIN);

    /* Computer wins */
    assert(rps_evaluate_round(MOVE_ROCK, MOVE_PAPER) == OUTCOME_COMPUTER_WIN);
    assert(rps_evaluate_round(MOVE_PAPER, MOVE_SCISSORS) == OUTCOME_COMPUTER_WIN);
    assert(rps_evaluate_round(MOVE_SCISSORS, MOVE_ROCK) == OUTCOME_COMPUTER_WIN);

    /* Parsing */
    assert(rps_parse_move("r") == MOVE_ROCK);
    assert(rps_parse_move("Rock") == MOVE_ROCK);
    assert(rps_parse_move("1") == MOVE_ROCK);
    assert(rps_parse_move("p") == MOVE_PAPER);
    assert(rps_parse_move("2") == MOVE_PAPER);
    assert(rps_parse_move("s") == MOVE_SCISSORS);
    assert(rps_parse_move("3") == MOVE_SCISSORS);
    assert(rps_parse_move("invalid") == MOVE_INVALID);

    printf("  [PASS] test_rules_and_evaluation\n");
}

static void test_deterministic_rounds_and_streaks(void) {
    RpsGame game;
    rps_game_init(&game);

    /* Setup mock RNG to play: SCISSORS, ROCK, PAPER */
    MockRngContext ctx = {
        .sequence = { MOVE_SCISSORS, MOVE_ROCK, MOVE_PAPER },
        .index = 0,
        .total = 3
    };

    RpsMove comp_played;

    /* Round 1: Player plays ROCK vs Computer SCISSORS -> WIN */
    RpsOutcome o1 = rps_play_single_round(&game, MOVE_ROCK, mock_cyclic_rng, &ctx, &comp_played);
    assert(o1 == OUTCOME_PLAYER_WIN);
    assert(comp_played == MOVE_SCISSORS);
    assert(game.stats.rounds_won == 1);
    assert(game.stats.current_streak == 1);
    assert(game.stats.best_streak == 1);

    /* Round 2: Player plays PAPER vs Computer ROCK -> WIN */
    RpsOutcome o2 = rps_play_single_round(&game, MOVE_PAPER, mock_cyclic_rng, &ctx, &comp_played);
    assert(o2 == OUTCOME_PLAYER_WIN);
    assert(comp_played == MOVE_ROCK);
    assert(game.stats.rounds_won == 2);
    assert(game.stats.current_streak == 2);
    assert(game.stats.best_streak == 2);

    /* Round 3: Player plays ROCK vs Computer PAPER -> LOSE */
    RpsOutcome o3 = rps_play_single_round(&game, MOVE_ROCK, mock_cyclic_rng, &ctx, &comp_played);
    assert(o3 == OUTCOME_COMPUTER_WIN);
    assert(comp_played == MOVE_PAPER);
    assert(game.stats.rounds_lost == 1);
    assert(game.stats.current_streak == 0); /* Streak reset */
    assert(game.stats.best_streak == 2);    /* Best streak preserved */

    /* Round 4: Player plays ROCK vs Computer SCISSORS -> WIN */
    RpsOutcome o4 = rps_play_single_round(&game, MOVE_ROCK, mock_cyclic_rng, &ctx, &comp_played);
    assert(o4 == OUTCOME_PLAYER_WIN);
    assert(game.stats.current_streak == 1);
    assert(game.stats.best_streak == 2);

    printf("  [PASS] test_deterministic_rounds_and_streaks\n");
}

static void test_match_state(void) {
    RpsMatch match;
    rps_match_init(&match, 3);
    assert(!rps_match_is_over(&match));
    assert(rps_match_winner(&match) == 0);

    /* Player wins round 1 */
    match.player_wins++;
    assert(!rps_match_is_over(&match));

    /* Computer wins round 2 */
    match.computer_wins++;
    assert(!rps_match_is_over(&match));

    /* Player wins round 3 (2 wins total in Best of 3) */
    match.player_wins++;
    assert(rps_match_is_over(&match));
    assert(rps_match_winner(&match) == 1);

    printf("  [PASS] test_match_state\n");
}

static void test_file_integrity_and_recovery(void) {
    char templ[] = "/tmp/rps_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    setenv("DATA_DIR", dir, 1);

    RpsGame g1;
    rps_game_init(&g1);
    assert(rps_game_load(&g1));
    assert(g1.stats.games_played == 0);

    /* Update stats and save */
    g1.stats.games_played = 5;
    g1.stats.rounds_won = 12;
    g1.stats.rounds_lost = 6;
    g1.stats.best_streak = 4;
    assert(rps_game_save(&g1));

    /* Load into g2 */
    RpsGame g2;
    rps_game_init(&g2);
    assert(rps_game_load(&g2));
    assert(g2.stats.games_played == 5);
    assert(g2.stats.rounds_won == 12);
    assert(g2.stats.best_streak == 4);

    /* Second save creates .bak */
    g2.stats.games_played = 6;
    assert(rps_game_save(&g2));

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, RPS_DATA_FILE);

    /* Corrupt file */
    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    fputs("BAD_MAGIC_CORRUPT", fp);
    fclose(fp);

    /* Verify recovery from backup */
    assert(safe_recover_backup(path, RPS_MAGIC, sizeof(RpsStats)));

    RpsGame g3;
    rps_game_init(&g3);
    assert(rps_game_load(&g3));
    assert(g3.stats.games_played == 5); /* Restored to v1 */

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);

    printf("  [PASS] test_file_integrity_and_recovery\n");
}

int main(void) {
    printf("=== Running Rock Paper Scissors Unit Tests ===\n");
    test_rules_and_evaluation();
    test_deterministic_rounds_and_streaks();
    test_match_state();
    test_file_integrity_and_recovery();
    printf("All rock paper scissors unit tests passed successfully!\n");
    return 0;
}
