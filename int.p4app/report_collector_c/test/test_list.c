#include <stdio.h>
#include <string.h>
#include "../list.h"

int main() {
    list mylist;
    char* string1 = "pierszy";
    char* string2 = "drugi";
    char* string3 = "trzeci";
    char* string4 = "czwarty";
    list_init(&mylist, 10);
    list_insert(&mylist, string1, strlen(string1));
    list_insert(&mylist, string2, strlen(string2));
    list_insert(&mylist, string3, strlen(string3));
    list_el* elem = mylist.first;
    while(elem != NULL) {
        printf("%s\n", (char*)elem->data);
        elem = elem->next;
    }
    list_insert(&mylist, string4, strlen(string4));
    elem = mylist.first;
    while(elem != NULL) {
        printf("%s\n", (char*)elem->data);
        elem = elem->next;
    }
    return 0;
}
