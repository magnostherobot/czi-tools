#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef int32_t czi_coord_t;
#define CZI_T "%i"

struct region {
    czi_coord_t up;
    czi_coord_t left;
    czi_coord_t down;
    czi_coord_t right;
    int         scale;
};

#define TILE_FILENAME_MAX_SIZE 1024

struct tile {
    struct region region;
    char          filename[TILE_FILENAME_MAX_SIZE];
};

struct options {
    int filename_value_base;
};

#endif /* _TYPES_H */
