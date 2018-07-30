#ifndef _MACROS_H
#define _MACROS_H

#define ferr(msg, ...) err(1, "%s: " msg, __func__, __VA_ARGS__)
#define ferrx(msg, ...) errx(1, "%s: " msg, __func__, __VA_ARGS__)

#define ferr1(msg) err(1, "%s: " msg, __func__)
#define ferrx1(msg) errx(1, "%s: " msg, __func__)

#define fwarn(msg, ...) warn("%s: " msg, __func__, __VA_ARGS__)
#define fwarnx(msg, ...) warnx("%s: " msg, __func__, __VA_ARGS__)

#define fwarn1(msg) warn("%s: " msg, __func__)
#define fwarnx1(msg) warnx("%s: " msg, __func__)

#define nerr(func, msg, ...) err(1, "%s: %s: " msg, func, __func__, __VA_ARGS__)
#define nerrx(func, msg, ...) errx(1, "%s: %s: " msg, func, __func__, __VA_ARGS__)

#define nerr1(func, msg) err(1, "%s: %s: " msg, func, __func__)
#define nerrx1(func, msg) errx(1, "%s: %s: " msg, func, __func__)

#endif /* _MACROS_H */
