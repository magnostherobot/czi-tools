
#include <stdint.h>
#include <string.h>

#include "zeiss.h"
#include "mmap.h"
#include "extractjxr.h"

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


