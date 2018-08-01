
#include "config.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "alloc.h"
#include "macros.h"
#include "mapfile.h"
#include "util.h"
#include "zeiss.h"
#include "zeissio.h"

#define DEV_ZERO "/dev/zero"

/* portable (if hacky) function which does something similar to
 * posix_fallocate(3). for best results, truncate the file named by fd to zero
 * bytes beforehand by opening with O_TRUNC or using ftruncate(2) */
int xfallocate(int fd, size_t sz) {
    static int zerofd = -1;
    struct map_ctx *ctx;

    if (sz == 0)
        return fwarnx1("invalid argument, size parameter is set to zero"), -1;
    
    if (zerofd == -1) {
        if ((zerofd = open(DEV_ZERO, O_RDONLY)) == -1)
            return fwarn1("could not open " DEV_ZERO " for reading"), -1;
    }

    if ((ctx = map_mopen(DEV_ZERO, zerofd, sz, PROT_READ)) == NULL)
        return fwarnx1("could not map " DEV_ZERO " into memory"), -1;

    if (map_dwrite(ctx, fd, sz) == -1) {
        fwarnx1("could not write to output file");
        map_mclose(ctx);
        return -1;
    }

    map_mclose(ctx);
    return 0;
}

uint32_t get_subsample_level(lzbuf dims, uint32_t ndims) {
    /* we only filter on the X and Y dimensions */
    char x[4] = "X\0\0\0";
    char y[4] = "Y\0\0\0";
    uint32_t xratio = 0, yratio = 0;
    struct czi_subblock_dimentry *entry;

    for (uint32_t i = 0; i < ndims; i++) {
        entry = &lzbuf_get(struct czi_subblock_dimentry, dims, i);
        if (memcmp(entry->dimension, x, 4) == 0) {
            if (xratio) {
                fwarnx1("directory entry contains duplicate X dimension, skipping dimension");
                continue;
            }
            xratio = czi_subblock_dim_ratio(entry);
        } else if (memcmp(entry->dimension, y, 4) == 0) {
            if (yratio) {
                fwarnx1("directory entry contains duplicate Y dimension, skipping dimension");
                continue;
            }
            yratio = czi_subblock_dim_ratio(entry);
        }
    }

    if (xratio == 0) {
        fwarnx1("directory entry missing X dimension, skipping entry");
        return 0;
    }

    if (yratio == 0) {
        fwarnx1("directory entry missing Y dimension, skipping entry");
        return 0;
    }

    if (xratio != yratio) {
        fwarnx1("directory entry does not have equal X and Y dimensions, skipping entry");
        return 0;
    }

    return xratio;
}

static void scan_res(lzbuf dims, uint32_t ndims, lzbuf ints, uint32_t *nints) {
    uint32_t res;

    res = get_subsample_level(dims, ndims);
    if (res == 0)
        return;

    for (uint32_t i = 0; i < *nints; i++) {
        if (res == lzbuf_get(uint32_t, ints, i))
            return;
    }

    if (*nints >= ints->len)
        lzbuf_grow(uint32_t, ints);

    lzbuf_get(uint32_t, ints, *nints) = res;
    *nints += 1;
    
    return;
}

/* scan a .czi file for subsampling levels. the mapping context provided must
 * point to the beginning of the ZISRAWDIRECTORY segment and the dynamic buffer
 * will be used for storing uint32_t */
int make_reslist(struct map_ctx *c, lzbuf list, uint32_t *num) {
    struct czi_seg_header head;
    struct czi_directory dir;
    struct czi_subblock_direntry sdir;
    struct czi_subblock_dimentry *entry;
    lzbuf dimensions;

    if (czi_read_sh(c, &head) == -1)
        return fwarnx1("could not read segment header"), -1;

    if (czi_getsegid(&head) != ZISRAWDIRECTORY)
        return fwarnx1("incorrect directory segment id"), -1;
    
    if (czi_read_directory(c, &dir) == -1)
        return fwarnx1("could not read directory segment"), -1;

    if (dir.entry_count == 0)
        return fwarnx1("directory entry segment contains no entries"), -1;

    dimensions = lzbuf_new(struct czi_subblock_dimentry);
    *num = 0;
    
    for (uint32_t i = 0; i < dir.entry_count; i++) {
        if (czi_read_sblk_direntry(c, &sdir) == -1)
            return fwarnx("could not read directory entry #%" PRIu32, i + 1), -1;

        if (sdir.dimension_count == 0) {
            fwarnx("directory entry #%" PRIu32 " has no dimension entries, continuing", i + 1);
            continue;
        }

        while (lzbuf_elems(struct czi_subblock_dimentry, dimensions) < sdir.dimension_count)
            lzbuf_grow(struct czi_subblock_dimentry, dimensions);

        for (uint32_t j = 0; j < sdir.dimension_count; j++) {
            entry = &lzbuf_get(struct czi_subblock_dimentry, dimensions, j);

            if (czi_read_sblk_dimentry(c, entry) == -1)
                return fwarnx1("could not read subblock dimension entries"), -1;
        }
        
        scan_res(dimensions, sdir.dimension_count, list, num);

        lzbuf_zero(dimensions);
    }

    lzbuf_free(dimensions);
    
    return 0;
}
