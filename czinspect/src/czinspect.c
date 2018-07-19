/*
 * Carl Zeiss CZI file format inspection utility.
 *
 * Written by David Miller, based on prior programs written by David Miller and
 * Callum Duff.
 */

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "helptxt.h"
#include "czinspect.h"
#include "macros.h"


/* getopt strings*/
#define GETOPT_OPS    "SEDh"

#define GETOPT_S_STR  ""
#define GETOPT_E_STR  "d:"
#define GETOPT_D_STR  "o:"
#define GETOPT_OTHER  ""

#define GETOPT_SETTINGS "+:"

#define GETOPT_STRING GETOPT_SETTINGS GETOPT_OPS GETOPT_OTHER GETOPT_S_STR \
    GETOPT_E_STR GETOPT_D_STR

/* values for config.operation */
#define OP_NONE      0x00
#define OP_SCAN      0x01
#define OP_EXTRACT   0x02
#define OP_DUMP      0x04

static struct config cfg;

#define START(x) _binary_helptxt_ ## x ## _help_start;
#define END(x) _binary_helptxt_ ## x ## _help_end;

/* print usage information out of embedded help text object file */
void usage() {
    char *start, *end;
    size_t len;
    ssize_t rv;

    switch (cfg.operation) {
    case OP_NONE:
        start = START(main);
        end = END(main);
        break;
    case OP_SCAN:
        start = START(scan);
        end = END(scan);
        break;
    case OP_EXTRACT:
        start = START(extract);
        end = END(extract);
        break;
    case OP_DUMP:
        start = START(dump);
        end = END(dump);
        break;
    default:
        ferrx1(1, "error printing help text: unknown operation");
        break;
    }

    len = end - start;
    while ((rv = write(STDOUT_FILENO, start, len)) != 0) {
        if (rv == -1) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            ferr1(1, "error printing help text");
        }

        start += rv;
        len -= rv;
    }

    exit(0);

#undef START
#undef END
}

/* parse operations (denoted by a capital latin letter) */
static void set_operation(int opt) {
    switch (opt) {
    case 'S':
        cfg.operation |= OP_SCAN;
        break;
    case 'E':
        cfg.operation |= OP_EXTRACT;
        break;
    case 'D':
        cfg.operation |= OP_DUMP;
        break;
    case 'h':
        cfg.help = 1;
        break;
    }
}

/* make sure that only one operation is defined */
static void check_ops() {
    uint8_t num = 0;

    for (int i = 0; i < 4; i++)
        num += (cfg.operation >> i) % 2;

    if (num > 1)
        errx(1, "only one operation may be specified (pass '-h' for help)");

    if (cfg.help)
        usage();

    if (num == 0)
        errx(1, "no operation specified (pass '-h' for help)");

    return;
}

/* print option parsing error message and exit */
static void badopt(int opt) {
    if (opt == ':')
        errx(1, "option '-%c' requires an argument (pass '-h' for help)", optopt);
    else if (opt == '?')
        errx(1, "unknown option '-%c' (pass '-h' for help)", optopt);
    else
        ferrx1(1, "internal option parsing error");
}

/* parse the given command line arguments */
static void parse_args(int argc, char* argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, GETOPT_STRING)) != -1) {
        switch (opt) {
        case ':':
        case '?':
            badopt(opt);
            break;
        default:
            set_operation(opt);
            break;
        }
    }

    check_ops();

    /* reset getopt and reprocess for operation specific options */
    optind = 1;
    switch (cfg.operation) {
    default:
        ferrx1(1, "internal option processing error");
        break;
    }
    
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
}


