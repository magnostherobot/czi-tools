
#include "config.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "alloc.h"
#include "operations.h"
#include "mapfile.h"
#include "macros.h"
#include "util.h"
#include "zeiss.h"
#include "zeissio.h"

#define EXT_S_META   0x01
#define EXT_S_DATA   0x02
#define EXT_S_ATTACH 0x04
#define EXT_S_ALL    (EXT_S_META | EXT_S_DATA | EXT_S_ATTACH)

enum subopt_index {
    SUBOPT_ALL = 0,
    SUBOPT_META = 1,
    SUBOPT_DATA = 2,
    SUBOPT_ATTACH = 3,
    SUBOPT_MAX = 4
};

static int dirfd;
static uint8_t sblk_opts;
static uint8_t ext_opts;
static uint8_t filt_opts;
static uint32_t filterlevel;
static lzstring fname;
static lzstring suffix;
static lzbuf dimensions;

static void set_subopt(uint8_t opt) {
    if ((sblk_opts & EXT_S_ALL) == EXT_S_ALL)
        errx(1, "cannot use 'all' with other options");
    else if ((sblk_opts & opt) == opt)
        errx(1, "duplicate options");

    sblk_opts |= opt;
    return;
}

static void parse_opts(struct config *cfg) {
    char *optptr, *valptr;
    char *subopts[] = {
        [SUBOPT_ALL] = "all",
        [SUBOPT_META] = "meta",
        [SUBOPT_DATA] = "data",
        [SUBOPT_ATTACH] = "attach",
        [SUBOPT_MAX] = NULL
    };

    if (!cfg->outdir)
        errx(1, "no output directory specified (pass '-h' for help)");

    if (cfg->eflags & EXT_F_SBLK) {
        optptr = cfg->esopts;
        while (*optptr) {
            switch (getsubopt(&optptr, subopts, &valptr)) {
            case SUBOPT_ALL:
                if ((sblk_opts & EXT_S_ALL) == EXT_S_ALL)
                    errx(1, "duplicate options");
                else if (sblk_opts != 0)
                    errx(1, "cannot use 'all' with other options");
                else
                    sblk_opts |= EXT_S_ALL;
                break;
            case SUBOPT_META:
                set_subopt(EXT_S_META);
                break;
            case SUBOPT_DATA:
                set_subopt(EXT_S_DATA);
                break;
            case SUBOPT_ATTACH:
                set_subopt(EXT_S_ATTACH);
                break;
            case -1:
                errx(1, "unknown argument to '-s': %s", valptr);
                break;
            }
        }

        xfree(cfg->esopts);
    }

    if (cfg->eflags == 0) {
        ext_opts = (EXT_F_META | EXT_F_SBLK | EXT_F_ATTACH);
        sblk_opts = EXT_S_ALL;
    } else {
        ext_opts = cfg->eflags;
    }

    if (((ext_opts & EXT_F_SBLK) == 0) && (cfg->filtflags & (EXT_FI_FILT | EXT_FI_FFUZZ))) {
        errx(1, "cannot perform subsampling level filtering when not extracting subblocks");
    }

    if ((cfg->filtflags & EXT_FI_FFUZZ) == EXT_FI_FFUZZ && (cfg->filtflags & EXT_FI_FILT) == 0)
        errx(1, "cannot round subsampling level when not performing filtering");

    filt_opts = cfg->filtflags;
    
    return;
}

static int write_file(char *path, struct map_ctx *c, size_t len) {
    struct map_ctx *out;
    int fd;

    if ((fd = openat(dirfd, path, O_RDWR | O_CREAT, 0644)) < 0)
        return fwarn("could not open \"%s\" for writing", path), -1;

    if (xfallocate(fd, len) == -1)
        return fwarnx("could not allocate space for file \"%s\"", path), -1;

    if ((out = map_mopen(path, fd, len, PROT_READ | PROT_WRITE)) == NULL)
        return -1; /* message printed in map_mopen() */

    if (map_splice(c, out, len) == -1)
        return -1;

    map_mclose(out);

    if (close(fd) < 0)
        fwarn("could not close file descriptor to file \"%s\"", path);
    
    return 0;
}

