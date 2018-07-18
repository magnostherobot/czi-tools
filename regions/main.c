#include <stdio.h>
#include <stdlib.h>
#include <vips/vips.h>
#include <unistd.h>
#include <err.h>

#include "region.h"

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
