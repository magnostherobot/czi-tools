
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "types.h"
#include "macros.h"
#include "mapfile.h"

#define DEV_ZERO "/dev/zero"

/* progress bar handling */
void update_progress_bar(struct map_ctx *c) {
    static unsigned int this_pct = 0;
    static unsigned int last_pct = 0;

    this_pct = ((100 * map_file_offset(c)) / c->fsize);
    if (this_pct > last_pct) {
        last_pct = this_pct;
        dprintf(STDOUT_FILENO, "Processing input file: %3u%%\r", this_pct);
    }

    if (map_file_offset(c) == c->fsize)
        dprintf(STDOUT_FILENO, "\n");

    return;
}

/* portable (if hacky) function which does something similar to
 * posix_fallocate(3). for best results, truncate the file named by fd to zero
 * bytes beforehand by opening with O_TRUNC or using ftruncate(2) */
int xfallocate(int fd, size_t sz) {
    static int zerofd = -1;
    struct map_ctx *ctx;

    if (sz == 0)
        ferrx1("invalid argument, size parameter is set to zero");
    
    if (zerofd == -1) {
        if ((zerofd = open(DEV_ZERO, O_RDONLY)) == -1)
            ferr1("could not open " DEV_ZERO " for reading");
    }

    if ((ctx = map_mopen(DEV_ZERO, zerofd, sz, PROT_READ)) == NULL)
        ferrx1("could not map " DEV_ZERO " into memory");

    if (map_dwrite(ctx, fd, sz) == -1)
        ferrx1("could not write to output file");

    map_mclose(ctx);
    
    return 0;
}
