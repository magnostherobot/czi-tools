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
// For getopt:
#include <unistd.h>
// For errx:
#include <err.h>

#include "debug.h"
#include "llist.h"

struct region {
    uint32_t up;
    uint32_t left;
    uint32_t down;
    uint32_t right;
    int      scale;
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

void debug_region(const struct region *region) {
    debug("u%d, d%d, l%d, r%d, z%d", region->up, region->down, region->left, region->right, region->scale);
}

/*
 * Finds the intersection of two regions.
 * Returns a null-pointer if the regions do not overlap.
 * WARNING: allocates memory!
 */
struct region *intersection(struct region *a, struct region *b) {
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

int get_region(struct dirent *ent, struct region *buf) {
#   define get_region_side(u, d, r, s) do { \
        char *filename = ent->d_name; \
        char *pos_str = strstr(filename, s "p"); \
        if (!pos_str) return 1; \
        pos_str += strlen(s "p"); \
        char *siz_str; \
        long pos = strtol(pos_str, &siz_str, 10); \
        if (siz_str[0] != 's') return 1; \
        char *rat_str; \
        long siz = strtol(++siz_str, &rat_str, 10); \
        long rat; \
        if (rat_str[0] != 'r') rat = 1; \
        else rat = strtol(++rat_str, NULL, 10); \
        u = pos; \
        d = pos + siz; \
        r = rat; \
    } while (0)
    get_region_side(buf->up,   buf->down,  buf->scale, "Y");
    get_region_side(buf->left, buf->right, buf->scale, "X");
#   undef get_region_side
    return 0;
}

void print_tiles(llist *list) {
    debug("%s", "--");
    for (struct ll_node *node = list; node; node = node->next) {
        debug("%s", ((struct tile *) (node->content))->filename);
    }
    debug("%s", "--");
}

llist *find_relevant_tiles(
        struct region *desired, char *tile_dirname) {
    llist *included_tiles = NULL;
    DIR *dir = opendir(tile_dirname);
    if (!dir) {
        err(errno, tile_dirname);
    }
    struct dirent *ent;
    while ((ent = readdir(dir))) {
        struct region tile_region;
        if (!get_region(ent, &tile_region)) continue;
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
        .start = fn_buf, .mid = fn_mid, v = v
    };
    ll_foreach(tiles, &ll_get_one_tile, &fnd);
    return ret;
}

int tiles_across(llist *tiles) {
    if (!tiles) return 0;
    uint32_t first_y = ((struct tile *) tiles->content)->region.up;
    int i = 1;
    debug("%d", first_y);
    for (struct ll_node *node = tiles->next; node && (((struct tile *) node->content)->region.up == first_y); node = node->next) {
        ++i;
    }
    return i;
}

bool print_filename(void *tile, void *data) {
    debug("%s", ((struct tile *) tile)->filename);
    return true;
}

void debug_llist(llist *list) {
    ll_foreach(list, &print_filename, NULL);
}

void stitch_region(struct region *desired, char *tile_dirname) {
    debug_region(desired);
    llist *included_tiles = find_relevant_tiles(desired, tile_dirname);
    VipsImage **vips_in = get_tile_data(included_tiles, tile_dirname);
    VipsImage *vips_mid;
    int ntiles = ll_length(included_tiles);

    if (ntiles == 0)
        errx(1, "no tiles found for specified region");

    if (vips_arrayjoin(vips_in, &vips_mid,
            ntiles, "across", tiles_across(included_tiles), NULL))
        vips_error_exit(NULL);

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
    debug("%s", "success?");
}

void usage(char *prog) {
    printf("Usage: %s <options>\n\n"
            "Mandatory options:\n"
            "   -i <dir>     input directory of images\n"
            "   -o <file>    output file name\n"
            "   -u <integer> top of region (inclusive)\n"
            "   -d <integer> bottom of region (exclusive)\n"
            "   -l <integer> leftmost edge of region (inclusive)\n"
            "   -r <integer> rightmost edge of region (exclusive)\n"
            "Optional options:\n"
            "   -h           prints this help text\n"
            "   -z <integer> scale for output image (default 1)\n"
            "   -b <integer> base for input coordinates (default 10)\n"
            , prog);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (VIPS_INIT(argv[0])) vips_error_exit(NULL);

    char *tile_dirname = NULL;
    char *output_name = NULL;
    int scale = 1;
    struct {
        char *up;
        char *down;
        char *left;
        char *right;
        int base;
    } region_str = {
        .up    = NULL,
        .down  = NULL,
        .left  = NULL,
        .right = NULL,
        .base  = 10
    };

    int ch;
    while ((ch = getopt(argc, argv, "+i:o:u:d:l:r:z:b:h")) != -1) {
        switch (ch) {
            case 'i':
                tile_dirname = optarg;
                break;
            case 'o':
                output_name = optarg;
                break;
            case 'u':
                region_str.up = optarg;
                break;
            case 'd':
                region_str.down = optarg;
                break;
            case 'l':
                region_str.left = optarg;
                break;
            case 'r':
                region_str.right = optarg;
                break;
            case 'z':
                scale = strtol(optarg, NULL, 10);
                break;
            case 'b':
                region_str.base = strtol(optarg, NULL, 10);
                if (0 >= region_str.base || region_str.base > 36)
                    errx(1, "base of %d outwith range of 1 <= b <= 36\n",
                            region_str.base);
                break;
            case 'h':
                usage(argv[0]);
                break;
            default:
                errx(1, "unknown option '%c'; use '%s -h' for help\n",
                        ch, argv[0]);
        }
    }

    if (!tile_dirname)
        errx(1, "missing input directory name ('-i'); use '%s -h' for help\n",
                argv[0]);

    if (!output_name)
        errx(1, "missing output file name ('-o'); use '%s -h' for help\n",
                argv[0]);

    if (!region_str.up
            || !region_str.down
            || !region_str.left
            || !region_str.right)
        errx(1, "Missing dimension parameter; use '%s -h' for help\n", argv[0]);

    struct region des = {
        .up    = strtol(region_str.up,    NULL, region_str.base),
        .down  = strtol(region_str.down,  NULL, region_str.base),
        .left  = strtol(region_str.left,  NULL, region_str.base),
        .right = strtol(region_str.right, NULL, region_str.base),
        .scale = scale
    };

    stitch_region(&des, tile_dirname);
}
