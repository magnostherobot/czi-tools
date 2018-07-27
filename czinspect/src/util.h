#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>

#include "types.h"

#define czi_subblock_dim_ratio(d)    ((d)->stored_size == 0 ? 1 : ((d)->size / (d)->stored_size))

int xfallocate(int, size_t);

#endif /* _UTIL_H */
