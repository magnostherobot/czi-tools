#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* simple config test program */

/* 
 * this program tests machine byte order and pointer size. we also test if we're
 * compiling on Darwin, as slightly different macros are required to expose recent
 * POSIX functions.
 */

const char posixmacro[] =
#if defined(__APPLE__) || defined(__MACH__)
    "_DARWIN_C_SOURCE"
#else
    "_POSIX_C_SOURCE"
#endif
    ;

int main() {
    uint32_t u32 = 0x44332211;
    uint32_t *u32ptr = &u32;
    unsigned char *cptr = (unsigned char *) u32ptr;
    unsigned long posixver = 200809L;

    printf("#ifndef _CONFIG_H\n");
    printf("#define _CONFIG_H\n");

    /* test address space size */
    if (sizeof(uintptr_t) <= sizeof(uint32_t)) {
        printf("#define SLIDING_MMAP\n");
    }

    /* test endianness */
    if (*cptr == 0x44) {
        printf("#define IS_BIG_ENDIAN\n");
    } else if (*cptr != 0x11) { /* detect weirdness */
        errx(1, "could not determine machine endianness");
    }

    printf("#define %s %luL\n", posixmacro, posixver);
    
    printf("#endif /* _CONFIG_H */\n");
    
    exit(0);
}
