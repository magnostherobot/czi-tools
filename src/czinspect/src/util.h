#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>

#include "types.h"
#include "alloc.h"

#define czi_subblock_dim_ratio(d)    ((d)->stored_size == 0 ? 1 : ((d)->size / (d)->stored_size))

int xfallocate(int, size_t);

uint32_t get_subsample_level(lzbuf, uint32_t);
int make_reslist(struct map_ctx *, lzbuf, uint32_t *);

#endif /* _UTIL_H */
