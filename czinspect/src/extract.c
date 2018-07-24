
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include "types.h"
#include "alloc.h"
#include "operations.h"
#include "mapfile.h"
#include "macros.h"
#include "util.h"
#include "zeiss.h"
#include "zeissio.h"

static int dirfd;

static void extract_subblock(struct map_ctx *c) {
    struct czi_subblock sblk;

    if (czi_read_subblock(c, &sblk) == -1)
        ferrx1("could not read subblock segment metadata");
}

static void extract_metadata(struct map_ctx *c) {
    return;
}

static void extract_attachment(struct map_ctx *c) {
    return;
}

static void check_config(struct config *cfg) {
    if (!cfg->outdir)
        errx(1, "no output directory specified (pass '-h' for help)");

    return;
}


void do_extract(struct config *cfg) {
    struct map_ctx *c;
    struct czi_seg_header head;
    enum czi_seg_t segtype;
    size_t next_segment;

    check_config(cfg);
    
    if ((dirfd = open(cfg->outdir, O_RDONLY | O_DIRECTORY)) < 0)
        err(1, "could not open output directory \"%s\"", cfg->outdir);

    c = cfg->inctx;
    
    while (map_file_offset(c) < c->fsize) {
        if (czi_read_sh(c, &head) == -1)
            ferrx1("could not read next segment header");

        next_segment = map_file_offset(c) + head.allocated_size;

        segtype = czi_getsegid(&head);

        if (segtype == UNKNOWN)
            fwarnx("unknown segment header type \"%.16s\" found in input file (continuing...)", head.name);
        else if (segtype == ZISRAWSUBBLOCK)
            extract_subblock(c);
        else if (segtype == ZISRAWMETADATA)
            extract_metadata(c);
        else if (segtype == ZISRAWATTACH)
            extract_attachment(c);

        map_seek(c, next_segment, MAP_SET);
        
        if (cfg->progress_bar)
            update_progress_bar(c);
    }
}
