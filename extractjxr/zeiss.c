
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zeiss.h"
#include "mmap.h"
#include "json.h"
#include "macros.h"

static int extractfd = -1;
static uint32_t *reslist = NULL;
static size_t reslistsize = 0;
static size_t reslistlen = 0;

#define RESLIST_START_SIZE 10

/* resolution list handling */
static void reslist_init() {
    if (reslist)
        return;

    if ((reslist = reallocarray(NULL, RESLIST_START_SIZE, sizeof(int))) == NULL)
        ferr1(1, "could not initialise resolution list");

    reslistsize = RESLIST_START_SIZE;
}

static void reslist_expand() {
    if (!reslist)
        return;

    if ((reslist = reallocarray(reslist, 2 * reslistsize, sizeof(int))) == NULL)
        ferr1(1, "could not expand resolution list");

    reslistsize *= 2;
}

static void reslist_add(uint32_t toadd) {
    if (!reslist)
        reslist_init();

    for (int i = 0; i < reslistlen; i++)
        if (reslist[i] == toadd)
            return;

    if (reslistsize == reslistlen)
        reslist_expand();

    reslist[reslistlen++] = toadd;
}
    

void czi_extract_setfd(int val) {
    extractfd = val;
}

int czi_extract_getfd() {
    return extractfd;
}

/* mmap()ed file output handler
 *
 * we assume here that the file pointer is in the correct place
 */
static void extract_data(char *fname, uint64_t size) {
    int fd;

    if ((fd = openat(extractfd,
                     fname,
                     O_CREAT | O_TRUNC | O_RDWR,
                     0644)) < 0)
        ferr(1, "could not open output file \"%s\"", fname);

    if (ftruncate(fd, size) < 0)
        ferr(1, "could not resize output file \"%s\" to %" PRIu64
             " bytes in size", fname, size);

    if (xwrite(fd, size) == -1)
        ferrx(1, "could not write data to output file \"%s\"", fname);
    
    if (close(fd) == -1)
        warn("\nextract_data: could not close file descriptor %u to output "
             "file \"%s\" (continuing anyway...)",
             fd, fname);
}

enum czi_seg_t czi_getsegid(struct czi_seg_header *header) {
    if (strcmp(header->name, "ZISRAWSUBBLOCK") == 0) {
        return ZISRAWSUBBLOCK;
    }
    else if (strcmp(header->name, "ZISRAWATTACH") == 0) {
        return ZISRAWATTACH;
    }
    else if (strcmp(header->name, "ZISRAWATTDIR") == 0) {
        return ZISRAWATTDIR;
    }
    else if (strcmp(header->name, "ZISRAWDIRECTORY") == 0) {
        return ZISRAWDIRECTORY;
    }
    else if (strcmp(header->name, "DELETED") == 0) {
        return DELETED;
    }
    else if (strcmp(header->name, "ZISRAWMETADATA") == 0) {
        return ZISRAWMETADATA;
    }
    else if (strcmp(header->name, "ZISRAWFILE") == 0) {
        return ZISRAWFILE;
    }
    else {
        return UNKNOWN;
    }
}

void czi_process_zrf() {
    struct czi_zrf data;
    
    if (xread((void*)&data, sizeof(struct czi_zrf)) == -1)
        ferrx1(1, "could not read ZISRAWFILE segment");
    
    calljson(write_zrf, &data);
}

void czi_process_directory() {
    struct czi_directory directory;
    void *ptr;

    /* the last member of a struct czi_directory is a pointer, which doesn't
     * come from the data file, so we should avoid reading into that value */
    
    if (xread((void*)&directory, (sizeof(struct czi_directory) - sizeof(void *))) == -1)
        ferrx1(1, "could not read ZISRAWDIRECTORY segment");
    
    /* reallocarray() is a BSD extension, but we use it here for the overflow checking */
    if ((ptr = reallocarray(NULL,
                            directory.entry_count,
                            sizeof(struct czi_subblock_direntry))) == NULL) {
        ferr(1, "could not allocate memory for %" PRIu32 " directory entries",
             directory.entry_count);
    }

    directory.dir_entries = (struct czi_subblock_direntry *) ptr;
    
    for (uint32_t i = 0; i < directory.entry_count; i++) {
        struct czi_subblock_direntry *entry = &directory.dir_entries[i];

        /* accomodate the pointer at the end of the direntry struct */
        if (xread((void*)entry,
                  (sizeof(struct czi_subblock_direntry) - sizeof(void *))) == -1)
            ferrx(1, "could not read subblock directory entry %" PRIu32, i);

        if ((ptr = reallocarray(NULL,
                                entry->dimension_count,
                                sizeof(struct czi_subblock_dimentry))) == NULL)
            ferrx(1, "could not allocate memory for %" PRIu32 " dimension entries",
                 entry->dimension_count);

        entry->dim_entries = (struct czi_subblock_dimentry *) ptr;
        
        if (xread((void*) entry->dim_entries,
                  entry->dimension_count * sizeof(struct czi_subblock_dimentry)) == -1)
            ferrx(1, "could not read %" PRIu32 " dimension entries",
                 entry->dimension_count);
    }

    calljson(write_directory, &directory);

    for (uint32_t i = 0; i < directory.entry_count; i++) {
        free(directory.dir_entries[i].dim_entries);
    }
    free(directory.dir_entries);
}

