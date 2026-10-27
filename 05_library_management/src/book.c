#include "book.h"
#include "library.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

/* Grow the books array if at capacity. Doubles allocation each time. */
static bool ensure_book_capacity(Library *lib) {
    if (lib->book_count < lib->book_capacity) {
        return true;
    }

    int new_cap = (lib->book_capacity == 0) ? 16 : lib->book_capacity * 2;
    Book *grown = realloc(lib->books, (size_t)new_cap * sizeof(Book));
    if (!grown) {
        fprintf(stderr, "Error: Memory allocation failed for books.\n");
        return false;
    }

    lib->books = grown;
    lib->book_capacity = new_cap;
    return true;
}

/* Case-insensitive substring search */
static bool str_contains_ci(const char *haystack, const char *needle) {
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);

    if (nlen == 0) return true;
    if (nlen > hlen) return false;

    for (size_t i = 0; i <= hlen - nlen; i++) {
        bool match = true;
        for (size_t j = 0; j < nlen; j++) {
            if (tolower((unsigned char)haystack[i + j]) !=
                tolower((unsigned char)needle[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/*  CRUD operations                                                    */
/* ------------------------------------------------------------------ */

int book_add(Library *lib, const char *title, const char *author,
             const char *isbn, int year, int copies) {
    /* Reject duplicate ISBNs (when ISBN is provided) */
    if (isbn && isbn[0] != '\0') {
        if (book_find_by_isbn(lib, isbn) != NULL) {
            fprintf(stderr, "Error: A book with ISBN '%s' already exists.\n", isbn);
            return -1;
        }
    }

    if (!ensure_book_capacity(lib)) {
        return -1;
    }

    Book *b = &lib->books[lib->book_count];
    memset(b, 0, sizeof(Book));

    b->id = lib->next_book_id++;
    snprintf(b->title,  sizeof(b->title),  "%s", title);
    snprintf(b->author, sizeof(b->author), "%s", author);
    snprintf(b->isbn,   sizeof(b->isbn),   "%s", isbn ? isbn : "");
    b->year             = year;
    b->copies_total     = copies;
    b->copies_available = copies;
    b->is_deleted       = 0;

    lib->book_count++;
    return b->id;
}

Book *book_find_by_id(Library *lib, int id) {
    for (int i = 0; i < lib->book_count; i++) {
        if (lib->books[i].id == id && !lib->books[i].is_deleted) {
            return &lib->books[i];
        }
    }
    return NULL;
}

Book *book_find_by_isbn(Library *lib, const char *isbn) {
    if (!isbn || isbn[0] == '\0') return NULL;

    for (int i = 0; i < lib->book_count; i++) {
        if (!lib->books[i].is_deleted &&
            strcmp(lib->books[i].isbn, isbn) == 0) {
            return &lib->books[i];
        }
    }
    return NULL;
}

int book_search(Library *lib, const char *query, int *results, int max_results) {
    int found = 0;
    for (int i = 0; i < lib->book_count && found < max_results; i++) {
        if (lib->books[i].is_deleted) continue;

        if (str_contains_ci(lib->books[i].title,  query) ||
            str_contains_ci(lib->books[i].author, query) ||
            str_contains_ci(lib->books[i].isbn,   query)) {
            results[found++] = lib->books[i].id;
        }
    }
    return found;
}

bool book_delete(Library *lib, int id) {
    Book *b = book_find_by_id(lib, id);
    if (!b) return false;
    b->is_deleted = 1;
    return true;
}

bool book_update(Library *lib, int id, const char *title, const char *author,
                 const char *isbn, int year, int copies) {
    Book *b = book_find_by_id(lib, id);
    if (!b) return false;

    /* Guard against ISBN collision with a different book */
    if (isbn && isbn[0] != '\0') {
        Book *dup = book_find_by_isbn(lib, isbn);
        if (dup && dup->id != id) {
            fprintf(stderr, "Error: ISBN '%s' is already assigned to another book.\n", isbn);
            return false;
        }
    }

    snprintf(b->title,  sizeof(b->title),  "%s", title);
    snprintf(b->author, sizeof(b->author), "%s", author);
    snprintf(b->isbn,   sizeof(b->isbn),   "%s", isbn ? isbn : "");
    b->year = year;

    /* Adjust available copies so the difference carries over.
     * E.g., if 2 of 5 were issued (3 available) and total changes to 7,
     * available becomes 3 + (7-5) = 5, keeping 2 issued. */
    int issued = b->copies_total - b->copies_available;
    b->copies_total = copies;
    b->copies_available = copies - issued;
    if (b->copies_available < 0) b->copies_available = 0;

    return true;
}

/* ------------------------------------------------------------------ */
/*  Display                                                            */
/* ------------------------------------------------------------------ */

void book_list_all(Library *lib) {
    printf("\n");
    util_print_separator(92);
    printf("  %-4s | %-30s | %-20s | %-13s | %-4s | %s\n",
           "ID", "Title", "Author", "ISBN", "Year", "Avail/Total");
    util_print_separator(92);

    int count = 0;
    for (int i = 0; i < lib->book_count; i++) {
        if (lib->books[i].is_deleted) continue;
        const Book *b = &lib->books[i];
        printf("  %-4d | %-30.30s | %-20.20s | %-13.13s | %-4d | %d/%d\n",
               b->id, b->title, b->author,
               b->isbn[0] ? b->isbn : "-",
               b->year, b->copies_available, b->copies_total);
        count++;
    }

    util_print_separator(92);
    printf("  Total: %d book(s)\n", count);
}

void book_print(const Book *book) {
    printf("  ID:        %d\n",   book->id);
    printf("  Title:     %s\n",   book->title);
    printf("  Author:    %s\n",   book->author);
    printf("  ISBN:      %s\n",   book->isbn[0] ? book->isbn : "(none)");
    printf("  Year:      %d\n",   book->year);
    printf("  Copies:    %d available / %d total\n",
           book->copies_available, book->copies_total);
}

bool book_validate(const Book *book) {
    if (book->id <= 0) return false;
    if (book->title[0] == '\0') return false;
    if (book->author[0] == '\0') return false;
    if (book->year < 0 || book->year > 9999) return false;
    if (book->copies_total < 0 || book->copies_total > 99999) return false;
    if (book->copies_available < 0) return false;
    if (book->copies_available > book->copies_total) return false;
    /* Strings must be NUL-terminated within their buffers */
    if (book->title[BOOK_TITLE_LEN - 1]  != '\0') return false;
    if (book->author[BOOK_AUTHOR_LEN - 1] != '\0') return false;
    if (book->isbn[BOOK_ISBN_LEN - 1]    != '\0') return false;
    return true;
}

/* ------------------------------------------------------------------ */
/*  Interactive menu                                                   */
/* ------------------------------------------------------------------ */

void book_menu(Library *lib) {
    for (;;) {
        printf("\n");
        util_print_header("Book Management");
        printf("  1. Add Book\n");
        printf("  2. List All Books\n");
        printf("  3. Search Books\n");
        printf("  4. View Book Details\n");
        printf("  5. Update Book\n");
        printf("  6. Delete Book\n");
        printf("  7. Back to Main Menu\n\n");

        int choice = input_menu_choice(1, 7);
        if (choice == -1 || choice == 7) return;

        switch (choice) {
        case 1: { /* Add Book */
            printf("\n--- Add New Book ---\n");
            char title[BOOK_TITLE_LEN], author[BOOK_AUTHOR_LEN];
            char isbn[BOOK_ISBN_LEN];
            int year, copies;

            if (!input_read_string("  Title: ", title, sizeof(title)))  break;
            if (!input_read_string("  Author: ", author, sizeof(author))) break;

            /* ISBN is optional — allow empty */
            printf("  ISBN (Enter to skip): ");
            fflush(stdout);
            if (!input_read_line(isbn, sizeof(isbn), stdin)) break;
            input_trim(isbn);

            if (!input_read_int("  Publication Year: ", &year, 1, 9999)) break;
            if (!input_read_int("  Number of Copies: ", &copies, 1, 99999)) break;

            int id = book_add(lib, title, author, isbn, year, copies);
            if (id > 0) {
                printf("\n  Book added successfully! (ID: %d)\n", id);
                library_save(lib);
            }
            break;
        }
        case 2: /* List All */
            book_list_all(lib);
            break;

        case 3: { /* Search */
            char query[128];
            if (!input_read_string("  Search (title/author/ISBN): ", query, sizeof(query)))
                break;

            int results[50];
            int found = book_search(lib, query, results, 50);
            if (found == 0) {
                printf("  No books found matching '%s'.\n", query);
            } else {
                printf("\n  Found %d book(s):\n", found);
                for (int i = 0; i < found; i++) {
                    Book *b = book_find_by_id(lib, results[i]);
                    if (b) {
                        printf("  [%d] \"%s\" by %s\n", b->id, b->title, b->author);
                    }
                }
            }
            break;
        }
        case 4: { /* View Details */
            int id;
            if (!input_read_int("  Enter Book ID: ", &id, 1, 999999)) break;
            Book *b = book_find_by_id(lib, id);
            if (b) {
                printf("\n");
                book_print(b);
            } else {
                printf("  Book with ID %d not found.\n", id);
            }
            break;
        }
        case 5: { /* Update */
            int id;
            if (!input_read_int("  Enter Book ID to update: ", &id, 1, 999999)) break;
            Book *b = book_find_by_id(lib, id);
            if (!b) {
                printf("  Book with ID %d not found.\n", id);
                break;
            }

            printf("\n  Current details:\n");
            book_print(b);
            printf("\n  Enter new values (press Enter to keep current):\n");

            char title[BOOK_TITLE_LEN], author[BOOK_AUTHOR_LEN];
            char isbn[BOOK_ISBN_LEN], buf[64];
            int year, copies;

            printf("  Title [%s]: ", b->title);
            fflush(stdout);
            if (!input_read_line(title, sizeof(title), stdin)) break;
            input_trim(title);
            if (title[0] == '\0') snprintf(title, sizeof(title), "%s", b->title);

            printf("  Author [%s]: ", b->author);
            fflush(stdout);
            if (!input_read_line(author, sizeof(author), stdin)) break;
            input_trim(author);
            if (author[0] == '\0') snprintf(author, sizeof(author), "%s", b->author);

            printf("  ISBN [%s]: ", b->isbn[0] ? b->isbn : "(none)");
            fflush(stdout);
            if (!input_read_line(isbn, sizeof(isbn), stdin)) break;
            input_trim(isbn);
            if (isbn[0] == '\0') snprintf(isbn, sizeof(isbn), "%s", b->isbn);

            printf("  Year [%d]: ", b->year);
            fflush(stdout);
            if (!input_read_line(buf, sizeof(buf), stdin)) break;
            input_trim(buf);
            if (buf[0] == '\0') {
                year = b->year;
            } else {
                char *end;
                long val = strtol(buf, &end, 10);
                if (*end != '\0' || val < 1 || val > 9999) {
                    printf("  Invalid year — keeping current.\n");
                    year = b->year;
                } else {
                    year = (int)val;
                }
            }

            printf("  Copies [%d]: ", b->copies_total);
            fflush(stdout);
            if (!input_read_line(buf, sizeof(buf), stdin)) break;
            input_trim(buf);
            if (buf[0] == '\0') {
                copies = b->copies_total;
            } else {
                char *end;
                long val = strtol(buf, &end, 10);
                if (*end != '\0' || val < 1 || val > 99999) {
                    printf("  Invalid count — keeping current.\n");
                    copies = b->copies_total;
                } else {
                    copies = (int)val;
                }
            }

            if (book_update(lib, id, title, author, isbn, year, copies)) {
                printf("\n  Book updated successfully!\n");
                library_save(lib);
            }
            break;
        }
        case 6: { /* Delete */
            int id;
            if (!input_read_int("  Enter Book ID to delete: ", &id, 1, 999999)) break;
            Book *b = book_find_by_id(lib, id);
            if (!b) {
                printf("  Book with ID %d not found.\n", id);
                break;
            }

            printf("  About to delete: \"%s\" by %s\n", b->title, b->author);
            if (input_confirm("  Are you sure?", false)) {
                if (book_delete(lib, id)) {
                    printf("  Book deleted successfully.\n");
                    library_save(lib);
                }
            } else {
                printf("  Deletion cancelled.\n");
            }
            break;
        }
        default:
            break;
        }

        util_press_enter();
    }
}
