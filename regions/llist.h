#ifndef _LLIST_H

#include <stdbool.h>

typedef struct ll_node {
    void           *content;
    struct ll_node *next;
    int             _length;
} llist;

typedef bool ll_comparator(void *, void *);

llist *ll_add_item(llist *list, void *content, ll_comparator *comp);

int ll_length(llist *list);

#define _LLIST_H
#endif
