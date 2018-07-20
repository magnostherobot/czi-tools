#ifndef _COMPAT_H
#define _COMPAT_H

#ifdef NOT_BSD

long long strtonum(const char *, long long, long long, const char **);
void *reallocarray(void *, size_t, size_t);

#endif

#endif /* _COMPAT_H */
