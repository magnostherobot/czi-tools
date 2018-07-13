
#include <err.h>
#include <stdint.h>
#include <string.h>

#include "zeiss.h"
#include "mmap.h"
#include "json.h"
#include "macros.h"

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

    calljson(start_sh, header);

    if (xread((void*)&data, sizeof(data)) == -1)
        ferrx1(1, "could not read ZISRAWFILE segment");
    
    calljson(write_zrf, &data);
    calljson0(finish_sh);

    xseek(header->allocated_size);
}


