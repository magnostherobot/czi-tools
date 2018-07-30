#ifndef _CONFIG_H
# define _CONFIG_H

# define POSIXVER 200909L

# if defined(__APPLE__) || defined(__MACH__)
#  undef _DARWIN_C_SOURCE
#  define _DARWIN_C_SOURCE POSIXVER
# else
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE POSIXVER
# endif

# undef POSIXVER

#endif /* _CONFIG_H */
