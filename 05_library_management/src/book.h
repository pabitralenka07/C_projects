#ifndef BOOK_H
#define BOOK_H

#include <stdbool.h>

/* Forward-declare Library to avoid circular includes */
typedef struct Library Library;

#define BOOK_TITLE_LEN   128
#define BOOK_AUTHOR_LEN   64
#define BOOK_ISBN_LEN     20

typedef struct {
    int  id;
    char title[BOOK_TITLE_LEN];
    char author[BOOK_AUTHOR_LEN];
    char isbn[BOOK_ISBN_LEN];
    int  year;
    int  copies_total;
    int  copies_available;
    int  is_deleted;           /* 0 = active, 1 = deleted (int for stable ABI) */
} Book;

/* Add a new book. Returns assigned ID, or -1 on failure (duplicate ISBN, alloc error). */
int  book_add(Library *lib, const char *title, const char *author,
              const char *isbn, int year, int copies);

/* Find an active book by ID. Returns pointer or NULL. */
Book *book_find_by_id(Library *lib, int id);

/* Find an active book by ISBN. Returns pointer or NULL. */
Book *book_find_by_isbn(Library *lib, const char *isbn);

/* Search books by title/author/ISBN substring (case-insensitive).
 * Fills results[] with matching IDs, up to max_results.
 * Returns the number of matches found. */
int  book_search(Library *lib, const char *query, int *results, int max_results);

/* Soft-delete a book. Returns true if found and deleted. */
bool book_delete(Library *lib, int id);

/* Update book details. Returns true on success. */
bool book_update(Library *lib, int id, const char *title, const char *author,
                 const char *isbn, int year, int copies);

/* Print a formatted table of all active books. */
void book_list_all(Library *lib);

/* Print details of a single book. */
void book_print(const Book *book);

/* Validate a Book record for field correctness. */
bool book_validate(const Book *book);

/* Interactive sub-menu for book management. */
void book_menu(Library *lib);

#endif /* BOOK_H */
