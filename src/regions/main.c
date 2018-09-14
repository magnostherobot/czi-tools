#include "config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vips/vips.h>
#include <unistd.h>
#include <err.h>
#include <getopt.h>
#include <errno.h>

#include "region.h"
#include "name.h"
#include "debug.h"

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
            "   -0           treat top-left of whole image as coord (0,0)\n"
            "   -x <integer> adds offset to x-direction (adds to -0)\n"
            "   -y <integer> adds offset to y-direction (adds to -0)\n"
            , prog);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (VIPS_INIT(argv[0])) vips_error_exit(NULL);

    char *tile_dirname = NULL;
    char *output_name = NULL;
    int scale = 1;

    czi_coord_t x_off = 0;
    czi_coord_t y_off = 0;

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

    bool coords_from_zero = false;
    int base_tmp;
    int ch;
    while ((ch = getopt(argc, argv, "+i:o:u:d:l:r:z:b:h0x:y:")) != -1) {
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
                            base_tmp);
                }
                opts.filename_value_base = (int) base_tmp;
                break;

            case 'h':
                usage(argv[0]);
                break;

            case '0':
                coords_from_zero = true;
                break;

            case 'x':
                x_off += strtol(optarg, NULL, 0);
                break;

            case 'y':
                y_off += strtol(optarg, NULL, 0);
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

    if (coords_from_zero) {
        czi_coord_t off_x, off_y;
        if (find_smallest(tile_dirname, &off_x, &off_y, NULL, &opts))
            errx(errno, "Error when calculating offsets");
        debug("Calculated offset is " CZI_T ", " CZI_T "\n", off_x, off_y);
        x_off += off_x;
        y_off += off_y;
    }

    struct region des = {
        .up    = strtol(region_str.up,    NULL, opts.filename_value_base),
        .down  = strtol(region_str.down,  NULL, opts.filename_value_base),
        .left  = strtol(region_str.left,  NULL, opts.filename_value_base),
        .right = strtol(region_str.right, NULL, opts.filename_value_base),
        .scale = scale
    };

    if (offset(&des, x_off, y_off))
        errx(1, "unable to use offset");

    stitch_region(&des, tile_dirname, output_name, &opts);
}
