#ifndef _DEBUG_H
#define _DEBUG_H

#ifndef NDEBUG

// For fprintf:
#include <stdio.h>

#define debug(...) fprintf(stderr, __VA_ARGS__);

#else

#define debug(...)

#endif /* NDEBUG */

#endif /* _DEBUG_H */
