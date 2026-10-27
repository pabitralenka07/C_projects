#include "palindrome.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

bool palindrome_check_str(const char *str, bool ignore_case_and_punct) {
    if (!str) return false;

    size_t len = strlen(str);
    if (len <= 1) return true;

    if (!ignore_case_and_punct) {
        size_t left = 0;
        size_t right = len - 1;
        while (left < right) {
            if (str[left] != str[right]) {
                return false;
            }
            left++;
            right--;
        }
        return true;
    }

    /* Relaxed mode: ignore punctuation, spaces, and case */
    const char *left = str;
    const char *right = str + len - 1;

    while (left < right) {
        while (left < right && !isalnum((unsigned char)*left)) {
            left++;
        }
        while (left < right && !isalnum((unsigned char)*right)) {
            right--;
        }

        if (left < right) {
            if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
                return false;
            }
            left++;
            right--;
        }
    }

    return true;
}

bool palindrome_check_int(long long n) {
    /* Negative numbers are not palindromic (e.g., -121 != 121-) */
    if (n < 0) return false;
    if (n < 10) return true;
    if (n % 10 == 0) return false;

    long long original = n;
    long long reversed = 0;

    while (n > 0) {
        int digit = (int)(n % 10);
        reversed = reversed * 10 + digit;
        n /= 10;
    }

    return original == reversed;
}

static void expand_center(const char *str, size_t len, int left, int right,
                          int *start_out, int *len_out) {
    while (left >= 0 && (size_t)right < len && str[left] == str[right]) {
        left--;
        right++;
    }
    int current_len = right - left - 1;
    if (current_len > *len_out) {
        *start_out = left + 1;
        *len_out = current_len;
    }
}

size_t palindrome_longest_substring(const char *str, char *out, size_t out_len) {
    if (!str || !out || out_len == 0) return 0;

    size_t n = strlen(str);
    if (n == 0) {
        out[0] = '\0';
        return 0;
    }

    int best_start = 0;
    int best_len = 1;

    for (size_t i = 0; i < n; i++) {
        /* Odd length centers (centered at character i) */
        expand_center(str, n, (int)i, (int)i, &best_start, &best_len);
        /* Even length centers (centered between i and i+1) */
        expand_center(str, n, (int)i, (int)i + 1, &best_start, &best_len);
    }

    size_t copy_len = (size_t)best_len;
    if (copy_len >= out_len) {
        copy_len = out_len - 1;
    }

    memcpy(out, str + best_start, copy_len);
    out[copy_len] = '\0';

    return (size_t)best_len;
}
