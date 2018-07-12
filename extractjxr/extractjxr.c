

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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zeiss.h"

void usage() {
    fprintf(stdout, "Usage: extractjxr <options>\n\n"
            "   -i <file>   input Zeiss CZI file\n"
            "   -d <dir>    directory for extracted JXR files\n"
            "   -h          print this help message\n"
            "   -j <file>   write CZI metadata to JSON file\n\n"
            " -i and -d are mandatory, -j is optional.\n"
            );
    exit(0);
}

int main(int argc, char *argv[]) {
    int ch;
    int czifd;
    int dirfd;
    int jsonfd;
    int dojson = 0;
    
    char *czifn = NULL;
    char *dirfn = NULL;
    char *jsonfn = NULL;

    /* argument handling */
    
    while ((ch = getopt(argc, argv, "+d:hi:j:")) != -1) {
        switch (ch) {
        case 'd':
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
            errx(1, "Error parsing arguments (pass '-h' for help)");
        }
    }

    if (argc > optind)
        errx(1, "too many arguments (pass '-h' for help)");

    if (!czifn || !dirfn)
        errx(1, "missing arguments (pass '-h' for help)");
    
    if ((czifd = open(czifn, O_RDONLY)) < 0)
        err(1, "could not open input CZI file %s", czifn);

    if ((dirfd = open(dirfn, O_RDONLY | O_DIRECTORY)) < 0)
        err(1, "could not open output directory %s", dirfn);

    if (dojson)
        if ((jsonfd = open(jsonfn, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0)
            err(1, "could not open output JSON file %s", jsonfd);
}





