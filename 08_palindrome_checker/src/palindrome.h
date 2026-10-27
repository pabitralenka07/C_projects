#ifndef PALINDROME_H
#define PALINDROME_H

#include <stdbool.h>
#include <stddef.h>

/* Check if a string is a palindrome.
 * If ignore_case_and_punct is true: skips non-alphanumeric chars and ignores case.
 * If false: requires exact byte-for-byte match from front to back. */
bool palindrome_check_str(const char *str, bool ignore_case_and_punct);

/* Check if an integer is a palindrome using numeric arithmetic (no string conversion).
 * Negative numbers are considered non-palindromic due to the minus sign. */
bool palindrome_check_int(long long n);

/* Find the longest palindromic substring in `str`.
 * Writes result to `out` (up to `out_len` including NUL).
 * Returns length of the longest palindromic substring. */
size_t palindrome_longest_substring(const char *str, char *out, size_t out_len);

#endif /* PALINDROME_H */