static void extract_metadata(struct map_ctx *c, uint64_t pos) {
    struct czi_seg_header head;
    struct czi_metadata data;
    
    if (map_seek(c, pos, MAP_SET) == -1)
        ferrx("could not seek to file offset %" PRIu64 " to read metadata segment", pos);

    if (czi_read_sh(c, &head) == -1)
        ferrx1("could not read metadata segment header");

    if (czi_getsegid(&head) != ZISRAWMETADATA)
        ferrx1("incorrect metadata segment id");

    if (czi_read_metadata(c, &data) == -1)
        ferrx1("could not read metadata segment");

    if (write_file("FILE-META.xml", c, data.xml_size) == -1)
        ferrx1("could not write metadata file");

    return;
}

static void extract_attachments(struct map_ctx *c, uint64_t pos) {
    struct czi_seg_header head;
    struct czi_attach_dir adir;
    struct czi_attach_entry entry;
    struct czi_attach att;
    size_t offset;
    lzstring aname;

    if (map_seek(c, pos, MAP_SET) == -1)
        ferrx("could not seek to file offset %" PRIu64 " to read attachment directory segment", pos);

    if (czi_read_sh(c, &head) == -1)
        ferrx1("could not read attachment directory segment header");

    if (czi_getsegid(&head) != ZISRAWATTDIR)
        ferrx1("incorrect attachment directory segment id");

    if (czi_read_atmt_dir(c, &adir) == -1)
        ferrx1("could not read attachment directory segment");

    if (adir.entry_count == 0) {
        fwarnx1("attachment directory contains no attachments, continuing...");
        return;
    }

    aname = lzstr_new();
    
    for (uint32_t i = 0; i < adir.entry_count; i++) {
        if (czi_read_atmt_entry(c, &entry) == -1)
            ferrx1("could not read attachment entry");

        offset = map_file_offset(c);

        if (map_seek(c, entry.file_position, MAP_SET) == -1)
            ferrx("could not seek to file offset %" PRIu64 "to read attachment segment", entry.file_position);

        if (czi_read_sh(c, &head) == -1)
            ferrx("could not read segment header for file attachment %" PRIu32, i + 1);

        if (czi_getsegid(&head) != ZISRAWATTACH)
            ferrx("attachment segment %" PRIu32 " does not point to a ZISRAWATTACH segment", i + 1);
        
        if (czi_read_attach(c, &att) == -1)
            ferrx1("could not read attachment segment");

        lzsprintf(aname, "attach-data-%" PRIu32, i + 1); /* if this fails, it will die in lzstr_grow() */

        if (write_file(aname->data, c, att.data_size) == -1)
            ferrx("could not write file attachment #%" PRIu32, i + 1);

        if (map_seek(c, offset, MAP_SET) == -1)
            ferrx1("could not reset file offset for next attachment segment");
    }

    lzstr_free(aname);

    return;
}

static void make_suffix(lzstring str, lzbuf diments, uint32_t count) {
    char dimname[5];
    lzstring tmp;
    struct czi_subblock_dimentry *dims;

    dims = &lzbuf_get(struct czi_subblock_dimentry, diments, 0);
    
    /* null termination, for sanity */
    memcpy(&dimname, dims->dimension, 4);
    dimname[4] = '\0';

    lzsprintf(str, "%sp%" PRId32 "s%" PRIu32 "r%" PRIu32, dimname, dims->start,
              dims->size, czi_subblock_dim_ratio(dims));

    if (count == 1)
        return;

    tmp = lzstr_new();
    
    for (uint32_t i = 1; i < count; i++) {
        dims = &lzbuf_get(struct czi_subblock_dimentry, diments, i);
        
        lzstr_zero(tmp);

        memcpy(&dimname, dims->dimension, 4);
        dimname[4] = '\0';

        lzsprintf(tmp, ",%sp%" PRId32 "s%" PRIu32 "r%" PRIu32, dimname, dims->start,
                  dims->size, czi_subblock_dim_ratio(dims));
        lzstr_cat(str, tmp->data);
    }

    lzstr_free(tmp);
    return;
}

