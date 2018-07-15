#ifndef _JSON_H
#define _JSON_H

#include "zeiss.h"

void czi_json_setfd(int);
int czi_json_getfd();

void czi_json_start();
void czi_json_finish();

#define JSON_GEN_FUNCS \
    X(start_sh, (struct czi_seg_header *))                              \
    X(finish_sh, ())                                                    \
    X(write_zrf, (struct czi_zrf *))                                    \
    X(write_uuid, (uuid_t))                                             \
    X(write_deleted, ())                                                \
    X(write_directory, (struct czi_directory *))                        \
    X(write_dir_entry, (struct czi_subblock_direntry *))                \
    X(write_dim_entry, (struct czi_subblock_dimentry *))                \
    X(write_subblock, (struct czi_subblock *, char *, char *, char *))  \
    X(write_metadata, (struct czi_metadata *))                          \
    X(write_attachment, (struct czi_attach *, char *))                  \
    X(write_attach_dir, (struct czi_attach_dir *))


#define X(name, sig) void czi_json_ ## name  sig;
JSON_GEN_FUNCS
#undef X

#define calljson0(funcname) (czi_json_getfd() == -1 ? : czi_json_ ## funcname())
#define calljson(funcname, ...) (czi_json_getfd() == -1 ? : czi_json_ ## funcname(__VA_ARGS__))


#endif /* _JSON_H */
