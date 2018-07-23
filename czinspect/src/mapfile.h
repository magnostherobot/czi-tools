#ifndef _MAPFILE_H
#define _MAPFILE_H

#include <stdint.h>

#include "types.h"

int map_configure(struct config *);
struct map_ctx *map_open(char *, int, int);
void map_close(struct map_ctx *);

/* whence flags for map_seek() */
#define MAP_SET   1   /* set the file offset */
#define MAP_FORW  2   /* current offset plus argument */
#define MAP_BACK  3   /* current offset minus argument */

int map_seek(struct map_ctx *, size_t, int);

int map_read(struct map_ctx *, void *, size_t);
int map_dwrite(struct map_ctx *, int, size_t);
int map_splice(struct map_ctx *, struct map_ctx *, size_t);

#endif /* _MAPFILE_H */

