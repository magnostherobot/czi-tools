#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* simple config test program */

/* 
 * what we want to know: machine wordsize and endianness 
 *
 * why we want to know it: CZI files are little endian; CZI files are typically
 * several gigabytes in size (i.e. more than four), so processes with a 32-bit
 * address space cannot map the entire file into core at once (therefore partial
 * mappings are required). I will be genuinely shocked if someone tries to run
 * this on a machine whose word size is something other than 32 or 64 bits (if
 * such hardware still exists).
 */

int main() {
    uint32_t u32 = 0x44332211;
    uint32_t *u32ptr = &u32;
    char *cptr = (unsigned char *) u32ptr;

    dprintf(STDOUT_FILENO, "#ifndef _CONFIG_H\n");
    dprintf(STDOUT_FILENO, "#define _CONFIG_H\n");

    /* test address space size */
    if (sizeof(uintptr_t) <= sizeof(uint32_t)) {
        dprintf(STDOUT_FILENO, "#define SLIDING_MMAP\n");
    }

    /* test endianness */
    if (*cptr == 0x44) {
        dprintf(STDOUT_FILENO, "#define BIG_ENDIAN\n");
    } else if (*cptr != 0x11) { /* detect weirdness */
        errx(1, "could not determine machine endianness");
    }

    dprintf(STDOUT_FILENO, "#endif /* _CONFIG_H */\n");
    
    exit(0);
}
