#ifndef _REGION_H
#define _REGION_H

#include <stdbool.h>
#include <dirent.h>

#include "llist.h"
#include "types.h"

struct region *intersection(const struct region *a, const struct region *b);

bool overlaps(struct region *a, struct region *b);

struct region *move_relative(struct region *root, struct region *x);

void stitch_region(struct region *desired, char *tile_dirname, char *out_name,
    struct options *opts);

int offset(struct region *region, czi_coord_t off_x, czi_coord_t off_y);

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#endif /* _REGION_H */
