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

#include "debug.h"

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

typedef struct tl_node {
    struct tile    *tile;
    struct tl_node *next;
} tile_list;

tile_list *add_tile(tile_list *list, struct tile *tile) {
    struct tl_node *node = list;
    struct tl_node *prev = NULL;
    while (node
            && node->tile->region.up   <= tile->region.up
            && node->tile->region.left <= tile->region.left) {
        prev = node;
        node = node->next;
    }
    struct tl_node *new_node = (struct tl_node *) malloc(sizeof (*new_node));
    if (!new_node) return NULL;
    new_node->tile = tile;
    new_node->next = node;
    if (!list) return new_node;
    if (prev) prev->next = new_node;
    return list;
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

FILE *stitch_region(struct region *desired, DIR *tile_directory) {
    struct dirent *ent;
    // Here, we need to find the files contained within the desired region
    // in both dimensions.
    tile_list *included_tiles = NULL;
    while (ent = readdir(tile_directory)) {
        fprintf(stderr, "checking file %s\n", ent->d_name);
        struct region tile_region;
        if (!get_region(ent, &tile_region)) continue;
        if (overlaps(&tile_region, desired)) {
            struct tile *tile = (struct tile *) malloc(sizeof (*tile));
            tile->region  = tile_region;
            strncpy(tile->filename, ent->d_name, sizeof (ent->d_name));
            included_tiles = add_tile(included_tiles, tile);
        }
    }

    // Now, we stitch all the selected parts together, in order:
    for (struct tl_node *node = included_tiles; node; node = node->next) {
        fprintf(stderr, "%s\n", node->tile->filename);
    }
}

int main() {
    struct region des = {
        .up    = 0,
        .down  = 7,
        .left  = 0,
        .right = 5
    };
    char *tile_dirname = "./test_images";

    DIR *dir = opendir(tile_dirname);
    if (!dir) {
        perror(tile_dirname);
        exit(errno);
    }
    stitch_region(&des, dir);
}
