#ifndef COMMON_HASH_H
#define COMMON_HASH_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Educational salted hash module for administrative PINs and logins.
 * NOTE: For production cryptographic security, industry algorithms such as
 * Argon2, scrypt, or bcrypt should be used. This implementation provides
 * an educational demonstration of salting and hashing principles without
 * external library dependencies.
 */

#define HASH_SALT_LEN 16
#define HASH_HEX_LEN  64

/* Generate a random hex salt string (null-terminated). */
void hash_generate_salt(char *salt, size_t len);

/* Compute salted hash: output is a null-terminated hex string. */
void hash_compute(const char *input, const char *salt, char *out, size_t out_len);

/* Verify that input matches expected hash when salted with given salt. */
bool hash_verify(const char *input, const char *salt, const char *expected_hash);

#endif /* COMMON_HASH_H */
