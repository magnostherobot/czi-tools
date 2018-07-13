
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

/*#define CALLUM_COMPAT*/

static int extractfd = -1;

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

/* check if a subblock has interesting metadata */
int czi_check_sblk_metadata(struct czi_subblock *sblk) {
    int rv;
    char buf[12];

    if (xread(&buf, 12))
        ferrx1(1, "could not read tile metadata");

    rv = strncmp(buf, "<METADATA />", 12);
    xseek_backward(12);

    return (rv != 0);
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
    
    if (xread((void*)&directory, sizeof(struct czi_directory)) == -1)
        ferrx1(1, "could not read ZISRAWDIRECTORY segment");

    xseek_backward(8);
    
    /* reallocarray() is a BSD extension, but we use it here for the overflow checking */
    if ((ptr = reallocarray(NULL,
                            sizeof(struct czi_subblock_direntry),
                            directory.entry_count)) == NULL) {
        ferr(1, "could not allocate memory for %" PRIu32 " directory entries",
             directory.entry_count);
    }

    directory.dir_entries = (struct czi_subblock_direntry *) ptr;
    
    for (uint32_t i = 0; i < directory.entry_count; i++) {
        struct czi_subblock_direntry *entry = &directory.dir_entries[i];
        
        if (xread((void*)entry,
                  sizeof(struct czi_subblock_direntry)) == -1)
            ferrx(1, "could not read subblock directory entry %" PRIu32, i);

        xseek_backward(8);

        if ((ptr = reallocarray(NULL,
                                sizeof(struct czi_subblock_dimentry),
                                entry->dimension_count)) == NULL)
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
     * subblock, which includes size information and a directory entry, and then
     * adjust for variable length dir_entry. after correcting the file offset we
     * can then read the {meta,}data and attachments.
     */

    if (xread(&sblk, /*sizeof(struct czi_subblock)*/ sizeof(struct czi_subblock_direntry) + 8) == -1)
        ferrx1(1, "could not read ZISRAWSUBBLOCK segment");

    if ((ptr = reallocarray(NULL,
                            sizeof(struct czi_subblock_dimentry),
                            sblk.dir_entry.dimension_count)) == NULL)
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
                         "%s-%" PRId32 "+%" PRIu32,
                         dimname,
                         sblk.dir_entry.dim_entries[i].start,
                         sblk.dir_entry.dim_entries[i].size) == -1)
                ferr1(1, "could not allocate memory for creating output filenames "
                      "from subblock dimension vector");

            fnames.suffix = tmp;
            tmp = NULL;
        } else {
            if (asprintf(&tmp,
                         "%s,%s-%" PRId32 "+%" PRIu32,
                         fnames.suffix,
                         dimname,
                         sblk.dir_entry.dim_entries[i].start,
                         sblk.dir_entry.dim_entries[i].size) == -1)
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
        if (czi_check_sblk_metadata(&sblk))
            extract_data(fnames.metadata, sblk.metadata_size);
        else
            xseek_forward(sblk.metadata_size);

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



