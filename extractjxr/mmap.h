#ifndef _MMAP_H
#define _MMAP_H

int mapsetup(int);

int xread(void *, size_t);
void xseek_forward(off_t);
void xseek_backward(off_t);
void xseek_set(off_t);
off_t xseek_offset();

#endif /* _MMAP_H */