#include <stdlib.h>

#include "llist.h"

llist *ll_add_item(llist *list, void *content, ll_comparator *comp) {
    struct ll_node *node = list;
    struct ll_node *prev = NULL;
    while (node && (*comp)(node->content, content)) {
        prev = node;
        node = node->next;
    }
    struct ll_node *new_node = (struct ll_node *) malloc(sizeof (*new_node));
    if (!new_node) return NULL;
    new_node->content = content;
    new_node->next    = node;
    if (prev) {
        prev->next = new_node;
        new_node->_length = prev->_length + 1;
    } else {
        new_node->_length = 1;
        return new_node;
    }
    return list;
}

int ll_length(llist *list) {
    return list->_length;
}
