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
// For assert:
#include <assert.h>
// For bool:
#include <stdbool.h>
// For errno:
#include <errno.h>
// For vips image manipulation:
#include <vips/vips.h>

#include "debug.h"
#include "llist.h"

struct region {
    uint32_t up;
    uint32_t left;
    uint32_t down;
    uint32_t right;
};

struct tile {
    struct region region;
    char          filename[256];
};

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

bool tile_comparator(struct tile *a, struct tile *b) {
    return !(a->region.up > b->region.up
            || (a->region.up == b->region.up
                && a->region.left > b->region.left));
}

bool tile_ll_comparator(void *a, void *b) {
    return tile_comparator((void *)a, (void *)b);
}

/*
 * Finds the intersection of two regions.
 * Returns a null-pointer if the regions do not overlap.
 * WARNING: allocates memory!
 */
struct region *intersection(struct region *a, struct region *b) {
    struct region *ret = (struct region *) malloc(sizeof (struct region));

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

    return ret;
}

bool overlaps(struct region *a, struct region *b) {
    struct region *i = intersection(a, b);
    if (i) free(i);
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

struct region *get_region(struct dirent *ent, struct region *buf) {
#   define get_region_side(u, d, s) do { \
        char *filename = ent->d_name; \
        char *pos_str = strstr(filename, s "-"); \
        if (!pos_str) return NULL; \
        pos_str += strlen(s "-"); \
        char *siz_str; \
        long pos = strtol(pos_str, &siz_str, 16); \
        if (siz_str[0] != '+') return NULL; \
        siz_str++; \
        long siz = strtol(siz_str, NULL, 16); \
        u = pos; \
        d = pos + siz; \
    } while (0)
    get_region_side(buf->up,   buf->down,  "Y");
    get_region_side(buf->left, buf->right, "X");
#   undef get_region_side
}

void print_tiles(llist *list) {
    debug("%s", "--");
    for (struct ll_node *node = list; node; node = node->next) {
        debug("%s", ((struct tile *) (node->content))->filename);
    }
    debug("%s", "--");
}

llist *find_relevant_tiles(struct region *desired, char *tile_dirname) {
    llist *included_tiles = NULL;
    DIR *dir = opendir(tile_dirname);
    if (!dir) {
        perror(tile_dirname);
        exit(errno);
    }
    struct dirent *ent;
    while (ent = readdir(dir)) {
        fprintf(stderr, "checking file %s\n", ent->d_name);
        struct region tile_region;
        if (!get_region(ent, &tile_region)) continue;
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

VipsImage **get_tile_data(llist *tiles, char *tile_dirname) {
    VipsImage **ret = (typeof(ret)) malloc((sizeof (*ret)) * ll_length(tiles));
    char *fn_buf = (typeof(fn_buf)) malloc((sizeof (*fn_buf)) * 525);
    strncpy(fn_buf, tile_dirname, 256);
    char *fn_mid = fn_buf + strlen(fn_buf);
    *(fn_mid++) = '/';
    VipsImage **v = ret;
    for (struct ll_node *node = tiles; node; node = node->next) {
        struct tile *tile = (typeof(tile)) node->content;
        strcpy(fn_mid, tile->filename);
        safe_vips(*(v++) = vips_image_new_from_file(fn_buf, NULL));
    }
    return ret;
}

int tiles_across(llist *tiles) {
    if (!tiles) return 0;
    uint32_t first_y = ((struct tile *) tiles->content)->region.up;
    int i = 1;
    for (struct ll_node *node = tiles->next; node && (((struct tile *) node->content)->region.up == first_y); node = node->next) {
        ++i;
    }
    return i;
}

FILE *stitch_region(struct region *desired, char *tile_dirname) {
    llist *included_tiles = find_relevant_tiles(desired, tile_dirname);
    VipsImage **vips_in = get_tile_data(included_tiles, tile_dirname);
    VipsImage *vips_mid;
    int ntiles = ll_length(included_tiles);

    if (vips_arrayjoin(vips_in, &vips_mid,
            ntiles, "across", tiles_across(included_tiles), NULL))
        vips_error_exit(NULL);

    VipsImage *vips_out;

    // Following line mutates desired:
    move_relative(&(((struct tile *) (included_tiles->content))->region), desired);

    if (vips_crop(vips_mid, &vips_out, desired->left, desired->up,
                desired->right - desired->left, desired->down - desired->up,
                NULL))
        vips_error_exit(NULL);

    if (vips_image_write_to_file(vips_out, "/cs/scratch/trh/out.png", NULL))
        vips_error_exit(NULL);
    debug("%s", "success?");
}

int main(int argc, char **argv) {
    if (VIPS_INIT(argv[0])) vips_error_exit(NULL);

    if (argc != 6) {
        printf("Usage: %s <tile_dir> <left> <top> <width> <height>\n", argv[0]);
        exit(1);
    }
    char *tile_dirname = argv[1];
    long left   = strtol(argv[2], NULL, 10);
    long up     = strtol(argv[3], NULL, 10);
    long width  = strtol(argv[4], NULL, 10);
    long height = strtol(argv[5], NULL, 10);
    struct region des = {
        .up    = up,
        .down  = up + height,
        .left  = left,
        .right = left + width
    };
    stitch_region(&des, tile_dirname);
}
