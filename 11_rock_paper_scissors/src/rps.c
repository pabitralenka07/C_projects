#include "rps.h"
#include "safe_utils.h"
#include "file_ops.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/*  Lifecycle & Persistence                                           */
/* ------------------------------------------------------------------ */

void rps_game_init(RpsGame *game) {
    if (!game) return;
    memset(game, 0, sizeof(RpsGame));

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(game->data_dir, sizeof(game->data_dir), "%s", env);
    } else {
        snprintf(game->data_dir, sizeof(game->data_dir), "./data");
    }

    /* Seed RNG (support RPS_SEED env var for deterministic tests) */
    static bool seeded = false;
    const char *seed_env = getenv("RPS_SEED");
    if (seed_env && seed_env[0] != '\0') {
        srand((unsigned int)strtoul(seed_env, NULL, 10));
        seeded = true;
    } else if (!seeded) {
        srand((unsigned int)(time(NULL) ^ (uintptr_t)getpid()));
        seeded = true;
    }
}

void rps_game_reset_stats(RpsGame *game) {
    if (!game) return;
    memset(&game->stats, 0, sizeof(RpsStats));
}

bool rps_game_load(RpsGame *game) {
    if (!game) return false;
    if (!util_ensure_dir(game->data_dir)) {
        return false;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", game->data_dir, RPS_DATA_FILE);

    void *data = NULL;
    uint32_t count = 0, next_id = 1;
    int res = file_validated_load(path, RPS_MAGIC, &data, sizeof(RpsStats), &count, &next_id);

    if (res == FILE_LOAD_CORRUPT) {
        printf("\n  WARNING: RPS statistics data file is corrupted!\n");
        if (safe_confirm("  Attempt recovery from backup?")) {
            if (safe_recover_backup(path, RPS_MAGIC, sizeof(RpsStats))) {
                res = file_validated_load(path, RPS_MAGIC, &data, sizeof(RpsStats), &count, &next_id);
                if (res == FILE_LOAD_OK) {
                    printf("  Recovery successful — statistics restored.\n");
                }
            }
        }
        if (res != FILE_LOAD_OK) {
            printf("  Starting with fresh game statistics.\n");
            data = NULL;
            memset(&game->stats, 0, sizeof(RpsStats));
        }
    } else if (res == FILE_LOAD_FRESH) {
        memset(&game->stats, 0, sizeof(RpsStats));
    }

    if (res == FILE_LOAD_OK && data != NULL && count == 1) {
        memcpy(&game->stats, data, sizeof(RpsStats));
        free(data);
    }

    return true;
}

bool rps_game_save(RpsGame *game) {
    if (!game) return false;
    if (!util_ensure_dir(game->data_dir)) return false;

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", game->data_dir, RPS_DATA_FILE);

    /* Zero struct before saving to ensure zero padding leaks */
    RpsStats to_save;
    memset(&to_save, 0, sizeof(to_save));
    memcpy(&to_save, &game->stats, sizeof(RpsStats));

    return safe_atomic_save(path, RPS_MAGIC, &to_save, sizeof(RpsStats), 1, 1);
}

/* ------------------------------------------------------------------ */
/*  Pure Game Evaluation Rules                                        */
/* ------------------------------------------------------------------ */

RpsOutcome rps_evaluate_round(RpsMove player, RpsMove computer) {
    if (player == computer) {
        return OUTCOME_DRAW;
    }

    if ((player == MOVE_ROCK && computer == MOVE_SCISSORS) ||
        (player == MOVE_PAPER && computer == MOVE_ROCK) ||
        (player == MOVE_SCISSORS && computer == MOVE_PAPER)) {
        return OUTCOME_PLAYER_WIN;
    }

    return OUTCOME_COMPUTER_WIN;
}

const char *rps_move_name(RpsMove move) {
    switch (move) {
        case MOVE_ROCK:     return "Rock";
        case MOVE_PAPER:    return "Paper";
        case MOVE_SCISSORS: return "Scissors";
        default:            return "Unknown";
    }
}

RpsMove rps_parse_move(const char *str) {
    if (!str) return MOVE_INVALID;
    while (*str != '\0' && isspace((unsigned char)*str)) str++;

    char first = (char)tolower((unsigned char)str[0]);
    if (first == 'r' || strcmp(str, "1") == 0) return MOVE_ROCK;
    if (first == 'p' || strcmp(str, "2") == 0) return MOVE_PAPER;
    if (first == 's' || strcmp(str, "3") == 0) return MOVE_SCISSORS;

    return MOVE_INVALID;
}

RpsMove rps_default_rng(void *ctx) {
    (void)ctx;
    return (RpsMove)(rand() % 3);
}

RpsOutcome rps_play_single_round(RpsGame *game, RpsMove player_move,
                                 RpsRngFunc rng_func, void *rng_ctx,
                                 RpsMove *out_computer_move) {
    RpsRngFunc rng = rng_func ? rng_func : rps_default_rng;
    RpsMove comp_move = rng(rng_ctx);

    if (comp_move < MOVE_ROCK || comp_move > MOVE_SCISSORS) {
        comp_move = MOVE_ROCK;
    }

    if (out_computer_move) {
        *out_computer_move = comp_move;
    }

    RpsOutcome outcome = rps_evaluate_round(player_move, comp_move);

    if (game) {
        if (outcome == OUTCOME_PLAYER_WIN) {
            game->stats.rounds_won++;
            game->stats.current_streak++;
            if (game->stats.current_streak > game->stats.best_streak) {
                game->stats.best_streak = game->stats.current_streak;
            }
        } else if (outcome == OUTCOME_COMPUTER_WIN) {
            game->stats.rounds_lost++;
            game->stats.current_streak = 0;
        } else {
            game->stats.rounds_drawn++;
            game->stats.current_streak = 0;
        }
    }

    return outcome;
}

/* ------------------------------------------------------------------ */
/*  Best-of-N Match State                                             */
/* ------------------------------------------------------------------ */

void rps_match_init(RpsMatch *match, int best_of_n) {
    if (!match) return;
    memset(match, 0, sizeof(RpsMatch));
    if (best_of_n < 1) best_of_n = 1;
    if (best_of_n % 2 == 0) best_of_n += 1; /* Best of N is always odd */
    match->total_rounds_target = best_of_n;
    match->current_round = 1;
}

bool rps_match_is_over(const RpsMatch *match) {
    if (!match) return true;
    int target = (match->total_rounds_target / 2) + 1;
    return (match->player_wins >= target || match->computer_wins >= target);
}

int rps_match_winner(const RpsMatch *match) {
    if (!match) return 0;
    int target = (match->total_rounds_target / 2) + 1;
    if (match->player_wins >= target) return 1;
    if (match->computer_wins >= target) return 2;
    return 0;
}
