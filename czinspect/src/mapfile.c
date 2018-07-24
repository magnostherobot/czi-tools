
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
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

#define round_up(n, t)         (((n) % (t)) ? (((n) / (t)) + 1) * (t) : (n))
#define to_pg_bound(n)         round_up(n, page_size)

static size_t page_size;
static size_t def_chunklen;

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
        def_chunklen = cfg->page_multiplier * page_size;
    else
        def_chunklen = DEFAULT_PAGE_MULTIPLIER * page_size;

    return 1;
}

/* open a file and mmap it. depending on the flags, we may perform the open() ourselves */
struct map_ctx *_map_open(char *fname, int nfd, size_t nsize, int oflags, int mflags) {
    struct map_ctx *new_ctx;
    struct stat statb;
    void *ptr;
    size_t sz;
    size_t chunklen;
    int fd;

    if (nfd == -1) {
        if ((fd = open(fname, oflags)) == -1)
            return fwarn("could not open \"%s\"", fname), NULL;
    } else {
        fd = nfd;
    }

    if (nsize == 0) {
        if (fstat(fd, &statb) < 0)
            return fwarn("could not stat file \"%s\"", fname), NULL;

        sz = statb.st_size;
    } else {
        sz = nsize;
    }

    
    chunklen = (sz < def_chunklen ? to_pg_bound(sz) : def_chunklen);
    
    if ((ptr = mmap(NULL, chunklen, mflags, MAP_SHARED, fd, 0)) == MAP_FAILED)
        return fwarn("could not map file \"%s\" into memory", fname), NULL;
    
    new_ctx = xmalloc(sizeof(struct map_ctx));

    new_ctx->fd = fd;
    new_ctx->fname = xstrdup(fname);
    new_ctx->fsize = sz;
    new_ctx->mflags = mflags;
    new_ctx->ptr = ptr;
    new_ctx->offset = 0;
    new_ctx->chunknum = 0;
    new_ctx->chunklen = chunklen;

    return new_ctx;
}

/* unmap a file, close the descriptor if the flag is set */
void _map_close(struct map_ctx *ctx, int flag) {
    if (munmap(ctx->ptr, ctx->chunklen) < 0)
        fwarnx("could not unmap file \"%s\" (this is probably an application bug)", ctx->fname);

    xfree(ctx->fname);
    xfree(ctx);

    if (flag && close(ctx->fd) < 0)
        fwarn("could not close file descriptor for file \"%s\"", ctx->fname);

    return;
}

/* move our mapping to the specified chunk; resets the offset pointer to zero */
static int map_move(struct map_ctx *ctx, size_t cnum) {

    if (munmap(ctx->ptr, ctx->chunklen) < 0)
        return fwarnx(
            "could not unmap file \"%s\" (this is probably an application bug)",
            ctx->fname), -1;

    if ((ctx->ptr = mmap(ctx->ptr,
                         ctx->chunklen,
                         ctx->mflags,
                         MAP_SHARED,
                         ctx->fd,
                         cnum * ctx->chunklen)) == MAP_FAILED)
        return fwarn("could not remap file \"%s\" into memory", ctx->fname), -1;

    ctx->chunknum = cnum;
    ctx->offset = 0;

    return 0;
}

/* move the file offset, possibly remapping the file */
int map_seek(struct map_ctx *ctx, size_t offset, int whence) {
    size_t target, targchunk;

    /* calculate the file offset to which we're jumping */
    switch (whence) {
    case MAP_SET:
        if (offset > ctx->fsize)
            return fwarnx("invalid seek offset of %" PRIi64, offset), -1;
        target = offset;
        break;
    case MAP_FORW:
        if (map_file_remaining(ctx) + offset > ctx->fsize)
            return fwarnx("invalid seek offset of %" PRIi64, offset), -1;
        target = map_file_offset(ctx) + offset;
        break;
    case MAP_BACK:
        if (offset > map_file_offset(ctx))
            return fwarnx("invalid seek offset of %" PRIi64, offset), -1;
        target = map_file_offset(ctx) - offset;
        break;
    default:
        fwarnx1("unknown \"whence\" flag (this is probably an application bug)");
        return -1;
    }

    /* calculate target offset's chunk number; note integer division with discarded remainder */
    targchunk = target / ctx->chunklen;

    if (targchunk != ctx->chunknum)
        if (map_move(ctx, targchunk) == -1)
            return -1;

    /* set offset pointer */
    ctx->offset = target % ctx->chunklen;

    return 0;
}

/* read memory from mapping into the nominated buffer */
int map_read(struct map_ctx *ctx, void *buf, size_t len) {

    if (len > map_file_remaining(ctx))
        return fwarnx("cannot read %" PRIu64 " bytes from \"%s\"", len, ctx->fname), -1;

    while ((ctx->offset + len) > ctx->chunklen) {
        memcpy(buf, map_chunk_ptr(ctx), map_chunk_remain(ctx));

        buf += map_chunk_remain(ctx);
        len -= map_chunk_remain(ctx);

        if (map_move(ctx, ctx->chunknum + 1) == -1) /* map next chunk */
            return -1;
    }

    memcpy(buf, map_chunk_ptr(ctx), len);
    ctx->offset += len;

    return 0;
}

/* loop for writing to a file descriptor */
static int write_loop(int fd, void *buf, size_t len) {
    ssize_t rv;

    while ((rv = write(fd, buf, len)) != 0) {
        if (rv == -1) {
            if (errno == EINTR || errno == EAGAIN)
                continue;

            return fwarn("cannot write %" PRIu64" bytes to file descriptor %u",
                         len, fd), -1;
        }

        buf += rv;
        len -= rv;
    }

    return 0;
}

/* read memory from the mapping and write to file descriptor */
int map_dwrite(struct map_ctx *ctx, int fd, size_t len) {

    if (len > map_file_remaining(ctx))
        return fwarnx("cannot read %" PRIu64 " bytes from \"%s\"", len, ctx->fname), -1;

    while ((ctx->offset + len) > ctx->chunklen) {
        if (write_loop(fd, map_chunk_ptr(ctx), map_chunk_remain(ctx)) == -1)
            return -1;

        len -= map_chunk_remain(ctx);
        ctx->offset += map_chunk_remain(ctx);

        if (map_move(ctx, ctx->chunknum + 1) == -1)
            return -1;
    }

    if (write_loop(fd, map_chunk_ptr(ctx), len) == -1) return -1;
    ctx->offset += len;

    return 0;
}

/* read memory from one mapping into another mapping */
int map_splice(struct map_ctx *from, struct map_ctx *to, size_t len) {
    struct map_ctx *least; /* mapped file with least available chunk space */
    size_t adv;            /* how many bytes to advance in the loop */
    
    if (len > map_file_remaining(from) || len > map_file_remaining(to))
        return fwarnx("cannot copy %" PRIu64 " bytes from \"%s\" to \"%s\"",
                      len, from->fname, to->fname), -1;

    while ((from->offset + len) > from->chunklen || (to->offset + len) > to->chunklen) {
        least = (map_chunk_remain(from) > map_chunk_remain(to) ? to : from);
        adv = map_chunk_remain(least);

        /* we cannot use the map_chunk_remain() macro for incrementing chunk
         * offsets as it uses this information itself */
        memcpy(map_chunk_ptr(to), map_chunk_ptr(from), adv);

        from->offset += adv;
        to->offset += adv;
        len -= adv;

        if (map_move(least, least->chunknum + 1) == -1)
            return -1;
    }

    memcpy(map_chunk_ptr(to), map_chunk_ptr(from), len);
    from->offset += len;
    to->offset += len;

    return 0;
}

