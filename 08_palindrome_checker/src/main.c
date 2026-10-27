/*
 * Palindrome Checker — Main Entry Point
 * Supports string checks (strict and relaxed sentence mode),
 * integer checks, and longest palindromic substring discovery.
 */

#include "palindrome.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_menu(void) {
    printf("\n");
    util_print_header("Palindrome Checker");
    printf("  1. Check String or Sentence\n");
    printf("  2. Check Integer\n");
    printf("  3. Find Longest Palindromic Substring\n");
    printf("  4. View Famous Palindrome Examples\n");
    printf("  5. Exit\n\n");
}

static void do_check_string(void) {
    char buf[512];
    printf("\n--- Check String or Sentence ---\n");
    if (!input_read_string("  Enter text: ", buf, sizeof(buf))) return;

    bool relaxed = input_confirm("  Ignore punctuation, spaces, and case? (Sentence mode)", true);
    bool is_pal = palindrome_check_str(buf, relaxed);

    printf("\n  Result: \"%s\" is %s\n",
           buf,
           is_pal ? "a PALINDROME! ✅" : "NOT a palindrome. ❌");
    if (relaxed && !palindrome_check_str(buf, false) && is_pal) {
        printf("  (Note: Matches in relaxed mode, but differs strictly due to case/punctuation)\n");
    }
}

static void do_check_integer(void) {
    printf("\n--- Check Integer ---\n");
    int val;
    if (!input_read_int("  Enter an integer: ", &val, -2000000000, 2000000000)) return;

    bool is_pal = palindrome_check_int((long long)val);
    printf("\n  Result: %d is %s\n",
           val,
           is_pal ? "a PALINDROMIC NUMBER! ✅" : "NOT a palindromic number. ❌");
}

static void do_longest_substring(void) {
    char buf[512];
    char result[512];
    printf("\n--- Longest Palindromic Substring ---\n");
    if (!input_read_string("  Enter text: ", buf, sizeof(buf))) return;

    size_t len = palindrome_longest_substring(buf, result, sizeof(result));
    printf("\n  Longest palindromic substring: \"%s\" (Length: %zu)\n", result, len);
}

static void do_famous_examples(void) {
    static const char *examples[] = {
        "A man, a plan, a canal: Panama!",
        "Was it a car or a cat I saw?",
        "No 'x' in Nixon",
        "Madam, I'm Adam",
        "Step on no pets",
        "Live on time, emit no evil",
        "Red rum, sir, is murder"
    };
    size_t count = sizeof(examples) / sizeof(examples[0]);

    printf("\n--- Famous Palindrome Examples ---\n");
    for (size_t i = 0; i < count; i++) {
        bool pal = palindrome_check_str(examples[i], true);
        printf("  [%zu] \"%-35s\" -> %s\n",
               i + 1, examples[i], pal ? "VALID ✅" : "INVALID ❌");
    }
}

int main(void) {
    for (;;) {
        print_menu();
        int choice = input_menu_choice(1, 5);
        if (choice == -1 || choice == 5) {
            break;
        }

        switch (choice) {
        case 1:
            do_check_string();
            break;
        case 2:
            do_check_integer();
            break;
        case 3:
            do_longest_substring();
            break;
        case 4:
            do_famous_examples();
            break;
        default:
            break;
        }

        util_press_enter();
    }

    printf("\nThank you for using Palindrome Checker. Goodbye!\n");
    return EXIT_SUCCESS;
}
