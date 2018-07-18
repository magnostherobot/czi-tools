#ifndef _DEBUG_H

// For fprintf:
#include <stdio.h>

#define debug(...) fprintf(stderr, __VA_ARGS__);

#define _DEBUG_H
#endif
