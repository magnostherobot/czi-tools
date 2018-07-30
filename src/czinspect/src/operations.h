#ifndef _OPERATIONS_H
#define _OPERATIONS_H

#include "types.h"

/* extraction flags */
#define EXT_F_META   0x01
#define EXT_F_ATTACH 0x02
#define EXT_F_SBLK   0x04

void do_scan(struct config *);
void do_extract(struct config *);
void do_dump(struct config *);
void do_check(struct config *);

#endif /* _OPERATIONS_H */

