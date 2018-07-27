#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <stdint.h>

#include "config.h"

/* magic endianness switching for platforms that require it */

#ifdef IS_BIG_ENDIAN
# define SWITCH_ENDS(v) switch_endianness((char *) v, sizeof(*v))

static inline void switch_endianness(char *val, size_t sz) {
    static char tmp;

    for (size_t i = 0; i < sz / 2; i++) {
        tmp = val[sz - i];
        val[sz - i] = val[i];
        val[i] = tmp;
    }
}

#else
# define SWITCH_ENDS(v)
#endif

#endif /* _ENDIAN_H */
