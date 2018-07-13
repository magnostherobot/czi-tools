#ifndef _JSON_H
#define _JSON_H

#include "zeiss.h"

void czi_json_setfd(int);
int czi_json_getfd();

void czi_json_start();
void czi_json_finish();

#define JSON_GEN_FUNCS \
X(start_sh, (struct czi_seg_header *)) \
X(finish_sh, ())

#define X(name, sig) void czi_json_ ## name  sig;
JSON_GEN_FUNCS
#undef X

#define calljson0(funcname) (czi_json_getfd() == -1 ? : czi_json_ ## funcname())
#define calljson(funcname, ...) (czi_json_getfd() == -1 ? : czi_json_ ## funcname(__VA_ARGS__))


#endif /* _JSON_H */