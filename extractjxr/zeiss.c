
#define _GNU_SOURCE
#include <err.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

void czi_process_zrf(struct czi_seg_header *header) {
    struct czi_zrf data;
    
    if (xread((void*)&data, sizeof(struct czi_zrf)) == -1)
        ferrx1(1, "could not read ZISRAWFILE segment");
    
    calljson(write_zrf, &data);
}

void czi_process_directory(struct czi_seg_header *header) {
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
