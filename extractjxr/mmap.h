#ifndef _MMAP_H
#define _MMAP_H

int mapsetup(int);

int xread(void *, size_t);
void xseek(off_t);

#endif /* _MMAP_H */