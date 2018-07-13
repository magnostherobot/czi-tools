
#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <yajl/yajl_gen.h>

#include "zeiss.h"
#include "json.h"

#define IS_JSON_ACTIVE(x) do { if (jsonfd == -1) return (x); } while(0);
#define IS_JSON_ACTIVE_V() do { if (jsonfd == -1) return; } while(0);

static int jsonfd = -1;

static yajl_gen gen;

/* callback passed to yajl which prints the serialised JSON structure to the JSON output file */
static yajl_print_t print_callback(void *ignored, const char *str, size_t len) {
    ssize_t rv;

    while ((rv = write(jsonfd, str, len)) != 0) {
        if (rv == -1) {
            if (errno == EINTR || errno == EAGAIN)
                continue;

            close(jsonfd);
            err(1, "print_callback: could not write to output JSON file");
        }

        str += rv;
        len -= rv;
    }
}

void czi_json_setfd(int val) {
    jsonfd = val;
}

int czi_json_getfd() {
    return jsonfd;
}

void czi_json_start() {
    IS_JSON_ACTIVE_V();
    
    if ((gen = yajl_gen_alloc(NULL)) == NULL)
        errx(1, "czi_json_start: could not allocate memory for JSON structure");

    if (!yajl_gen_config(gen, yajl_gen_print_callback, &print_callback, NULL))
        errx(1, "czi_json_start: could not set up JSON output callback");

    if (yajl_gen_map_open(gen) != yajl_gen_status_ok)
        errx(1, "czi_json_start: could not open JSON map");

    if (yajl_gen_string(gen, "czi", 3) != yajl_gen_status_ok)
        errx(1, "czi_json_start: could not create \"czi\" key");

    if (yajl_gen_array_open(gen) != yajl_gen_status_ok)
        errx(1, "czi_json_start: could not open JSON array");
}

