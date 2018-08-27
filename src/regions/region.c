// For readdir, DIR, dirent:
#include <dirent.h>
// For uint32_t:
#include <stdint.h>
// For NULL:
#include <stddef.h>
// For malloc:
#include <stdlib.h>
// For FILE:
#include <stdio.h>
// For strcpy:
#include <string.h>
// For bool:
#include <stdbool.h>
// For errno:
#include <errno.h>
// For vips image manipulation:
#include <vips/vips.h>
// For errx:
#include <err.h>
// For assert:
#include <assert.h>

#include "region.h"
#include "main.h"
#include "debug.h"
#include "llist.h"

struct tile {
    struct region region;
    char          filename[256];
};

static bool tile_comparator(struct tile *a, struct tile *b) {
    return !(a->region.up > b->region.up
            || (a->region.up == b->region.up
                && a->region.left > b->region.left));
}

static bool tile_ll_comparator(void *a, void *b) {
    return tile_comparator((void *)a, (void *)b);
}

void debug_region(const struct region *region) {
    debug("u%d, d%d, l%d, r%d, z%d\n", region->up, region->down,
            region->left, region->right, region->scale);
}

/*
 * Finds the intersection of two regions.
 * Returns a null-pointer if the regions do not overlap.
 * WARNING: allocates memory!
 */
struct region *intersection(const struct region *a, const struct region *b) {
    if (a->scale != b->scale) return NULL;

    struct region *ret = (struct region *) malloc(sizeof (struct region));
    if (!ret) return NULL;
#   define intersection_sides(s, t) do { \
        ret->s = max(a->s, b->s); \
        ret->t = min(a->t, b->t); \
        if (ret->s >= ret->t) { \
            free(ret); \
            return NULL; \
        } \
    } while (0)
    intersection_sides(up, down);
    intersection_sides(left, right);
#   undef intersection_sides
    ret->scale = a->scale;
    return ret;
}

bool overlaps(struct region *a, struct region *b) {
    debug_region(a);
    debug_region(b);
    struct region *i = intersection(a, b);
    if (i) {
        free(i);
        debug("%s\n", "yes");
    } else debug("no\n");
    return (bool) i;
}

/*
 * Moves a region relative to another "root" region.
 * The root region only requires `up` and `left` values, and maybe should be
 * replaced with a "point" data type or similar.
 */
struct region *move_relative(struct region *root, struct region *x) {
    x->up    -= root->up;
    x->down  -= root->up;
    x->left  -= root->left;
    x->right -= root->left;
    return x;
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
     * While the ordering of values in a filename has been agreed, this has
     * been written to work with values in any order, just in case.
     */
#   define place(str, x) (strstr((str), (x)) + strlen((x)))
    char *dim_start = place(filename,id);

    char *left_start = place(dim_start, "p");
    *left = str_czi_coord(left_start, NULL, base);

    if (right) {
        char *size_start = place(dim_start, "s");
        *right = *left + str_czi_coord(size_start, NULL, base);
    }

    if (scale) {
        char *scale_start = place(dim_start, "r");
        czi_coord_t scale_tmp = str_czi_coord(scale_start, NULL, base);
        assert(scale_tmp <= INT_MAX);
        *scale = (int) scale_tmp;
    }

    return 0;
#   undef place
}

int get_region(struct dirent *ent, struct region *buf, struct options *opts) {
    set_side(ent, "X", opts, &(buf->left), &(buf->right), &(buf->scale));

    int scale_tmp;
    set_side(ent, "Y", opts, &(buf->up),   &(buf->down),  &scale_tmp);
    assert(buf->scale == scale_tmp);

    return 0;
}

void print_tiles(llist *list) {
    debug("%s\n", "--");
    for (struct ll_node *node = list; node; node = node->next) {
        debug("%s\n", ((struct tile *) (node->content))->filename);
    }
    debug("%s\n", "--");
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
        if (!get_region(ent, &tile_region, opts)) continue;
        if (tile_region.scale != desired->scale) continue;
        if (overlaps(&tile_region, desired)) {
            struct tile *tile = (struct tile *) malloc(sizeof (*tile));
            tile->region = tile_region;
            strncpy(tile->filename, ent->d_name, strlen(ent->d_name)+1);
            included_tiles = ll_add_item(included_tiles, tile, &tile_ll_comparator);
        }
    }
    return included_tiles;
}

#define safe_vips(...) if (!(__VA_ARGS__)) vips_error_exit(NULL)

struct filenamedata {
    char       *start;
    char       *mid;
    VipsImage **v;
};

bool get_one_tile(struct tile *tile, struct filenamedata *data) {
    strcpy(data->mid, tile->filename);
    if (!(*((data->v)++) = vips_image_new_from_file(data->start, NULL)))
        vips_error_exit(NULL);
    return true;
}

bool ll_get_one_tile(void *tile, void *data) {
    return get_one_tile((struct tile *) tile, (struct filenamedata *) data);
}

VipsImage **get_tile_data(llist *tiles, char *tile_dirname) {
    VipsImage **ret = (__typeof__(ret)) malloc((sizeof (*ret)) * ll_length(tiles));
    char *fn_buf = (__typeof__(fn_buf)) malloc((sizeof (*fn_buf)) * 525);
    strncpy(fn_buf, tile_dirname, 256);
    char *fn_mid = fn_buf + strlen(fn_buf);
    *(fn_mid++) = '/';
    VipsImage **v = ret;
    struct filenamedata fnd = {
        .start = fn_buf, .mid = fn_mid, .v = v
    };
    ll_foreach(tiles, &ll_get_one_tile, &fnd);
    return ret;
}

int tiles_across(llist *tiles) {
    if (!tiles) return 0;
    uint32_t first_y = ((struct tile *) tiles->content)->region.up;
    int i = 1;
    for (struct ll_node *node = tiles->next; node && (((struct tile *)
                    node->content)->region.up == first_y); node = node->next) {
        ++i;
    }
    return i;
}

static bool print_filename(void *tile, void *data) {
    debug("%s\n", ((struct tile *) tile)->filename);
    return true;
}

void debug_llist(llist *list) {
    ll_foreach(list, &print_filename, NULL);
}

void stitch_region(struct region *desired, char *tile_dirname,
        struct options *opts) {
    debug_region(desired);
    llist *included_tiles = find_relevant_tiles(desired, tile_dirname, opts);
    VipsImage **vips_in = get_tile_data(included_tiles, tile_dirname);
    VipsImage *vips_mid;
    int ntiles = ll_length(included_tiles);

    if (ntiles == 0) {
        errx(1, "no tiles found for specified region");
    }

    if (vips_arrayjoin(vips_in, &vips_mid, ntiles, "across",
                tiles_across(included_tiles), NULL)) {
        vips_error_exit(NULL);
    }

    VipsImage *vips_out;

    // Following line mutates desired:
    move_relative(&(((struct tile *) (included_tiles->content))->region), desired);
    debug_region(desired);
    debug_llist(included_tiles);


    if (vips_crop(vips_mid, &vips_out, desired->left, desired->up,
                desired->right - desired->left, desired->down - desired->up,
                NULL))
        vips_error_exit(NULL);

    if (vips_image_write_to_file(vips_out, "./out.png", NULL))
        vips_error_exit(NULL);
    debug("%s\n", "success?");
}
