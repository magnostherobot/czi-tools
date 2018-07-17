#ifndef _LLIST_H

#include <stdbool.h>

typedef struct ll_node {
    void           *content;
    struct ll_node *next;
} llist;

typedef bool ll_comparator(void *, void *);

typedef bool ll_foreach_fn(void *, void*);

llist *ll_add_item(llist *list, void *content, ll_comparator *comp);

bool ll_foreach(llist *list,ll_foreach_fn *fn, void *data);

int ll_length(llist *list);

#define _LLIST_H
#endif
