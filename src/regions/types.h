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

struct tile {
    struct region region;
    char          filename[256];
};

struct options {
    int filename_value_base;

    czi_coord_t x_offset;
    czi_coord_t y_offset;
};

#endif /* _TYPES_H */
