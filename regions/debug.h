#ifndef _DEBUG_H

// For fprintf:
#include <stdio.h>

#define debug(m, ...) fprintf(stderr, m "\n", __VA_ARGS__);

#define _DEBUG_H
#endif
