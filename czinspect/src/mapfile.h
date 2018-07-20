#ifndef _MAPFILE_H
#define _MAPFILE_H

#include "types.h"

int map_configure(struct config *);
struct map_ctx *map_open(char *, int, int);
void map_close(struct map_ctx *);

#endif /* _MAPFILE_H */

