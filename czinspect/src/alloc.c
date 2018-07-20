
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "macros.h"

#include "compat/compat.h"

void *_xmalloc(const char *name, size_t sz) {
    void *p = malloc(sz);
    if (p == NULL)
        ferr("could not allocate memory in function \"%s\"", name);
    return p;
}

void *_xreallocarray(const char *name, void *ptr, size_t num, size_t size) {
    void *p = reallocarray(ptr, num, size);
    if (p == NULL)
        ferr("could not allocate memory in function \"%s\"", name);
    return p;
}

char *_xstrdup(const char *name, char *str) {
    char *p = strdup(str);
    if (p == NULL)
        ferr("could not duplicate string in function \"%s\"", name);
    return p;
}