void czi_process_subblock() {
    struct czi_subblock sblk;
    void *ptr;
    off_t offset = xseek_offset();
    struct {
        char *suffix;
        char *metadata;
        char *data;
        char *attach;
    } fnames = { NULL, NULL, NULL, NULL };
    char *tmp = NULL;

    /*
     * processing a subblock takes some care. we read in the beginning of the
     * subblock, which includes size information and a directory entry (though
     * the direntry struct has an extra pointer at the end, see above), and then
     * adjust for variable length dir_entry. after correcting the file offset we
     * can then read the {meta,}data and attachments.
     */

    if (xread(&sblk, (sizeof(struct czi_subblock) - sizeof(void *))) == -1)
        ferrx1(1, "could not read ZISRAWSUBBLOCK segment");

    if ((ptr = reallocarray(NULL,
                            sblk.dir_entry.dimension_count,
                            sizeof(struct czi_subblock_dimentry))) == NULL)
        ferr(1, "could not allocate memory for %" PRIu32 " dimension entries",
             sblk.dir_entry.dimension_count);

    sblk.dir_entry.dim_entries = (struct czi_subblock_dimentry *) ptr;

    if (xread(sblk.dir_entry.dim_entries,
              sizeof(struct czi_subblock_dimentry) * sblk.dir_entry.dimension_count) == -1)
        ferrx1(1, "could not read subblock dimension directory");
    
    /* twiddle file offset */
    offset = xseek_offset() - offset;
    offset = 256 - offset;
    if (offset > 0)
        xseek_forward(offset);

    /* generate filename suffix -- XXX move to a file static and allocate on demand? */
    for (uint32_t i = 0; i < sblk.dir_entry.dimension_count; i++) {
        char dimname[5];

        memcpy(&dimname, sblk.dir_entry.dim_entries[i].dimension, 4);
        dimname[4] = '\0';
        
        if (i == 0) {
            if (asprintf(&tmp,
                         "%sp%" PRId32 "s%" PRIu32 "r%" PRIu32,
                         dimname,
                         sblk.dir_entry.dim_entries[i].start,
                         sblk.dir_entry.dim_entries[i].size,
                         (sblk.dir_entry.dim_entries[i].stored_size == 0 ? 1 :
                          (sblk.dir_entry.dim_entries[i].size /
                           sblk.dir_entry.dim_entries[i].stored_size)))
                         == -1)
                ferr1(1, "could not allocate memory for creating output filenames "
                      "from subblock dimension vector");

            fnames.suffix = tmp;
            tmp = NULL;
        } else {
            if (asprintf(&tmp,
                         "%s,%sp%" PRId32 "s%" PRIu32 "r%" PRIu32,
                         fnames.suffix,
                         dimname,
                         sblk.dir_entry.dim_entries[i].start,
                         sblk.dir_entry.dim_entries[i].size,
                         (sblk.dir_entry.dim_entries[i].stored_size == 0 ? 1 :
                          (sblk.dir_entry.dim_entries[i].size /
                           sblk.dir_entry.dim_entries[i].stored_size))) == -1)
                ferr1(1, "could not allocate memory for creating output filenames "
                      "from subblock dimension vector");

            free(fnames.suffix);
            fnames.suffix = tmp;
            tmp = NULL;
        }
    }

    /* generate filenames */
    if (asprintf(&fnames.metadata,
                 "metadata-%s.xml",
                 fnames.suffix) == -1)
        ferr(1, "could not allocate memory for creating output metadata filename "
             "in tile suffix %s", fnames.suffix);

    if (asprintf(&fnames.data,
                 "data-%s.jxr",
                 fnames.suffix) == -1)
        ferr(1, "could not allocate memory for creating output data filename "
             "in tile suffix %s", fnames.suffix);

    if (asprintf(&fnames.attach,
                 "attach-%s",
                 fnames.suffix) == -1)
        ferr(1, "could not allocate memory for creating output attachment filename "
             "in tile suffix %s", fnames.suffix);

    
    /* write the JSON... */
    calljson(write_subblock, &sblk, fnames.metadata, fnames.data, fnames.attach);

    /* then jump into the scary heavy lifting */
    if (extractfd != -1) {
        if (sblk.metadata_size != 0)
            extract_data(fnames.metadata, sblk.metadata_size);

        if (sblk.data_size != 0)
            extract_data(fnames.data, sblk.data_size);

        if (sblk.attachment_size != 0)
            extract_data(fnames.attach, sblk.attachment_size);
    }
        
    free(sblk.dir_entry.dim_entries);
    free(fnames.suffix);
    free(fnames.metadata);
    free(fnames.data);
    free(fnames.attach);
    return;
}

