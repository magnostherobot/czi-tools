#ifndef _ALLOC_H
#define _ALLOC_H

#define xmalloc(sz)                     _xmalloc(__func__, sz);
#define xreallocarray(ptr, num, size)   _xreallocarray(__func__, ptr, num, size);
#define xfree(p)                        free(p)
#define xstrdup(s)                      _xstrdup(__func__, s);

void *_xmalloc(const char *, size_t);
void *_xreallocarray(const char *, void *, size_t, size_t);
char *_xstrdup(const char *, char *);


#endif /* _ALLOC_H */
