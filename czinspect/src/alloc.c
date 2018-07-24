
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

/* volatile function pointer magic taken from OpenBSD explicit_bzero
 * to avoid this call being optimised out */
static void* (*volatile lzmemset)(void *, int, size_t) = memset;

lzstring _lzstr_new(const char *name, size_t num, size_t sz) {
    lzstring ret = malloc(sizeof(lzstring));
    if (ret == NULL)
        ferr("could not allocate dynamic string in function \"%s\"", name);

    ret->data = reallocarray(NULL, num, sz);
    if (ret->data == NULL)
        ferr("could not allocate dynamic string in function \"%s\"", name);

    ret->len = sz;
    lzmemset(ret->data, 0, ret->len);

    return ret;
}

void lzstr_free(lzstring l) {
    free(l->data);
    free(l);
}
        
