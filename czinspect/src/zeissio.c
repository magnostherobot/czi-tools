
#include <sys/types.h>
#include <err.h>
#include <stdint.h>

#include "config.h"

#include "zeiss.h"
#include "endian.h"
#include "macros.h"
#include "mapfile.h"
#include "types.h"

#define F(n, t)   int czi_read_ ## n (struct map_ctx *ctx, struct t *data)

#define READM() do {                                            \
        if (map_read(ctx, (void*) data, sizeof(*data)) == -1)   \
            return -1;                                          \
    } while (0)

#define READM2(m) do { \
        if (map_read(ctx, (void*) &data->m, sizeof(data->m)) == -1) \
            return -1;                                                  \
    } while (0)

#define SWE(m)    SWITCH_ENDS(&data->m)

/* read in a segment header */
F(sh, czi_seg_header) {
    READM();

    SWE(allocated_size);
    SWE(used_size);

    return 0;
}

F(zrf, czi_zrf) {
    READM();

    SWE(major);
    SWE(minor);
    SWE(reserved1);
    SWE(reserved2);

    SWE(file_part);
    SWE(directory_position);
    SWE(metadata_position);
    SWE(update_pending);
    SWE(attachment_directory_position);
    
    return 0;
}

F(metadata, czi_metadata) {
    READM();

    SWE(xml_size);
    SWE(attachment_size);
    
    return 0;
}

F(sblk_dimentry, czi_subblock_dimentry) {
    READM();

    SWE(start);
    SWE(size);
    SWE(start_coordinate);
    SWE(stored_size);

    return 0;
}

F(sblk_direntry, czi_subblock_direntry) {
    READM();

    SWE(pixel_type);
    SWE(file_position);
    SWE(file_part);
    SWE(compression);
    SWE(dimension_count);
    
    return 0;
}

/* nested structures, so don't read everything in at once */
F(subblock, czi_subblock) {
    READM2(metadata_size);
    READM2(attachment_size);
    READM2(data_size);

    SWE(metadata_size);
    SWE(attachment_size);
    SWE(data_size);

    return czi_read_sblk_direntry(ctx, &data->dir_entry);
}

F(directory, czi_directory) {
    READM();

    SWE(entry_count);

    return 0;
}

F(atmt_entry, czi_attach_entry) {
    READM();

    SWE(file_position);
    SWE(file_part);

    return 0;
}

F(attach, czi_attach) {
    READM2(data_size);
    READM2(reserved1);

    if (czi_read_atmt_entry(ctx, &data->att_entry) == -1)
        return -1;

    READM2(reserved2);

    SWE(data_size);
    SWE(reserved1);
    SWE(reserved2);

    return 0;
}

F(atmt_dir, czi_attach_dir) {
    READM();

    SWE(entry_count);

    return 0;
}



