#ifndef _CZINSPECT_H
#define _CZINSPECT_H

#include <stdint.h>

#include "config.h"

struct config {
    char *infile;
    uint16_t operation;
    uint8_t help;
    uint8_t progress_bar;
    
    char *outdir;

    char *outfile;
    
#ifdef SMALL_ADDRESS
    size_t page_multiplier;
#endif
};

#endif /* _CZINSPECT_H */
