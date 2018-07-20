
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

#include "alloc.h"
#include "macros.h"
#include "mapfile.h"
#include "types.h"

/* 
 * API notes:
 *
 * On error conditions, we print a generic message then return an error to the
 * calling context, who may print more contextual information. This code doesn't
 * make any exit calls (though the memory allocation functions might).
 */

#define round_up(n, t) (((n) % (t)) ? (((n) / (t)) + 1) * (t) : (n))
#define to_pg_bound(n) round_up(n, page_size) 

static size_t page_size;
static size_t chunklen;

#ifdef SMALL_ADDRESS
# define DEFAULT_PAGE_MULTIPLIER 1024  /* TODO: figure out sane default for both 32- and 64-bit cases */
#else
# define DEFAULT_PAGE_MULTIPLIER 1024
#endif

/* configure memory-mapped file code */
int map_configure(struct config *cfg) {
    if ((page_size = sysconf(_SC_PAGESIZE)) == -1)
        return 0;
    
    if (cfg->page_multiplier != 0)
        chunklen = cfg->page_multiplier * page_size;
    else
        chunklen = DEFAULT_PAGE_MULTIPLIER * page_size;

    return 1;
}

/* open a file and mmap it */
struct map_ctx *map_open(char *fname, int oflags, int mflags) {
    struct map_ctx *new_ctx;
    struct stat statb;
    void *ptr;
    size_t sz;
    int fd;

    if ((fd = open(fname, oflags)) == -1)
        fwarn("could not open \"%s\"", fname);

    if (fstat(fd, &statb) < 0)
        fwarn("could not stat file \"%s\"", fname);

    sz = statb.st_size;
    
    if ((ptr = mmap(NULL, to_pg_bound(sz), mflags, MAP_SHARED, fd, 0)) == MAP_FAILED)
        fwarn("could not map file \"%s\" into memory", fname);
    
    new_ctx = xmalloc(sizeof(struct map_ctx));

    new_ctx->fd = fd;
    new_ctx->fname = xstrdup(fname);
    new_ctx->fsize = sz;
    new_ctx->ptr = ptr;
    new_ctx->offset = 0;
    new_ctx->chunknum = 0;

    return new_ctx;
}

/* unmap a file and close it */
void map_close(struct map_ctx *ctx) {
    if (munmap(ctx->ptr, to_pg_bound(ctx->fsize)) < 0)
        fwarnx("could not unmap file \"%s\" (this is probably an application bug)", ctx->fname);

    if (close(ctx->fd) < 0)
        fwarn("could not close file descriptor for file \"%s\"", ctx->fname);

    xfree(ctx->fname);

    return;
}
