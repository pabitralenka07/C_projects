#ifndef MEMBER_H
#define MEMBER_H

#include <stdbool.h>

typedef struct Library Library;

#define MEMBER_NAME_LEN   64
#define MEMBER_PHONE_LEN  16
#define MEMBER_EMAIL_LEN  64

typedef struct {
    int  id;
    char name[MEMBER_NAME_LEN];
    char phone[MEMBER_PHONE_LEN];
    char email[MEMBER_EMAIL_LEN];
    int  is_deleted;
} Member;

int     member_add(Library *lib, const char *name, const char *phone, const char *email);
Member *member_find_by_id(Library *lib, int id);
int     member_search(Library *lib, const char *query, int *results, int max_results);
bool    member_delete(Library *lib, int id);
bool    member_update(Library *lib, int id, const char *name, const char *phone, const char *email);
void    member_list_all(Library *lib);
void    member_print(const Member *member);
bool    member_validate(const Member *member);
void    member_menu(Library *lib);

#endif /* MEMBER_H */
