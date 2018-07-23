#ifndef _ZEISSIO_H
#define _ZEISSIO_H

#include "mapfile.h"
#include "zeiss.h"

#define CZI_IO_FUNCS \
    R(sh, czi_seg_header)                       \
    R(zrf, czi_zrf)                             \
    R(metadata, czi_metadata)                   \
    R(sblk_dimentry, czi_subblock_dimentry)     \
    R(sblk_direntry, czi_subblock_direntry)     \
    R(subblock, czi_subblock)                   \
    R(directory, czi_directory)                 \
    R(atmt_entry, czi_attach_entry)             \
    R(attach, czi_attach)                       \
    R(atmt_dir, czi_attach_dir)

#define R(name, type) int czi_read_ ## name (struct map_ctx *, struct type *);
CZI_IO_FUNCS
#undef R

#endif /* _ZEISSIO_H */
