#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <vips/vips.h>
#include <unistd.h>
#include <err.h>
#include <getopt.h>

#include "main.h"
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

    struct options opts = {
        .filename_value_base = 10
    };

    struct {
        char *up;
        char *down;
        char *left;
        char *right;
    } region_str = {
        .up    = NULL,
        .down  = NULL,
        .left  = NULL,
        .right = NULL
    };

    int base_tmp;
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
                base_tmp = strtol(optarg, NULL, 0);
                if (2 > base_tmp || base_tmp > 36) {
                    errx(1, "base of %d outwith range of 2 <= b <= 36\n",
                            opts.filename_value_base);
                }
                opts.filename_value_base = (int) base_tmp;
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
        .up    = strtol(region_str.up,    NULL, opts.filename_value_base),
        .down  = strtol(region_str.down,  NULL, opts.filename_value_base),
        .left  = strtol(region_str.left,  NULL, opts.filename_value_base),
        .right = strtol(region_str.right, NULL, opts.filename_value_base),
        .scale = scale
    };

    stitch_region(&des, tile_dirname, &opts);
}
