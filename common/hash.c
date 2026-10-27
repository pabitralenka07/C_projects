#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

void hash_generate_salt(char *salt, size_t len) {
    if (!salt || len <= 1) return;

    static const char hex_chars[] = "0123456789abcdef";
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }

    size_t count = len - 1;
    if (count > HASH_SALT_LEN) count = HASH_SALT_LEN;

    for (size_t i = 0; i < count; i++) {
        salt[i] = hex_chars[rand() % 16];
    }
    salt[count] = '\0';
}

void hash_compute(const char *input, const char *salt, char *out, size_t out_len) {
    if (!input || !salt || !out || out_len <= HASH_HEX_LEN) return;

    /* Combine salt + input */
    char combined[512];
    snprintf(combined, sizeof(combined), "%s:%s", salt, input);

    /* 64-bit FNV-1a */
    uint64_t fnv = 14695981039346656037ULL;
    /* 64-bit DJB2 */
    uint64_t djb = 5381ULL;
    /* Murmur-inspired mixing accumulator */
    uint64_t mix1 = 0x9e3779b97f4a7c15ULL;
    uint64_t mix2 = 0xbf58476d1ce4e5b9ULL;

    for (size_t i = 0; combined[i] != '\0'; i++) {
        unsigned char c = (unsigned char)combined[i];
        fnv ^= c;
        fnv *= 1099511628211ULL;

        djb = ((djb << 5) + djb) + c;

        mix1 = (mix1 ^ (c + (mix1 << 6) + (mix1 >> 2)));
        mix2 = (mix2 ^ (c + (mix2 << 7) + (mix2 >> 3)));
    }

    /* Mix passes */
    fnv ^= (fnv >> 33); fnv *= 0xff51afd7ed558ccdULL; fnv ^= (fnv >> 33);
    djb ^= (djb >> 33); djb *= 0xc4ceb9fe1a85ec53ULL; djb ^= (djb >> 33);
    mix1 ^= (mix1 >> 31); mix1 *= 0xbf58476d1ce4e5b9ULL; mix1 ^= (mix1 >> 31);
    mix2 ^= (mix2 >> 32); mix2 *= 0x94d049bb133111ebULL; mix2 ^= (mix2 >> 32);

    snprintf(out, out_len, "%016llx%016llx%016llx%016llx",
             (unsigned long long)fnv,
             (unsigned long long)djb,
             (unsigned long long)mix1,
             (unsigned long long)mix2);
}

bool hash_verify(const char *input, const char *salt, const char *expected_hash) {
    if (!input || !salt || !expected_hash) return false;

    char calculated[HASH_HEX_LEN + 1];
    hash_compute(input, salt, calculated, sizeof(calculated));

    /* Constant-time string comparison */
    size_t len1 = strlen(calculated);
    size_t len2 = strlen(expected_hash);
    if (len1 != len2) return false;

    unsigned char diff = 0;
    for (size_t i = 0; i < len1; i++) {
        diff |= ((unsigned char)calculated[i] ^ (unsigned char)expected_hash[i]);
    }
    return diff == 0;
}
