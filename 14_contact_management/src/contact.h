#ifndef CONTACT_H
#define CONTACT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONTACT_NAME_LEN    64
#define CONTACT_PHONE_LEN   20
#define CONTACT_EMAIL_LEN   64
#define CONTACT_ADDR_LEN    96
#define CONTACT_CAT_LEN     32

#define CONTACT_MAGIC       "CTMs"
#define CONTACT_DATA_FILE   "contacts.dat"

typedef struct {
    int  id;
    char name[CONTACT_NAME_LEN];
    char phone[CONTACT_PHONE_LEN];
    char email[CONTACT_EMAIL_LEN];
    char address[CONTACT_ADDR_LEN];
    char category[CONTACT_CAT_LEN];
    int  is_deleted;
} Contact;

typedef struct {
    Contact *items;
    int      count;
    int      capacity;
    int      next_id;
    char     data_dir[256];
} ContactBook;

typedef enum {
    SORT_NAME_ASC = 1,
    SORT_NAME_DESC = 2,
    SORT_ID_ASC = 3
} ContactSortOrder;

/* Lifecycle */
void contact_book_init(ContactBook *book);
void contact_book_free(ContactBook *book);
bool contact_book_load(ContactBook *book);
bool contact_book_save(ContactBook *book);

/* Format Validation */
bool contact_validate_phone(const char *phone);
bool contact_validate_email(const char *email);

/* Duplicate Checking */
int contact_find_duplicate_phone(const ContactBook *book, const char *phone, int exclude_id);
int contact_find_duplicate_email(const ContactBook *book, const char *email, int exclude_id);

/* Core Operations */
int  contact_add(ContactBook *book, const char *name, const char *phone,
                 const char *email, const char *address, const char *category);
bool contact_update(ContactBook *book, int id, const char *name, const char *phone,
                    const char *email, const char *address, const char *category);
bool contact_delete(ContactBook *book, int id);
Contact *contact_find_by_id(ContactBook *book, int id);

/* Search and Sort */
int  contact_search(const ContactBook *book, const char *query, int *results, int max_results);
void contact_sort(ContactBook *book, ContactSortOrder order);
void contact_list_all(const ContactBook *book);

/* CSV Import / Export */
bool contact_export_csv(const ContactBook *book, const char *filepath);
bool contact_import_csv(ContactBook *book, const char *filepath, int *imported_count, int *skipped_count);

#endif /* CONTACT_H */
