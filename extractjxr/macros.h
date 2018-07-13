#ifndef _MACROS_H
#define _MACROS_H

#define ferr(s, msg, ...) err(s, "%s: " msg, __func__, __VA_ARGS__)
#define ferrx(s, msg, ...) errx(s, "%s: " msg, __func__, __VA_ARGS__)

#define ferr1(s, msg) err(s, "%s: " msg, __func__)
#define ferrx1(s, msg) errx(s, "%s: " msg, __func__)

#endif /* _MACROS_H */