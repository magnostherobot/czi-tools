
#include <sys/types.h>
#include <err.h>
#include <stdint.h>

#include "config.h"

#include "zeiss.h"
#include "endian.h"
#include "mapfile.h"
#include "types.h"

#define F(n, t)   int czi_read_ ## n (struct map_ctx *ctx, struct t *data)

#define READM() do {                                            \
        if (map_read(ctx, (void*) data, sizeof(*data)) == -1)   \
            return -1;                                          \
    } while (0)

#define READM2(m) do { \
        if (map_read(ctx, (void*) &data->m, sizeof(data->m)) == -1) \
            return -1;                                              \
    } while (0)

#ifdef BIG_ENDIAN
# define SWE(t, m) do {                           \
        t tmp = data->m;                          \
        SWITCH_ENDS(&tmp);                        \
        data->m = tmp;                            \
    } while (0)
#else
# define SWE(t, n)
#endif

/* read in a segment header */
F(sh, czi_seg_header) {
    READM();

    SWE(uint64_t, allocated_size);
    SWE(uint64_t, used_size);

    return 0;
}

F(zrf, czi_zrf) {
    READM();

    SWE(uint32_t, major);
    SWE(uint32_t, minor);
    SWE(uint32_t, reserved1);
    SWE(uint32_t, reserved2);

    SWE(uint32_t, file_part);
    SWE(uint64_t, directory_position);
    SWE(uint32_t, metadata_position);
    SWE(uint32_t, update_pending);
    SWE(uint64_t, attachment_directory_position);
    
    return 0;
}

F(metadata, czi_metadata) {
    READM();

    SWE(uint32_t, xml_size);
    SWE(uint32_t, attachment_size);
    
    return 0;
}

F(sblk_dimentry, czi_subblock_dimentry) {
    READM();

    SWE(int32_t, start);
    SWE(uint32_t, size);
    SWE(float, start_coordinate);
    SWE(uint32_t, stored_size);

    return 0;
}

F(sblk_direntry, czi_subblock_direntry) {
    READM();

    SWE(uint32_t, pixel_type);
    SWE(uint64_t, file_position);
    SWE(uint32_t, file_part);
    SWE(uint32_t, compression);
    SWE(uint32_t, dimension_count);
    
    return 0;
}

/* nested structures, so don't read everything in at once */
F(subblock, czi_subblock) {
    READM2(metadata_size);
    READM2(attachment_size);
    READM2(data_size);

    SWE(uint32_t, metadata_size);
    SWE(uint32_t, attachment_size);
    SWE(uint64_t, data_size);

    return czi_read_sblk_direntry(ctx, &data->dir_entry);
}

F(directory, czi_directory) {
    READM();

    SWE(uint32_t, entry_count);

    return 0;
}

F(atmt_entry, czi_attach_entry) {
    READM();

    SWE(uint64_t, file_position);
    SWE(int32_t, file_part);

    return 0;
}

/* another nested structure */
F(attach, czi_attach) {
    READM2(data_size);
    READM2(reserved1);

    if (czi_read_atmt_entry(ctx, &data->att_entry) == -1)
        return -1;

    READM2(reserved2);

    SWE(uint32_t, data_size);

    return 0;
}

F(atmt_dir, czi_attach_dir) {
    READM();

    SWE(uint32_t, entry_count);

    return 0;
}



