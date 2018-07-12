
#include <sys/mman.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mmap.h"
#include "extractjxr.h"

static int mapstarted = 0;
static void *cziptr;
static off_t mapoffset = 0;

static void startmap() {
    czifileoffset = 0;
    cziptr = mmap(NULL, MAPSIZE, PROT_READ, MAP_PRIVATE, czifd,
                  czifileoffset);

    if (cziptr == MAP_FAILED)
        err(1, "could not map memory");

    mapstarted = 1;
    czifileoffset += MAPSIZE;
    mapoffset = 0;
}

/* move our mapping forwards in the file */
static void mapforward() {
    if (munmap(cziptr, MAPSIZE) < 0)
        err(1, "could not unmap memory");

    cziptr = mmap(cziptr, MAPSIZE, PROT_READ, MAP_PRIVATE, czifd,
                  czifileoffset);

    if (cziptr == MAP_FAILED)
        err(1, "could not map memory");

    czifileoffset += MAPSIZE;
    mapoffset = 0;
}

/* memory-mapped file interface */
void xread(void *buf, size_t nbytes) {
    void *ptr = buf;
    /* map the file if not already done so */
    if (!mapstarted)
        startmap();

    /* if the read would run over the end of the mapping, we should read up to
     * the end of the mapping and then map further pages */
    while ((mapoffset + nbytes) > MAPSIZE) {
        memcpy(ptr, (cziptr + mapoffset), MAPSIZE - mapoffset);
        nbytes -= (MAPSIZE - mapoffset);
        ptr += (MAPSIZE - mapoffset);
        mapforward();
    }
    
    memcpy(ptr, (cziptr + mapoffset), nbytes);
    mapoffset += nbytes;
    
    return;
}





