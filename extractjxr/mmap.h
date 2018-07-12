#ifndef _MMAP_H
#define _MMAP_H

#define MAPSIZE (page_size * 4)        /* number of pages to be mapped at one time */
long page_size;

int xread(void *, size_t);
void xseek(off_t);

#endif /* _MMAP_H */