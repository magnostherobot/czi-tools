#ifndef _NAME_H
#define _NAME_H

#include <dirent.h>

#include "region.h"
#include "llist.h"

int set_side(struct dirent *ent, char *id, struct options *opts,
    czi_coord_t *left, czi_coord_t *right, int *scale);

llist *find_relevant_tiles(struct region *desired, char *tile_dirname,
    struct options *opts);

int find_smallest(char *tile_dirname, czi_coord_t *x, czi_coord_t *y,
    char **filename, struct options *opts);

int region_filename(const struct region *region, char *buffer,
    const struct options *opts);

#endif /* _NAME_H */
