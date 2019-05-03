
#include "config.h"

#include <err.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "types.h"
#include "alloc.h"
#include "macros.h"
#include "mapfile.h"
#include "util.h"
#include "zeiss.h"
#include "zeissio.h"

static void parse_opts(struct config *cfg) {
    /* currently a nop */
    (void)cfg;
    return;
}

void do_scan(struct config *cfg) {
    struct czi_seg_header head;
    struct czi_zrf zisrf;
    struct map_ctx *c;
    lzbuf *reslist;

    parse_opts(cfg);

    c = cfg->inctx;

        /* read initial segment */
    if (czi_read_sh(c, &head) == -1)
        ferrx1("could not read initial segment header from input file");

    if (czi_getsegid(&head) != ZISRAWFILE)
        ferrx1("input file does not start with a ZISRAWFILE segment");

    if (czi_read_zrf(c, &zisrf) == -1)
        ferrx1("could not read ZISRAWFILE segment");

    /* XXX this should be changed when segment-local scanning is implemented */
    if (zisrf.directory_position == 0)
        ferrx1("file contains no subblock directory, cannot scan for subsampling levels");

    if (map_seek(c, zisrf.directory_position, MAP_SET) == -1)
        ferrx("could not seek to offset %" PRIu64 " to read subblock directory", zisrf.directory_position);

    reslist = lzbuf_new(uint32_t);

    if (make_reslist(c, reslist) == -1)
        ferrx1("could not scan for subsampling levels in input file");

    printf("Available subsampling levels (smaller number indicates higher resolution):\n\n");

    for (uint32_t i = 0; i < lzbuf_elems(uint32_t, reslist); i++)
        printf("\t%" PRIu32 "\n", lzbuf_get(uint32_t, reslist, i));

    printf("\n");
    lzbuf_free(reslist);
    
    return;
}
