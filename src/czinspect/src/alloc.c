
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

lzstring *_lzstr_new(const char *name, size_t num, size_t sz) {
    lzstring *ret = malloc(sizeof(struct lzstring_s));
    if (ret == NULL)
        nerr1(name, "could not allocate dynamic string");

    ret->data = reallocarray(NULL, num, sz);
    if (ret->data == NULL)
        nerr1(name, "could not allocate dynamic string");

    ret->alen = num * sz;
    ret->len = 0;
    lzmemset(ret->data, 0, ret->alen);
    return ret;
}

void _lzstr_zero(lzstring *l) {
    lzmemset(l->data, 0, l->alen);
    l->len = 0;
}

void _lzstr_resize(const char *name, lzstring *l, size_t num, size_t sz) {
    l->data = reallocarray(l->data, num, sz);
    if (l->data == NULL)
        nerr1(name, "could not resize dynamic string");

    if (num * sz > l->alen)
        lzmemset(l->data + l->alen, 0, (num * sz) - l->alen);

    l->alen = num * sz;
}

void _lzstr_free(lzstring *l) {
    free(l->data);
    free(l);

    return;
}

void _lzstr_setlen(const char *name, lzstring *l, size_t num, size_t sz) {
    while (l->alen < num * sz)
        _lzstr_resize(name, l, l->alen * 2, sz);

    l->len = num * sz;
}

void _lzstr_cat(const char *name, lzstring *l, char *ap) {
    /* corner case: either the target buffer is empty or it contains
     * chars ended by a null byte. */
    size_t llen = l->len == 0 ? 0 : l->len - 1;
    size_t clen = strlen(ap) + 1; /* include null byte */

    _lzstr_setlen(name, l, llen + clen, sizeof(char));

    memcpy(&l->data[llen], ap, clen);
    /* the string is not explicitly null-terminated here, the
     * argument's null byte is included. */
}

int lzsprintf(lzstring *l, const char *fmt, ...) {
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = lzvprintf(l, fmt, ap);
    va_end(ap);

    return ret;
}

int lzvprintf(lzstring *l, const char *fmt, va_list ap) {
    int len;
    va_list ap2;

    va_copy(ap2, ap);
    len = vsnprintf(0, 0, fmt, ap2);
    va_end(ap2);

    if (len < 0)
        return -1;

    lzstr_setlen(l, (size_t)len + 1);

    return vsnprintf(l->data, (size_t)len + 1, fmt, ap);
}