void czi_process_metadata() {
    struct czi_metadata data;

    if (xread((void *)&data, sizeof(struct czi_metadata)) == -1)
        ferrx1(1, "could not read ZISRAWMETADATA segment");

    calljson(write_metadata, &data);

    if (extractfd != -1)
        extract_data("FILE-META-1.xml", data.xml_size);

    return;
}

void czi_process_attachment() {
    static unsigned int attmt_no = 1;
    struct czi_attach att;
    char *name;

    if (xread((void *)&att, sizeof(struct czi_attach)) == -1)
        ferrx1(1, "could not read ZISRAWATTACH segment");

    /* generate filenames */
    if (asprintf(&name, "attach-data-%u", attmt_no) == -1)
        ferr1(1, "could not allocate memory for attachment filename");
    
    calljson(write_attachment, &att, name);

    if (extractfd != -1)
        extract_data(name, att.data_size);
    
    free(name);
    attmt_no++;
    return;
}

void czi_process_attach_dir() {
    struct czi_attach_dir attd;
    void *ptr;

    /* strip trailing pointer */
    if (xread((void *)&attd,
              (sizeof(struct czi_attach_dir) - sizeof(void *))) == -1)
        ferrx1(1, "could not read ZISRAWATTDIR segment");

    if ((ptr = reallocarray(NULL,
                            attd.entry_count,
                            sizeof(struct czi_attach_entry))) == NULL)
        ferr(1, "could not allocate memory for %" PRIu32
             " attachment directory entries", attd.entry_count);

    attd.att_entries = (struct czi_attach_entry *) ptr;
    
    if (xread((void *)attd.att_entries,
              sizeof(struct czi_attach_entry) * attd.entry_count) == -1)
        ferrx(1, "could not read %" PRIu32 "attachment directory entries",
              attd.entry_count);

    calljson(write_attach_dir, &attd);

    free(attd.att_entries);
    
    return;
}

/* scan the directory's dimension entries for resolution ratios */
void czi_scan_directory() {
    struct czi_directory directory;
    struct czi_subblock_direntry direntry;
    struct czi_subblock_dimentry dimentry;
    void *ptr;

    /* ZISRAWDIRECTORY reading logic as above */
    
    if (xread((void*)&directory, (sizeof(struct czi_directory) - sizeof(void *))) == -1)
        ferrx1(1, "could not read ZISRAWDIRECTORY segment");
    
    for (uint32_t i = 0; i < directory.entry_count; i++) {
        /* accomodate the pointer at the end of the direntry struct */
        if (xread((void*)&direntry,
                  (sizeof(struct czi_subblock_direntry) - sizeof(void *))) == -1)
            ferrx(1, "could not read subblock directory entry %" PRIu32, i);

        for (uint32_t j = 0; j < direntry.dimension_count; j++) {
            if (xread((void*)&dimentry, sizeof(struct czi_subblock_dimentry)) == -1)
                ferrx(1, "could not read dimension entry %" PRIu32 " of directory entry %" PRIu32,
                      j, i);

            czi_scan_dimentry(&dimentry);
        }
    }
}

void czi_scan_dimentry(struct czi_subblock_dimentry *dim) {
    /* Optimisation: only check X and Y dimensions */
    char x[4] = "X\0\0\0";
    char y[4] = "Y\0\0\0";
    uint32_t ratio;

    if ((memcmp(dim->dimension, x, 4) != 0) && (memcmp(dim->dimension, y, 4) != 0))
        return;

    if (dim->stored_size == 0)
        reslist_add(1);
    else
        reslist_add(dim->size / dim->stored_size);

    return;
}

void czi_get_reslist(uint32_t **listout, size_t *listlen) {
    if (!reslist)
        ferrx1(1, "attempted to access uninitialised resolution list");

    *listout = reslist;
    *listlen = reslistlen;
}