static void extract_subblock(struct map_ctx *c) {
    struct czi_seg_header head;
    struct czi_subblock sblk;
    struct czi_subblock_dimentry *entry;
    size_t offset;
    uint32_t level;

    if (czi_read_sh(c, &head) == -1)
        ferrx1("could not read segment header");

    if (czi_getsegid(&head) != ZISRAWSUBBLOCK)
        ferrx1("incorrect subblock segment id");
    
    offset = map_file_offset(c);
    
    if (czi_read_subblock(c, &sblk) == -1)
        ferrx1("could not read subblock");

    if (sblk.dir_entry.dimension_count == 0) {
        fwarnx1("segment has no dimension entries, skipping segment");
        return;
    }
    
    while (lzbuf_elems(struct czi_subblock_dimentry, dimensions) < sblk.dir_entry.dimension_count)
        lzbuf_grow(struct czi_subblock_dimentry, dimensions);

    uint32_t i;
    for (i = 0; i < sblk.dir_entry.dimension_count; i++) {
        entry = &lzbuf_get(struct czi_subblock_dimentry, dimensions, i);
        if (czi_read_sblk_dimentry(c, entry) == -1)
            ferrx1("could not read subblock dimension entries");
    }

    if (filterlevel != 0) {
        level = get_subsample_level(dimensions, sblk.dir_entry.dimension_count);
        if (level != filterlevel) {
            lzbuf_zero(dimensions);
            return;
        }
    }
    
    /* fix file offset */
    offset = map_file_offset(c) - offset;
    offset = 256 - offset;
    if (offset > 0)
        if (map_seek(c, offset, MAP_FORW) == -1)
            ferrx("cannot seek forwards %zu bytes", offset);
    
    make_suffix(suffix, dimensions, sblk.dir_entry.dimension_count);

    if (sblk.metadata_size != 0) {
        if (sblk_opts & EXT_S_META) {
            lzsprintf(fname, "metadata-%s.xml", suffix->data);
            if (write_file(fname->data, c, sblk.metadata_size) == -1)
                ferrx("could not write subblock metadata to file \"%s\"", fname->data);
        } else {
            if (map_seek(c, sblk.metadata_size, MAP_FORW) == -1)
                ferrx1("could not seek past segment metadata");
        }
    }

    if (sblk.data_size != 0) {
        if (sblk_opts & EXT_S_DATA) {
            lzsprintf(fname, "data-%s.jxr", suffix->data);
            if (write_file(fname->data, c, sblk.data_size) == -1)
                ferrx("could not write subblock data to file \"%s\"", fname->data);
        } else {
            if (map_seek(c, sblk.data_size, MAP_FORW) == -1)
                ferrx1("could not seek past segment data");
        }
    }

    if (sblk.attachment_size != 0) {
        if (sblk_opts & EXT_S_ATTACH) {
            lzsprintf(fname, "attach-%s", suffix->data);
            if (write_file(fname->data, c, sblk.attachment_size) == -1)
                ferrx("could not write subblock attachment to file \"%s\"", fname->data);
        } else {
            if (map_seek(c, sblk.attachment_size, MAP_FORW) == -1)
                ferrx1("could not seek past segment metadata");
        }
    }

    lzbuf_zero(dimensions);
    
    return;
}

