
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mmap.h"

#define MAPSIZE (page_size * 4)        /* number of pages to be mapped at one time */
static long page_size;
static off_t czifileoffset;  /* number of bytes read from file */
static long czifilesize;     /* file size */

static int czifd;

static int mapstarted = 0;   /* mapping started flag */
static void *cziptr;         /* mapping pointer */
static off_t mapoffset = 0;  /* number of bytes read in the current mapping */
static off_t nextchunk = 0;  /* starting offset of the next mapping chunk to read */

int mapsetup(int fd) {
    struct stat statb;

    if ((page_size = sysconf(_SC_PAGESIZE)) < 0) {
        warn("mapsetup: could not get system page size");
        return -1;
    }

    if (fstat(fd, &statb) < 0) {
        warn("mapsetup: could not read input file size");
        return -1;
    }

    czifilesize = statb.st_size;
    
    czifd = fd;
    return 0;
}

static void startmap() {
    czifileoffset = nextchunk = 0;
    cziptr = mmap(NULL, MAPSIZE, PROT_READ, MAP_PRIVATE, czifd,
                  nextchunk);

    if (cziptr == MAP_FAILED)
        err(1, "startmap: could not map memory");

    mapstarted = 1;
    nextchunk += MAPSIZE;
    mapoffset = 0;
}

/* move our mapping forwards in the file */
static void mapforward() {
    if (munmap(cziptr, MAPSIZE) < 0)
        err(1, "mapforward: could not unmap memory");

    cziptr = mmap(cziptr, MAPSIZE, PROT_READ, MAP_PRIVATE, czifd,
                  nextchunk);

    if (cziptr == MAP_FAILED)
        err(1, "mapforward: could not map memory");

    nextchunk += MAPSIZE;
    mapoffset = 0;
}

/* move our mapping backwards in the file */
static void mapbackward() {
    if (munmap(cziptr, MAPSIZE) < 0)
        err(1, "mapforward: could not unmap memory");

    nextchunk -= (2 * MAPSIZE);
    
    cziptr = mmap(cziptr, MAPSIZE, PROT_READ, MAP_PRIVATE, czifd,
                  nextchunk);

    if (cziptr == MAP_FAILED)
        err(1, "mapforward: could not map memory");

    mapoffset = MAPSIZE;
}

/* memory-mapped file interface */
int xread(void *buf, size_t nbytes) {
    void *ptr = buf;

    if ((czifileoffset + nbytes) > czifilesize)
        return -1;
    
    /* map the file if not already done so */
    if (!mapstarted)
        startmap();

    /* if the read would run over the end of the mapping, we should read up to
     * the end of the mapping and then map further pages */
    while ((mapoffset + nbytes) > MAPSIZE) {
        memcpy(ptr, (cziptr + mapoffset), MAPSIZE - mapoffset);
        nbytes -= (MAPSIZE - mapoffset);
        ptr += (MAPSIZE - mapoffset);
        czifileoffset += (MAPSIZE - mapoffset);
        mapforward();
    }
    
    memcpy(ptr, (cziptr + mapoffset), nbytes);
    czifileoffset += nbytes;
    mapoffset += nbytes;
    
    return 0;
}

/* seek forward a given number of bytes -- this does not report errors */
void xseek_forward(off_t nbytes) {

    if ((czifileoffset + nbytes) > czifilesize)
        return;

    if (!mapstarted)
        startmap();

    while ((mapoffset + nbytes) > MAPSIZE) {
        nbytes -= (MAPSIZE - mapoffset);
        czifileoffset += (MAPSIZE - mapoffset);
        mapforward();
    }

    czifileoffset += nbytes;
    mapoffset += nbytes;

    return;
}

/* seek backwards a given number of bytes */
void xseek_backward(off_t nbytes) {
    if (nbytes > czifileoffset)
        return;

    if (!mapstarted)
        return;

    while (nbytes > mapoffset) {
        nbytes -= MAPSIZE;
        czifileoffset -= MAPSIZE;
        mapbackward();
    }

    czifileoffset -= nbytes;
    mapoffset -= nbytes;

    return;
}

/* seek to a given position within the file */
void xseek_set(off_t pos) {
    if (pos > czifilesize)
        return;
    else if (pos == czifileoffset)
        return;

    if (pos > czifileoffset)
        xseek_forward(pos - czifileoffset);
    else
        xseek_backward(czifileoffset - pos);
    
    return;
}

/* report current file offset */
off_t xseek_offset() {
    return czifileoffset;
}



