
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
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "zeiss.h"
#include "mmap.h"
#include "json.h"
#include "macros.h"


extern long long strtonum(const char *, long long, long long, const char **);

static int czifd, dirfd, jsonfd;
static int dojson, doextract, doscan;

void usage() {
    dprintf(STDOUT_FILENO, "Usage: extractjxr <options>\n\n"
            "   -i <file>   input Zeiss CZI file\n"
            "   -d <dir>    directory for extracted JXR files\n"
            "   -h          print this help message\n"
            "   -j <file>   write CZI metadata to JSON file\n"
            "   -f <num>    filter extracted images to only extract those with the given subsampling level\n"
            "   -s          scan for available tile subsampling levels (lower resolutions)\n\n"
            "-i is mandatory; at least one of -j, -d and -s are required.\n"
            );
    exit(0);
}

void status_update() {
#define STATUS_STRING   "Processing input file: %3lu%%"
    static long this_pct = 0;
    static long last_pct = 0;
    static int started = 0;
    int ret;

    this_pct = ((100 * xseek_offset()) / xseek_size());
    if (this_pct > last_pct) {
        last_pct = this_pct;
        ret = dprintf(STDOUT_FILENO, STATUS_STRING, this_pct);
        for (int i = 0; i < ret; i++)
            dprintf(STDOUT_FILENO, "\b");
    }

    if (xseek_offset() == xseek_size()) {
        dprintf(STDOUT_FILENO, "\n");
    }
#undef STATUS_STRING
}

int scan_next_segment() {
    struct czi_seg_header header;
    off_t next_segment;

    if (xread((void*) &header, sizeof(header)) == -1)
        return -1;

    next_segment = xseek_offset() + header.allocated_size;

    switch (czi_getsegid(&header)) {
    case ZISRAWDIRECTORY:
        czi_scan_directory();
        return -1;
        break;
    default:
        break;
    }

    xseek_set(next_segment);

    return 0;
}

void print_resolutions() {
    uint32_t *list;
    size_t len;

    czi_get_reslist(&list, &len);

    dprintf(STDOUT_FILENO, "available subsampling levels (bigger number means smaller images):\n");
    
    for (int i = 0; i < len; i++)
        dprintf(STDOUT_FILENO, "%u\n", list[i]);
}

/* process the next file segment */
int extract_next_segment() {
    struct czi_seg_header header;
    off_t next_segment;

    status_update();
    
    if (xread((void*) &header, sizeof(header)) == -1)
        return -1;

    next_segment = xseek_offset() + header.allocated_size;
    
    calljson(start_sh, &header);
    
    switch (czi_getsegid(&header)) {
    case ZISRAWFILE:
        czi_process_zrf();
        break;
    case ZISRAWDIRECTORY:
        czi_process_directory();
        break;
    case ZISRAWSUBBLOCK:
        czi_process_subblock();
        break;
    case ZISRAWMETADATA:
        czi_process_metadata();
        break;
    case ZISRAWATTACH:
        czi_process_attachment();
        break;
    case ZISRAWATTDIR:
        czi_process_attach_dir();
        break;
    case DELETED:
        calljson0(write_deleted);
        break;
    default:
    case UNKNOWN:
        ferrx(1, "failed to parse segment header: %s", header.name);
        break;
    }

    calljson0(finish_sh);
    xseek_set(next_segment);

    return 0;
}

int main(int argc, char *argv[]) {
    int ch;
    int filterlevel = 0;
    char *czifn = NULL;
    char *dirfn = NULL;
    char *jsonfn = NULL;
    const char *errstr;

    dojson = 0;
    doextract = 0;
    doscan = 0;
    
    /* argument handling */
    
    while ((ch = getopt(argc, argv, "+d:f:hi:j:s")) != -1) {
        switch (ch) {
        case 'd':
            doextract = 1;
            dirfn = optarg;
            break;
        case 'f':
            filterlevel = strtonum(optarg, 1, INT_MAX, &errstr);
            if (errstr)
                ferrx(1, "invalid filter level: %s", errstr);
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
        case 's':
            doscan = 1;
            break;
        default:
            errx(1, "error parsing arguments (pass '-h' for help)");
        }
    }

    if (argc > optind)
        errx(1, "too many arguments (pass '-h' for help)");

    if (!czifn)
        errx(1, "missing arguments (pass '-h' for help)");
    
    if (!dojson && !doextract && !doscan)
        errx(1, "missing arguments (pass '-h' for help");

    if (!doextract && filterlevel)
        errx(1, "cannot specify filter level when not performing extraction");
    
    if (doscan && (doextract || dojson))
        errx(1, "conflicting arguments (cannot perform resolution scan and tile extraction simultaneously)");
    
    if ((czifd = open(czifn, O_RDONLY)) < 0)
        err(1, "could not open input CZI file %s", czifn);

    if (mapsetup(czifd) < 0)
        errx(1, "could not initialise memory mapping information");

    if (doextract || dojson) {
        if (doextract) {
            if ((dirfd = open(dirfn, O_RDONLY | O_DIRECTORY)) < 0)
                err(1, "could not open output directory %s", dirfn);

            czi_extract_setfd(dirfd);
            czi_extract_setfilter(filterlevel);
        }

        if (dojson) {
            if ((jsonfd = open(jsonfn, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0)
                err(1, "could not open output JSON file %s", jsonfn);

            czi_json_setfd(jsonfd);
            czi_json_start();
        }
    
        while (extract_next_segment() == 0);
    
        if (dojson) {
            czi_json_finish();
        }
    } else if (doscan) {
        while (scan_next_segment() == 0);
        print_resolutions();
    }

    
    exit(0);
}





