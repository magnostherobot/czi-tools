#ifndef _TYPES_H
#define _TYPES_H

#include <sys/types.h>
#include <stdint.h>

struct map_ctx {
    /* in effect, this is segment + offset addressing. our overall file offset
     * can be calculated by multiplying the chunk number with the chunk length
     * and adding the mapping offset. */

    int     fd;        /* file descriptor associated with a mapping */
    char   *fname;     /* name of the underlying file */
    size_t  fsize;     /* size of the underlying file */
    int     mflags;    /* flags to pass to mmap */
    void   *ptr;       /* pointer to the beginning of the mapping */
    size_t  offset;    /* offset within the current mapping -- not off_t as that's a signed type */
    size_t  chunknum;  /* chunk number within the file */
    size_t  chunklen;  /* chunk length for this mapping */
};

struct config {
    /* global options */
    char *infile;
    uint16_t operation;
    uint8_t help;
    uint8_t progress_bar;
    struct map_ctx *inctx;
    size_t page_multiplier;

    /* extraction options */
    char *outdir;

    /* dumping options */
    char *outfile;
};


#endif /* _TYPES_H */
