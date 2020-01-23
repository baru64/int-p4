#include <stdlib.h>
#include "list.h"

int list_init(list* ls, int max_len) {
    ls->max_len = max_len;
    ls->len = 0;
    ls->first = NULL;
    ls->last = NULL;
}

int list_insert(list* ls, void* data, int datalen) {
    if (ls->len > ls->max_len) {
        // remove last el
        list_el* last = ls->last;
        ls->last = last->prev;
        ls->len--;
        free(last);
    }
    list_el* new_el = malloc(sizeof(list_el));
    if (new_el == NULL) return -1;
    new_el->data = data;
    new_el->len = datalen;
    new_el->prev = NULL;
    new_el->next = NULL;

    if (ls->len == 0) {
        ls->first = new_el;
        ls->last = new_el;
    } else {
        new_el->next = ls->first;
        ls->first = new_el;
    }
    ls->len++;
    return 0;
}

void list_free(list* ls) {
    if (ls->len == 1) {
        free(ls->first->data);
        free(ls->first);
        free(ls);
    }
    list_el* i = ls->first->next;
    while(i != NULL) {
        free(i->prev->data);
        free(i->prev);
        i = i->next;
    }
    free(ls);
}