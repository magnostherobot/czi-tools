#ifndef _MAPFILE_H
#define _MAPFILE_H

#include <stdint.h>

#include "types.h"

int map_configure(struct config *);

#define map_open(f, o, m)      _map_open(f, -1, 0, o, m)
#define map_mopen(f, d, s, m)  _map_open(f, d, s, 0, m)

#define map_close(m)           _map_close(m, 1)
#define map_mclose(m)          _map_close(m, 0)

struct map_ctx *_map_open(char *, int, size_t, int, int);
void _map_close(struct map_ctx *, int);

#define map_file_offset(c)     (c->chunknum * c->chunklen + c->offset)
#define map_file_remaining(c)  (c->fsize - map_file_offset(c))
#define map_chunk_ptr(c)       (c->ptr + c->offset)
#define map_chunk_remain(c)    (c->chunklen - c->offset)

/* whence flags for map_seek() */
#define MAP_SET   1   /* set the file offset */
#define MAP_FORW  2   /* current offset plus argument */
#define MAP_BACK  3   /* current offset minus argument */

int map_seek(struct map_ctx *, size_t, int);

int map_read(struct map_ctx *, void *, size_t);
int map_dwrite(struct map_ctx *, int, size_t);
int map_splice(struct map_ctx *, struct map_ctx *, size_t);

#endif /* _MAPFILE_H */

