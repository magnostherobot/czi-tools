
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mmap.h"

#define round_up(n, t) (((n) % (t)) ? (((n) / (t)) + 1) * (t) : (n))

static int mapstarted = 0;
static long page_size;
static off_t fileoffset;
static size_t filesize;

static int czifd;
static void *cziptr;

/* set up file mapping */
int mapsetup(int fd) {
    struct stat statb;

    if ((page_size = sysconf(_SC_PAGESIZE)) < 0) {
        warn("\nmapsetup: could not get system page size");
        return -1;
    }

    if (fstat(fd, &statb) < 0) {
        warn("\nmapsetup: could not read input file size");
        return -1;
    }

    filesize = statb.st_size;
    czifd = fd;

    if ((cziptr = mmap(NULL, round_up(filesize, page_size), PROT_READ,
                       MAP_SHARED, czifd, 0)) == MAP_FAILED)
        warn("\nmapsetup: could not map memory");

    fileoffset = 0;
    mapstarted = 1;
    return 0;
}

/* read from the memory mapped file */
int xread(void *buf, size_t nbytes) {
    if (!mapstarted || (fileoffset + nbytes) > filesize)
        return -1;

    memcpy(buf, (cziptr + fileoffset), nbytes);
    fileoffset += nbytes;
    
    return 0;
}

/* mmap the nominated fd, and copy the given number of bytes from the input file
 * to the output fd */
int xwrite(int fd, size_t nbytes) {
    if (!mapstarted || (fileoffset + nbytes) > filesize) {
        warnx("\nxwrite: invalid argument");
        return -1;
    }
    
    void *ptr;

    if ((ptr = mmap(NULL, round_up(nbytes, page_size), PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0)) == MAP_FAILED) {
        warn("\nxwrite: could not map memory");
        return -1;
    }

    /* and away we go, on the way-out wacky races */
    memcpy(ptr, (cziptr + fileoffset), nbytes);
    fileoffset += nbytes;

    if (munmap(ptr, round_up(nbytes, page_size)) < 0) {
        warn("\nxwrite: could not unmap memory");
        return -1;
    }
    
    return 0;
}

/* seek forward a given number of bytes -- XXX should report errors */
void xseek_forward(off_t nbytes) {
    if (!mapstarted || (fileoffset + nbytes) > filesize)
        return;

    fileoffset += nbytes;

    return;
}

/* seek backwards a given number of bytes */
void xseek_backward(off_t nbytes) {
    if (!mapstarted || nbytes > fileoffset)
        return;

    fileoffset -= nbytes;

    return;
}

/* set offset to a given position in the file */
void xseek_set(off_t pos) {
    if (!mapstarted || pos > filesize)
        return;

    fileoffset = pos;

    return;
}

/* report current file offset */
off_t xseek_offset() {
    return fileoffset;
}

/* report file size */
size_t xseek_size() {
    return filesize;
}

