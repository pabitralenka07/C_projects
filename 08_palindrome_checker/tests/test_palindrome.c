#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "palindrome.h"

static void test_strict_strings(void) {
    assert(palindrome_check_str("racecar", false));
    assert(palindrome_check_str("kayak", false));
    assert(palindrome_check_str("deified", false));
    assert(palindrome_check_str("a", false));
    assert(palindrome_check_str("", false));

    /* Strict mode differentiates case */
    assert(!palindrome_check_str("Racecar", false));
    assert(!palindrome_check_str("hello", false));
    assert(!palindrome_check_str("ab", false));
    printf("  [PASS] test_strict_strings\n");
}

static void test_relaxed_strings(void) {
    assert(palindrome_check_str("Racecar", true));
    assert(palindrome_check_str("A man, a plan, a canal: Panama!", true));
    assert(palindrome_check_str("Was it a car or a cat I saw?", true));
    assert(palindrome_check_str("No 'x' in Nixon", true));
    assert(palindrome_check_str("Madam, I'm Adam", true));
    assert(palindrome_check_str("Step on no pets", true));

    assert(!palindrome_check_str("This is definitely not a palindrome", true));
    printf("  [PASS] test_relaxed_strings\n");
}

static void test_integers(void) {
    assert(palindrome_check_int(0));
    assert(palindrome_check_int(7));
    assert(palindrome_check_int(121));
    assert(palindrome_check_int(12321));
    assert(palindrome_check_int(1221));
    assert(palindrome_check_int(1000000001LL));

    assert(!palindrome_check_int(10));
    assert(!palindrome_check_int(123));
    assert(!palindrome_check_int(100));
    /* Negative numbers are not palindromes due to '-' */
    assert(!palindrome_check_int(-121));
    assert(!palindrome_check_int(-1));
    printf("  [PASS] test_integers\n");
}

static void test_longest_substring(void) {
    char out[128];
    size_t len;

    len = palindrome_longest_substring("forgeeksskeegfor", out, sizeof(out));
    assert(len == 10);
    assert(strcmp(out, "geeksskeeg") == 0);

    len = palindrome_longest_substring("cbbd", out, sizeof(out));
    assert(len == 2);
    assert(strcmp(out, "bb") == 0);

    len = palindrome_longest_substring("babad", out, sizeof(out));
    assert(len == 3);
    assert(strcmp(out, "bab") == 0 || strcmp(out, "aba") == 0);

    len = palindrome_longest_substring("abcdef", out, sizeof(out));
    assert(len == 1);

    len = palindrome_longest_substring("", out, sizeof(out));
    assert(len == 0);
    assert(strcmp(out, "") == 0);

    printf("  [PASS] test_longest_substring\n");
}

int main(void) {
    printf("=== Running Palindrome Checker Unit Tests ===\n");
    test_strict_strings();
    test_relaxed_strings();
    test_integers();
    test_longest_substring();
    printf("All palindrome unit tests passed!\n");
    return 0;
}
