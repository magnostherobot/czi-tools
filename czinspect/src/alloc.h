#ifndef _ALLOC_H
#define _ALLOC_H

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

#define LZS_DEFAULT_LEN  40 /* default length of allocated strings */

#define lzstr_new()            _lzstr_new(__func__, LZS_DEFAULT_LEN, sizeof(char))
#define lzstr_arr_new(t, e)    _lzstr_new(__func__, e, sizeof(t))
#define lzstr_arr_get(t, l, i) (((t *) (l->data))[i])

lzstring _lzstr_new(const char *, size_t, size_t);
void _lzstr_free(lzstring);

#endif /* _ALLOC_H */
