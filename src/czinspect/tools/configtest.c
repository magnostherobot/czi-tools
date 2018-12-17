#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* simple config test program */

/* 
 * this program tests machine pointer size. we also test if we're compiling on
 * Darwin, as slightly different macros are required to expose recent POSIX
 * functions. endianness used to be detected by this program, but there are
 * standard macros exposed by gcc and clang which can be used to detect
 * appropriate endianness.
 */


int main() {
    printf("#ifndef _CONFIG_H\n");
    printf("#define _CONFIG_H\n");

    /* test address space size */
    if (sizeof(uintptr_t) <= sizeof(uint32_t)) {
        printf("#define SLIDING_MMAP\n");
    }

#if defined(__APPLE__)
    printf("#define _DARWIN_C_SOURCE 200809L\n");
#else
    printf("#define _DEFAULT_SOURCE\n");
#endif

    printf("#endif /* _CONFIG_H */\n");
    
    exit(0);
}
