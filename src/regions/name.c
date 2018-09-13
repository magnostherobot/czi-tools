#include <string.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include "name.h"
#include "debug.h"

/*
 * Like strstr(3), but points to the end of the needle rather than the start.
 */
char *strstrend(const char *str, const char *needle) {
    char *mid = strstr(str, needle);
    if (!mid) {
        return 0;
    }
    return mid + strlen(needle);
}

czi_coord_t str_czi_coord(char* str, char **end, int base) {
    int errno_tmp = errno;
    errno = 0;
    czi_coord_t ret = strtol(str, end, base);
    if (errno) {
        err(errno, NULL);
    }
    errno = errno_tmp;
    return ret;
}

int set_side(struct dirent *ent, char *id, struct options *opts,
        czi_coord_t *left, czi_coord_t *right, int *scale) {
    char *filename = ent->d_name;
    int base = opts->filename_value_base;

    /*
     * The following lines find the dimension, then the dimension's values. A
     * problem could occur if a value hasn't been speicified for a dimension,
     * in which case this would find the value of a different dimension.
     *
     * While the ordering of values in a dimension has been agreed, this has
     * been written to work with values in any order, just in case.
     */
    char *dim_start = strstrend(filename, id);
    if (!dim_start) return 1;

    char *left_start = strstrend(dim_start, "p");
    if (!left_start) return 2;
    *left = str_czi_coord(left_start, NULL, base);

    if (right) {
        char *size_start = strstrend(dim_start, "s");
        if (!size_start) return 3;
        *right = *left + str_czi_coord(size_start, NULL, base);
    }

    if (scale) {
        char *scale_start = strstrend(dim_start, "r");
        if (!scale_start) return 4;

        czi_coord_t scale_tmp = str_czi_coord(scale_start, NULL, base);
        assert(scale_tmp <= INT_MAX);
        *scale = (int) scale_tmp;
    }

    return 0;
}

int get_region(struct dirent *ent, struct region *buf, struct options *opts) {
    int ss_ret;

    ss_ret = set_side(ent, "X", opts, &(buf->left), &(buf->right),
            &(buf->scale));
    if (ss_ret) return ss_ret;

    int scale_tmp;
    ss_ret = set_side(ent, "Y", opts, &(buf->up), &(buf->down), &scale_tmp);
    if (ss_ret) return ss_ret;
    assert(buf->scale == scale_tmp);

    return 0;
}

int find_smallest(char *tile_dirname, czi_coord_t *x, czi_coord_t *y,
        char **filename, struct options *opts) {
    DIR *dir = opendir(tile_dirname);
    if (!dir) err(errno, "%s", tile_dirname);

    struct dirent *ent = readdir(dir);
    if (!ent) return 1;

    bool first_pass = true;

    czi_coord_t min_x = 0;
    czi_coord_t min_y = 0;
    char *min_fn = NULL;
    while ((ent = readdir(dir))) {
        czi_coord_t tmp_x, tmp_y;

        if (set_side(ent, "X", opts, &tmp_x, NULL, NULL)) continue;
        if (set_side(ent, "Y", opts, &tmp_y, NULL, NULL)) continue;

        if (first_pass) {
            min_x = tmp_x;
            min_y = tmp_y;
            min_fn = ent->d_name;
            first_pass = false;
        } else {
            if (min_x > tmp_x) {
                min_x = tmp_x;
                min_fn = ent->d_name;
            }
            if (min_y > tmp_y) {
                min_y = tmp_y;
                min_fn = ent->d_name;
            }
        }
    }

    if (!min_fn) return 2;

    if (x) *x = min_x;
    if (y) *y = min_y;
    if (filename) *filename = min_fn;

    return 0;
}

static bool tile_comparator(struct tile *a, struct tile *b) {
    return !(a->region.up > b->region.up
            || (a->region.up == b->region.up
                && a->region.left > b->region.left));
}

static bool tile_ll_comparator(void *a, void *b) {
    return tile_comparator((void *)a, (void *)b);
}

llist *find_relevant_tiles(struct region *desired, char *tile_dirname,
        struct options *opts) {
    llist *included_tiles = NULL;
    DIR *dir = opendir(tile_dirname);
    if (!dir) {
        err(errno, "%s", tile_dirname);
    }

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        struct region tile_region;
        if (get_region(ent, &tile_region, opts)) continue;
        if (tile_region.scale != desired->scale) continue;
        if (overlaps(&tile_region, desired)) {
            struct tile *tile = (struct tile *) malloc(sizeof (*tile));
            tile->region = tile_region;
            strncpy(tile->filename, ent->d_name, TILE_FILENAME_MAX_SIZE);
            included_tiles = ll_add_item(included_tiles, tile, &tile_ll_comparator);
        }
    }
    return included_tiles;
}
