#ifndef LIST_H
#define LIST_H

typedef struct list_el list_el;
struct list_el {
    list_el*    next;
    list_el*    prev;
    void*       data;
    int         len;
};

typedef struct list {
    list_el* first;
    list_el* last;
    int      len;
    int      max_len;
} list;

int list_init(list* ls, int max_len);
int list_insert(list* ls, void* data, int datalen);
void list_free(list* ls);

#endif // LIST_H