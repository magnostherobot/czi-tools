/*
 * Minunit taken from Jera Design's "Tech Notes":
 * http://www.jera.com/techinfo/jtns/jtn002.html
 */

#ifndef _MINUNIT_H
#define _MINUNIT_H

#define mu_assert(message, test) do { \
  if (!(test)) return message; \
} while (0)

#define mu_run_test(test) do { \
  char *message = test(); tests_run++; \
  if (message) return message; \
} while (0)

extern int tests_run;

#endif /* _MINUNIT_H */
