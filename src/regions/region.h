#ifndef _REGION_H
#define _REGION_H

#include <stdint.h>
#include <stdbool.h>

#include "llist.h"

struct region {
    uint32_t up;
    uint32_t left;
    uint32_t down;
    uint32_t right;
    int      scale;
};

struct region *intersection(const struct region *a, const struct region *b);
bool           overlaps(struct region *a, struct region *b);
struct region *move_relative(struct region *root, struct region *x);
llist         *find_relevant_tiles(struct region *desired, char *tile_dirname);
void           stitch_region(struct region *desired, char *tile_dirname);

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#endif /* _REGION_H */
