#ifndef _STACK_H
#define _STACK_H

// For malloc, realloc:
#include <stdlib.h>

#define stack(t) struct { \
    t *content; \
    int size; \
    int max_size; \
}

#define init_stack(t, s) { \
    .content  = (t *) malloc((sizeof t) * s), \
    .size     = 0, \
    .max_size = s \
}

#define pop(s) (s.content[--(s.size)])

#define push_unsafe(s, a) (s.content[(s.size)++] = a)
#define push(s, a) ({ \
    __typeof__(s.content) x; \
    if (s.size == s.max_size) { \
        x = (__typeof__(x)) realloc(s.content, sizeof (*x) * s.max_size * 2); \
        if (x) { \
            s.content = x; \
            s.max_size *= 2; \
            push_unsafe(s, a); \
        } \
    } else { \
        x = s.content; \
        push_unsafe(s, a); \
    } \
    x; \
})

#endif /* _STACK_H */
