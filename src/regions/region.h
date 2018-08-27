#ifndef _REGION_H
#define _REGION_H

#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>

#include "llist.h"
#include "main.h"

typedef uint32_t czi_coord_t;

struct region {
    czi_coord_t up;
    czi_coord_t left;
    czi_coord_t down;
    czi_coord_t right;
    int         scale;
};

struct region *intersection(const struct region *a, const struct region *b);
bool           overlaps(struct region *a, struct region *b);
struct region *move_relative(struct region *root, struct region *x);
llist         *find_relevant_tiles(struct region *desired, char *tile_dirname, struct options *opts);
void           stitch_region(struct region *desired, char *tile_dirname, struct options *opts);
int            set_side(struct dirent *ent, char *id, struct options *opts, czi_coord_t *left, czi_coord_t *right, int *scale);

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#endif /* _REGION_H */
