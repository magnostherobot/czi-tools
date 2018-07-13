
/*
 * Extract JpegXR tiles from a Zeiss CZI file.
 *
 * Written by David Miller, based on code by Callum Duff.
 */

/*
 * At present this code assumes you're running on a little-endian machine. CZI
 * stores all values in little endian order, so we can optimise using memcpy()
 * to copy data structures between memory regions.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "zeiss.h"
#include "mmap.h"
#include "json.h"

struct czi_seg_header header;

void usage() {
    dprintf(STDOUT_FILENO, "Usage: extractjxr <options>\n\n"
            "   -i <file>   input Zeiss CZI file\n"
            "   -d <dir>    directory for extracted JXR files\n"
            "   -h          print this help message\n"
            "   -j <file>   write CZI metadata to JSON file\n\n"
            " -i is mandatory, -d and -j are optional.\n"
            );
    exit(0);
}

/* process the next file segment */
int next_segment() {

    if (xread((void*) &header, sizeof(header)) == -1)
        return -1;

    switch (czi_getsegid(&header)) {
    case ZISRAWFILE:
        czi_process_zrf(&header);
        break;
    case UNKNOWN:
        errx(1, "next_segment: failed to parse segment header: %s", header.name);
        break;
    }
}

int main(int argc, char *argv[]) {
    int ch, doextract, dojson;
    int czifd, dirfd, jsonfd;
    char *czifn = NULL;
    char *dirfn = NULL;
    char *jsonfn = NULL;

    dojson = 0;
    doextract = 0;
    
    /* argument handling */
    
    while ((ch = getopt(argc, argv, "+d:hi:j:")) != -1) {
        switch (ch) {
        case 'd':
            doextract = 1;
            dirfn = optarg;
            break;
        case 'h':
            usage();
            break;
        case 'i':
            czifn = optarg;
            break;
        case 'j':
            dojson = 1;
            jsonfn = optarg;
            break;
        default:
            errx(1, "error parsing arguments (pass '-h' for help)");
        }
    }

    if (argc > optind)
        errx(1, "too many arguments (pass '-h' for help)");

    if (!czifn)
        errx(1, "missing arguments (pass '-h' for help)");

    if (!dojson && !doextract)
        errx(1, "missing arguments (one of '-d' and '-j' requried -- pass '-h' for help");
    
    if ((czifd = open(czifn, O_RDONLY)) < 0)
        err(1, "could not open input CZI file %s", czifn);

    if (mapsetup(czifd) < 0)
        errx(1, "could not initialise memory mapping information");

    if (doextract) {
        if ((dirfd = open(dirfn, O_RDONLY | O_DIRECTORY)) < 0)
            err(1, "could not open output directory %s", dirfn);

        czi_extract_setfd(dirfd);
    }

    if (dojson) {
        if ((jsonfd = open(jsonfn, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0)
            err(1, "could not open output JSON file %s", jsonfd);

        czi_json_setfd(jsonfd);
        czi_json_start();
    }
    
    while (next_segment() == 0);

    exit(0);
}





