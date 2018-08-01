/*
 * Carl Zeiss CZI file format inspection utility.
 *
 * Written by David Miller, based on prior programs written by David Miller and
 * Callum Duff.
 */

#include "config.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helptxt.h"

#include "types.h"
#include "alloc.h"
#include "mapfile.h"
#include "macros.h"
#include "operations.h"

#include "compat/compat.h"

/* getopt strings*/
#define GETOPT_OPS    "SEDCh"

#define GETOPT_S_STR  ""
#define GETOPT_E_STR  "ad:ef:gs:"
#define GETOPT_D_STR  "o:"
#define GETOPT_C_STR  ""

#define GETOPT_GLOBAL "m:"

#define GETOPT_SETTINGS "+:"

#define GETOPT_STRING GETOPT_SETTINGS GETOPT_OPS GETOPT_GLOBAL GETOPT_S_STR \
    GETOPT_E_STR GETOPT_D_STR GETOPT_C_STR

/* values for config.operation */
#define OP_NONE      0x00
#define OP_SCAN      0x01
#define OP_EXTRACT   0x02
#define OP_DUMP      0x04
#define OP_CHECK     0x08

static struct config cfg;

/* print usage information out of embedded help text object file */
void usage() {
    char *start, *end;
    size_t len;
    ssize_t rv;

#define START(x) _binary_helptxt_ ## x ## _help_start;
#define END(x) _binary_helptxt_ ## x ## _help_end;
    
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
    case OP_CHECK:
        start = START(check);
        end = END(check);
        break;
    default:
        ferrx1("error printing help text: unknown operation");
        break;
    }

    len = end - start;
    while ((rv = write(STDOUT_FILENO, start, len)) != 0) {
        if (rv == -1) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            ferr1("error printing help text");
        }

        start += rv;
        len -= rv;
    }

    exit(0);

#undef START
#undef END
}

/* parse operations (denoted by a capital latin letter) */
static void parse_operation(int opt) {
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
    case 'C':
        cfg.operation |= OP_CHECK;
        break;
    case 'h':
        cfg.help = 1;
        break;
    }
}

static int parse_opt_global(int opt) {
    size_t mult;
    const char *errstr;
    
    switch (opt) {
    case 'm':
        mult = strtonum((const char *) optarg, 1, 512, &errstr); /* XXX TODO: empirically figure out a nice upper limit for this */
        if (errstr)
            errx(1, "invalid page multiplier '%s': %s", optarg, errstr);
        cfg.page_multiplier = mult;
        return 1;
    }

    return 0;
}

/* process scanning options */
static void parse_opt_scan(int opt) {
    /* currently, no options which can be used in scanning mode are defined */
    errx(1, "option '-%c' cannot be used with '-S'", opt);
    return;
}

/* process extraction options */
static void parse_opt_extract(int opt) {
    const char *errstr;
    uint16_t filt;
    
    switch (opt) {
    case 'a':
        cfg.eflags |= EXT_F_ATTACH;
        break;
    case 'd':
        cfg.outdir = xstrdup(optarg);
        break;
    case 'e':
        cfg.eflags |= EXT_F_META;
        break;
    case 'f':
        filt = strtonum((const char *) optarg, 1, UINT16_MAX, &errstr);
        if (errstr)
            errx(1, "invalid filter level '%s': %s", optarg, errstr);
        cfg.filter = filt;
        cfg.filtflags |= EXT_FI_FILT;
        break;
    case 'g':
        cfg.filtflags |= EXT_FI_FFUZZ;
        break;
    case 's':
        cfg.eflags |= EXT_F_SBLK;
        cfg.esopts = xstrdup(optarg);
        break;
    default:
        errx(1, "option '-%c' cannot be used with '-E'", opt);
        break;
    }

    return;
}

/* process dumping options */
static void parse_opt_dump(int opt) {
    switch (opt) {
    case 'o':
        cfg.outfile = xstrdup(optarg);
        break;
    default:
        errx(1, "option '-%c' cannot be used with '-D'", opt);
        break;
    }

    return;
}

/* process checking options */
static void parse_opt_check(int opt) {
    /* currently, no options which can be used in checking mode are defined */
    errx(1, "option '-%c' cannot be used with '-C'", opt);
    return;
}

/* make sure that only one operation is defined */
static void check_ops() {
    uint8_t num = 0;

    for (int i = 0; i < sizeof(uint8_t) * 8; i++)
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
        ferrx1("internal option parsing error");
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
            parse_operation(opt);
            break;
        }
    }

    check_ops();

    /* reset getopt and reprocess for operation specific options */
    optind = 1;
    while ((opt = getopt(argc, argv, GETOPT_STRING)) != -1) {
        if (opt == '?' || opt == ':')
            ferrx1("internal option processing error (bad option character)");
        else if (strchr(GETOPT_OPS, opt) != NULL) /* operation found */
            continue;

        if (parse_opt_global(opt))
            continue;
        
        switch (cfg.operation) {
        case OP_SCAN:
            parse_opt_scan(opt);
            continue;
        case OP_EXTRACT:
            parse_opt_extract(opt);
            continue;
        case OP_DUMP:
            parse_opt_dump(opt);
            continue;
        case OP_CHECK:
            parse_opt_check(opt);
            continue;
        default:
            ferrx1("internal option processing error (unknown operation)");
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc == 0)
        errx(1, "missing input filename (pass '-h' for help)");
    else if (argc > 1)
        errx(1, "too many arguments (pass '-h' for help)");

    cfg.infile = argv[0];

    return;
}

int main(int argc, char *argv[]) {
    struct map_ctx *infile_map;
    
    if (isatty(STDOUT_FILENO))
        cfg.progress_bar = 1;

    parse_args(argc, argv);

    if (!map_configure(&cfg))
        errx(1, "could not configure memory-mapped file interface");
    
    if ((infile_map = map_open(cfg.infile, O_RDONLY, PROT_READ)) == NULL)
        errx(1, "could not open input file for reading");

    cfg.inctx = infile_map;
    
    switch (cfg.operation) {
    case OP_SCAN:
        do_scan(&cfg);
        break;
    case OP_DUMP:
        do_dump(&cfg);
        break;
    case OP_EXTRACT:
        do_extract(&cfg);
        break;
    case OP_CHECK:
        do_check(&cfg);
        break;
    default:
        ferrx1("internal configuration error");
        break;
    }

    map_close(infile_map);
    
    exit(0);
}


