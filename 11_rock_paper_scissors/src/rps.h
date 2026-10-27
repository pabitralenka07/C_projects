#ifndef RPS_H
#define RPS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define RPS_MAGIC       "RPSs"
#define RPS_DATA_FILE   "rps_stats.dat"

typedef enum {
    MOVE_ROCK = 0,
    MOVE_PAPER = 1,
    MOVE_SCISSORS = 2,
    MOVE_INVALID = -1
} RpsMove;

typedef enum {
    OUTCOME_DRAW = 0,
    OUTCOME_PLAYER_WIN = 1,
    OUTCOME_COMPUTER_WIN = 2
} RpsOutcome;

/* Injectable RNG signature: allows unit tests to supply deterministic computer moves */
typedef RpsMove (*RpsRngFunc)(void *ctx);

typedef struct {
    uint32_t games_played;
    uint32_t rounds_won;
    uint32_t rounds_lost;
    uint32_t rounds_drawn;
    uint32_t current_streak;
    uint32_t best_streak;
} RpsStats;

typedef struct {
    RpsStats stats;
    char     data_dir[256];
} RpsGame;

/* Lifecycle and Persistence */
void rps_game_init(RpsGame *game);
bool rps_game_load(RpsGame *game);
bool rps_game_save(RpsGame *game);
void rps_game_reset_stats(RpsGame *game);

/* Core Game Rules (Pure Logic) */
RpsOutcome rps_evaluate_round(RpsMove player, RpsMove computer);
const char *rps_move_name(RpsMove move);
RpsMove rps_parse_move(const char *str);

/* Round Execution with Injectable RNG */
RpsOutcome rps_play_single_round(RpsGame *game, RpsMove player_move,
                                 RpsRngFunc rng_func, void *rng_ctx,
                                 RpsMove *out_computer_move);

/* Match State for Best-of-N */
typedef struct {
    int total_rounds_target;  /* e.g., 3 or 5 */
    int player_wins;
    int computer_wins;
    int draws;
    int current_round;
} RpsMatch;

void rps_match_init(RpsMatch *match, int best_of_n);
bool rps_match_is_over(const RpsMatch *match);
int  rps_match_winner(const RpsMatch *match); /* 1: player, 2: computer, 0: in progress */

/* Production default RNG */
RpsMove rps_default_rng(void *ctx);

#endif /* RPS_H */
