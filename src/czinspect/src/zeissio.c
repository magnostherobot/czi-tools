
#include "config.h"

#include <sys/types.h>
#include <err.h>
#include <stdint.h>

#include "zeiss.h"
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

#ifdef IS_BIG_ENDIAN
# define SW32(v)  data->v = sw32(data->v)
# define SW64(v)  data->v = sw64(data->v)
# define SWF(f)   data->f = swf(data->f)

static uint32_t sw32(uint32_t v) {
    return ((v & 0xff) << 24) | (((v >> 8) & 0xff) << 16) | (((v >> 16) & 0xff) << 8) | ((v >> 24) & 0xff);
}

static uint64_t sw64(uint64_t v) {
    return ((v & 0xff) << 56) | (((v >> 8) & 0xff) << 48) | (((v >> 16) & 0xff) << 40) | (((v >> 24) & 0xff) << 32) |
        (((v >> 32) & 0xff) << 24) | (((v >> 40) & 0xff) << 16) | (((v >> 48) & 0xff) << 8) | ((v >> 56) & 0xff);
}

static float swf(float f) {
    float ret;
    unsigned char *in = (char *) &f;
    unsigned char *out = (char *) &ret;

    out[0] = in[3];
    out[1] = in[2];
    out[2] = in[1];
    out[3] = in[0];
    
    return ret;
}

#else
# define SW32(v)
# define SW64(v)
# define SWF(v)
#endif

/* read in a segment header */
F(sh, czi_seg_header) {
    READM();

    SW64(allocated_size);
    SW64(used_size);
    
    return 0;
}

F(zrf, czi_zrf) {
    READM();

    SW32(major);
    SW32(minor);
    SW32(reserved1);
    SW32(reserved2);

    SW32(file_part);
    SW64(directory_position);
    SW32(metadata_position);
    SW32(update_pending);
    SW64(attachment_directory_position);
        
    return 0;
}

F(metadata, czi_metadata) {
    READM();

    SW32(xml_size);
    SW32(attachment_size);
        
    return 0;
}

F(sblk_dimentry, czi_subblock_dimentry) {
    READM();

    SW32(start);
    SW32(size);
    SWF(start_coordinate);
    SW32(stored_size);
    
    return 0;
}

F(sblk_direntry, czi_subblock_direntry) {
    READM();

    SW32(pixel_type);
    SW64(file_position);
    SW32(file_part);
    SW32(compression);
    SW32(dimension_count);
        
    return 0;
}

/* nested structures, so don't read everything in at once */
F(subblock, czi_subblock) {
    READM2(metadata_size);
    READM2(attachment_size);
    READM2(data_size);

    SW32(metadata_size);
    SW32(attachment_size);
    SW64(data_size);

    return czi_read_sblk_direntry(ctx, &data->dir_entry);
}

F(directory, czi_directory) {
    READM();

    SW32(entry_count);

    return 0;
}

F(atmt_entry, czi_attach_entry) {
    READM();

    SW32(file_position);
    SW32(file_part);

    return 0;
}

/* another nested structure */
F(attach, czi_attach) {
    READM2(data_size);
    READM2(reserved1);

    if (czi_read_atmt_entry(ctx, &data->att_entry) == -1)
        return -1;

    READM2(reserved2);

    SW32(data_size);

    return 0;
}

F(atmt_dir, czi_attach_dir) {
    READM();

    SW32(entry_count);

    return 0;
}



