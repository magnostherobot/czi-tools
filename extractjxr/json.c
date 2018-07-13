
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <yajl/yajl_gen.h>

#include "zeiss.h"
#include "json.h"
#include "macros.h"

#define yg_m_open() \
    if (yajl_gen_map_open(gen) != yajl_gen_status_ok) \
        ferrx1(1, "could not open JSON map")

#define yg_m_close() \
    if (yajl_gen_map_close(gen) != yajl_gen_status_ok) \
        ferrx1(1, "could not open JSON map")

#define yg_s(str, len) \
    if (yajl_gen_string(gen, str, len) != yajl_gen_status_ok) \
        ferrx(1, "could not write string value \"%s\"", str)

#define yg_i(i) \
    if (yajl_gen_integer(gen, i) != yajl_gen_status_ok) \
        ferrx(1, "could not write integer value \"%" PRIu64 "\"", i)

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
            ferr1(1, "could not write to output JSON file");
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
    if ((gen = yajl_gen_alloc(NULL)) == NULL)
        ferrx1(1, "could not allocate memory for JSON structure");

    if (!yajl_gen_config(gen, yajl_gen_print_callback, &print_callback, NULL))
        ferrx1(1, "could not set up JSON output callback");

    if (yajl_gen_map_open(gen) != yajl_gen_status_ok)
        ferrx1(1, "could not open JSON map");

    if (yajl_gen_string(gen, "czi", 3) != yajl_gen_status_ok)
        ferrx1(1, "could not create \"czi\" key");

    if (yajl_gen_array_open(gen) != yajl_gen_status_ok)
        ferrx1(1, "could not open JSON array");
}

void czi_json_finish() {
    if (yajl_gen_array_close(gen) != yajl_gen_status_ok)
        ferrx1(1, "could not close JSON array");
    
    if (yajl_gen_map_close(gen) != yajl_gen_status_ok)
        ferrx1(1, "could not close JSON map");
}

void czi_json_start_sh(struct czi_seg_header *header) {
    yg_m_open();
    yg_s("Id",             2); yg_s(header->name, strlen(header->name));
    yg_s("AllocatedSize", 13); yg_i(header->allocated_size);
    yg_s("UsedSize",       8); yg_i(header->used_size);
    yg_s("Data",           4);
}

void czi_json_finish_sh() {
    yg_m_close();
}

