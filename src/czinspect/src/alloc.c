
#include "config.h"

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "macros.h"

#include "compat/compat.h"

void *_xmalloc(const char *name, size_t sz) {
    void *p = malloc(sz);
    if (p == NULL)
        nerr1(name, "could not allocate memory");
    return p;
}

void *_xreallocarray(const char *name, void *ptr, size_t num, size_t size) {
    void *p = reallocarray(ptr, num, size);
    if (p == NULL)
        nerr1(name, "could not allocate memory");
    return p;
}

char *_xstrdup(const char *name, char *str) {
    char *p = strdup(str);
    if (p == NULL)
        nerr1(name, "could not duplicate string");
    return p;
}

/* volatile function pointer magic taken from OpenBSD explicit_bzero
 * to avoid this call being optimised out */
static void* (*volatile lzmemset)(void *, int, size_t) = memset;

lzstring _lzstr_new(const char *name, size_t num, size_t sz) {
    lzstring ret = malloc(sizeof(struct lzstring_s));
    if (ret == NULL)
        nerr1(name, "could not allocate dynamic string");

    ret->data = reallocarray(NULL, num, sz);
    if (ret->data == NULL)
        nerr1(name, "could not allocate dynamic string");

    ret->len = num * sz;
    lzmemset(ret->data, 0, ret->len);

    return ret;
}

void _lzstr_zero(lzstring l) {
    lzmemset(l->data, 0, l->len);
}

void _lzstr_resize(const char *name, lzstring l, size_t num, size_t sz) {
    l->data = reallocarray(l->data, num, sz);
    if (l->data == NULL)
        nerr1(name, "could not resize dynamic string");

    if (num * sz > l->len)
        lzmemset(l->data + l->len, 0, (num * sz) - l->len);

    l->len = num * sz;
    
    return;
}

void _lzstr_free(lzstring l) {
    free(l->data);
    free(l);

    return;
}

void _lzstr_cat(const char *name, lzstring l, char *ap) {
    size_t llen = strlen(l->data);
    size_t alen = strlen(ap);

    while (l->len < llen + alen + 1)
        _lzstr_resize(name, l, l->len * 2, sizeof(char));

    memcpy(&l->data[llen], ap, alen);
    l->data[llen + alen] = '\0';

    return;
}

/* lz{s,v}printf() implementation based on {v,}asprintf() in musl libc */
int lzsprintf(lzstring l, const char *fmt, ...) {
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = lzvprintf(l, fmt, ap);
    va_end(ap);

    return ret;
}

int lzvprintf(lzstring l, const char *fmt, va_list ap) {
    int len;
    va_list ap2;

    va_copy(ap2, ap);
    len = vsnprintf(0, 0, fmt, ap2);
    va_end(ap2);

    if (len < 0)
        return -1;

    while (l->len < len + 1)
        lzstr_grow(l);

    return vsnprintf(l->data, len + 1, fmt, ap);
}



