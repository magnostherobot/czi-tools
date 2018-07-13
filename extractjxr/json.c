
#include <stdint.h>
#include <string.h>

#include "zeiss.h"
#include "json.h"

static int dojson;

void czi_set_json(int val) {
    dojson = val;
}

int czi_doing_json() {
    return dojson;
}

void czi_start_yajl() {

}

