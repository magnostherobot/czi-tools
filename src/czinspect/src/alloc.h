#ifndef _ALLOC_H
#define _ALLOC_H

#include <stdarg.h>
#include <stdint.h>

#define xmalloc(sz)                     _xmalloc(__func__, sz)
#define xreallocarray(ptr, num, size)   _xreallocarray(__func__, ptr, num, size)
#define xfree(p)                        free(p)
#define xstrdup(s)                      _xstrdup(__func__, s)

void *_xmalloc(const char *, size_t);
void *_xreallocarray(const char *, void *, size_t, size_t);
char *_xstrdup(const char *, char *);


/* Lazily allocated string structure; inspired by djb's stralloc and
 * skarnet's genalloc */
struct lzstring_s {
    char *data;   /* pointer to data */
    size_t len;   /* size of allocated data area */
};
typedef struct lzstring_s * lzstring;
typedef lzstring lzbuf;

#define LZS_DEFAULT_LEN  40 /* default length of allocated strings */
#define LZB_DEFAULT_LEN   8 /* default length of allocated buffer arrays */

#define lzstr_new()            _lzstr_new(__func__, LZS_DEFAULT_LEN, sizeof(char))
#define lzbuf_new(type)        _lzstr_new(__func__, LZB_DEFAULT_LEN, sizeof(type))

#define lzbuf_get(type, b, i)  (((type*) ((b)->data))[i])
#define lzbuf_elems(type, b)   (b->len / sizeof(type))

#define lzstr_zero(l)          _lzstr_zero(l)
#define lzbuf_zero(l)          _lzstr_zero(l)

#define lzstr_grow(l)          _lzstr_resize(__func__, l, (l)->len * 2, sizeof(char))
#define lzbuf_grow(type, b)    _lzstr_resize(__func__, b, (b)->len * 2, sizeof(type))

#define lzstr_cat(l, s)        _lzstr_cat(__func__, l, s)

#define lzstr_free(s)          _lzstr_free(s)
#define lzbuf_free(s)          _lzstr_free(s)

lzstring _lzstr_new(const char *, size_t, size_t);
void _lzstr_zero(lzstring);
void _lzstr_resize(const char *, lzstring, size_t, size_t);
void _lzstr_free(lzstring);

void _lzstr_cat(const char *, lzstring, char *);

int lzsprintf(lzstring, const char *, ...);
int lzvprintf(lzstring, const char *, va_list);

#endif /* _ALLOC_H */
