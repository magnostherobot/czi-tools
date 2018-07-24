
#include <string.h>

#include "zeiss.h"

/* get the segment type from a segment header */
enum czi_seg_t czi_getsegid(struct czi_seg_header *header) {
    char name[17];

    memcpy(name, header->name, 16);
    name[16] = '\0';
    
    if (strcmp(name, "ZISRAWSUBBLOCK") == 0) {
        return ZISRAWSUBBLOCK;
    }
    else if (strcmp(name, "ZISRAWATTACH") == 0) {
        return ZISRAWATTACH;
    }
    else if (strcmp(name, "ZISRAWATTDIR") == 0) {
        return ZISRAWATTDIR;
    }
    else if (strcmp(name, "ZISRAWDIRECTORY") == 0) {
        return ZISRAWDIRECTORY;
    }
    else if (strcmp(name, "DELETED") == 0) {
        return DELETED;
    }
    else if (strcmp(name, "ZISRAWMETADATA") == 0) {
        return ZISRAWMETADATA;
    }
    else if (strcmp(name, "ZISRAWFILE") == 0) {
        return ZISRAWFILE;
    }
    else {
        return UNKNOWN;
    }
}