static void extract_sblk_directory(struct map_ctx *c, uint64_t pos) {
    struct czi_seg_header head;
    struct czi_directory cdir;
    struct czi_subblock_direntry sdir;
    size_t offset;

    fname = lzstr_new();
    suffix = lzstr_new();
    dimensions = lzbuf_new(struct czi_subblock_dimentry);
    
    if (map_seek(c, pos, MAP_SET) == -1)
        ferrx("could not seek to %" PRIu64 " to read directory segment", pos);

    if (czi_read_sh(c, &head) == -1)
        ferrx1("could not read segment header");

    if (czi_getsegid(&head) != ZISRAWDIRECTORY)
        ferrx1("incorrect directory segment id");

    if (czi_read_directory(c, &cdir) == -1)
        ferrx1("could not read directory segment");

    if (cdir.entry_count == 0) {
        /* in this case, we fall past the loop */
        fwarnx1("directory entry segment contains no entries");
    }

    for (uint32_t i = 0; i < cdir.entry_count; i ++) {
        if (czi_read_sblk_direntry(c, &sdir) == -1)
            ferrx("could not read directory entry #%" PRIu32, i + 1);

        if (sdir.dimension_count == 0) {
            fwarnx("directory entry #%" PRIu32 " has no dimension entries, continuing...", i + 1);
            continue;
        }

        if (map_seek(c, sdir.dimension_count * sizeof(struct czi_subblock_dimentry), MAP_FORW) == -1)
            ferrx("could not seek past %" PRIu32 "dimension entries in directory entry #%" PRIu32,
                  sdir.dimension_count, i + 1);

        offset = map_file_offset(c);

        if (map_seek(c, sdir.file_position, MAP_SET) == -1)
            ferrx("could not seek to subblock segment starting at offset %" PRIu64, sdir.file_position);

        extract_subblock(c);

        if (map_seek(c, offset, MAP_SET) == -1)
            ferrx("could not seek to previous offset %zu to resume scanning subblocks", offset);
    }

    lzstr_free(fname);
    lzstr_free(suffix);
    lzbuf_free(dimensions);
    
    return;
}

static void setup_filterlevel(struct config *cfg, struct map_ctx *c, uint64_t dirpos) {
    lzbuf reslist;
    uint32_t rnum = 0;
    uint32_t max = 0;
    uint32_t l;

    if ((filt_opts & EXT_FI_FILT) == 0)
        return;

    if (map_seek(c, dirpos, MAP_SET) == -1)
        ferrx("could not seek to offset %" PRIu64 " to read subblock directory for subsampling scan",
              dirpos);

    reslist = lzbuf_new(uint32_t);

    if (make_reslist(c, reslist, &rnum) == -1)
        ferrx1("could not scan for subsampling levels in input file");


    if (filt_opts & EXT_FI_FFUZZ) {
        for (uint32_t i = 0; i < rnum; i++) {
            l = lzbuf_get(uint32_t, reslist, i);

            if (l <= cfg->filter && l > max) {
                filterlevel = l;
                max = l;
            }
        }
    } else {
        for (uint32_t i = 0; i < rnum; i++)
            if (lzbuf_get(uint32_t, reslist, i) == cfg->filter)
                filterlevel = cfg->filter;
    }
        
    if (filterlevel == 0)
        errx(1, "invalid filter level: %" PRIu32, cfg->filter);

    return;
}

void do_extract(struct config *cfg) {
    struct czi_seg_header head;
    struct czi_zrf zisrf;
    struct map_ctx *c;

    parse_opts(cfg);
    
    if ((dirfd = open(cfg->outdir, O_RDONLY | O_DIRECTORY)) < 0)
        err(1, "could not open output directory \"%s\"", cfg->outdir);

    c = cfg->inctx;

    /* read initial segment */
    if (czi_read_sh(c, &head) == -1)
        ferrx1("could not read initial segment header from input file");

    if (czi_getsegid(&head) != ZISRAWFILE)
        ferrx1("input file does not start with a ZISRAWFILE segment");

    if (czi_read_zrf(c, &zisrf) == -1)
        ferrx1("could not read ZISRAWFILE segment");
    
    if (ext_opts & EXT_F_META) {
        if (zisrf.metadata_position == 0) {
            /* the CZI spec doesn't give any information on how to detect a
             * nonexistant metadata segment, so I'm guessing here*/
            fwarnx1("file metadata extraction requested but none found, continuing...");
        } else {
            extract_metadata(c, zisrf.metadata_position);
        }
    }

    if (ext_opts & EXT_F_ATTACH) {
        if (zisrf.attachment_directory_position == 0) {
            /* see above */
            fwarnx1("file attachment extraction requested but none found, continuing...");
        } else {
            extract_attachments(c, zisrf.attachment_directory_position);
        }
    }

    if (ext_opts & EXT_F_SBLK) {
        if (zisrf.directory_position == 0) {
            /* as above */
            fwarnx1("file subblock extraction requested but none found, continuing...");
        } else {
            setup_filterlevel(cfg, c, zisrf.directory_position);
            extract_sblk_directory(c, zisrf.directory_position);
        }
    }
    
    return;
}
