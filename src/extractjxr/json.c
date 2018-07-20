
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef YAJL_BROKEN
#include <yajl/yajl_gen.h>
#endif

#include "zeiss.h"
#include "json.h"
#include "macros.h"

static int jsonfd = -1;

#ifdef YAJL_BROKEN
static yajl_gen gen;

#define yg_m_open() \
    if (yajl_gen_map_open(gen) != yajl_gen_status_ok) \
        ferrx1(1, "could not open JSON map")

#define yg_m_close() \
    if (yajl_gen_map_close(gen) != yajl_gen_status_ok) \
        ferrx1(1, "could not open JSON map")

#define yg_a_open() \
    if (yajl_gen_array_open(gen) != yajl_gen_status_ok) \
        ferrx1(1, "could not open JSON array")

#define yg_a_close() \
    if (yajl_gen_array_close(gen) != yajl_gen_status_ok) \
        ferrx1(1, "could not open JSON array")

#define yg_s(str) \
    if (yajl_gen_string(gen, str, strlen(str)) != yajl_gen_status_ok) \
        ferrx(1, "could not write string value \"%s\"", str)


#define yg_i(i) \
    if (yajl_gen_integer(gen, i) != yajl_gen_status_ok) \
        ferrx(1, "could not write integer value \"%" PRIu64 "\"", i)

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
    yg_s("Id");            yg_s(header->name);
    yg_s("AllocatedSize"); yg_i(header->allocated_size);
    yg_s("UsedSize");      yg_i(header->used_size);
    yg_s("Data");
}

void czi_json_finish_sh() {
    yg_m_close();
}

void czi_json_write_zrf(struct czi_zrf *zrf) {
    yg_m_open();

    yg_s("Major");             yg_i(zrf->major);
    yg_s("Minor");             yg_i(zrf->minor);
    yg_s("Reserved1");         yg_i(zrf->reserved1);
    yg_s("Reserved2");         yg_i(zrf->reserved2);
    yg_s("PrimaryFileGuid");   czi_json_write_uuid(zrf->primary_file_guid);
    yg_s("FileGuid");          czi_json_write_uuid(zrf->file_guid);
    yg_s("FilePart");          yg_i(zrf->file_part);
    yg_s("DirectoryPosition"); yg_i(zrf->directory_position);
    yg_s("MetadataPosition");  yg_i(zrf->metadata_position);
    yg_s("UpdatePending");     yg_i(zrf->update_pending);
    yg_s("AttachmentDirectoryPosition"); yg_i(zrf->attachment_directory_position);
    
    yg_m_close();
}

void czi_json_write_uuid(uuid_t data) {
    yg_a_open();

    for (int i = 0; i < 15; i++)
        yg_i(data[i]);
    
    yg_a_close();
}

void czi_json_write_deleted() {
    /* yg_m_open(); */
    yg_s("none");
    /* yg_m_close(); */
}

void czi_json_write_directory(struct czi_directory *dir) {
    char reserved[125];

    memcpy(reserved, dir->reserved, 124);
    reserved[124] = '\0';
    
    yg_m_open();

    yg_s("EntryCount"); yg_i(dir->entry_count);
    yg_s("Reserved");   yg_s(reserved);
    yg_s("Entry");      yg_a_open();

    for (uint32_t i = 0; i < dir->entry_count; i++)
        czi_json_write_dir_entry(&dir->dir_entries[i]);

    yg_a_close();
    
    yg_m_close();
}

void czi_json_write_dir_entry(struct czi_subblock_direntry *ent) {
    char schema_type[3] = {ent->schema[0], ent->schema[1], '\0'};
#ifdef CALLUM_COMPAT
    char *spareptr;
#endif
    
    yg_m_open();

    yg_s("SchemaType");   yg_s(schema_type);
    yg_s("PixelType");    yg_i(ent->pixel_type);
    yg_s("FilePosition"); yg_i(ent->file_position);
    yg_s("FilePart");     yg_i(ent->file_part);
    yg_s("Compression");  yg_i(ent->compression);
    yg_s("PyramidType");  yg_i(ent->pyramid_type);

#ifdef CALLUM_COMPAT
    if (asprintf(&spareptr, "%d %d %d %d",
                 ent->reserved2[0],
                 ent->reserved2[1],
                 ent->reserved2[2],
                 ent->reserved2[3]) == -1)
        ferr1(1, "could not allocate memory for \"spare2\" region of directory entry");
    
    yg_s("spare1"); yg_i((unsigned int)ent->reserved1);
    yg_s("spare2"); yg_s(spareptr);

    free(spareptr);
#else
    yg_s("Reserved1"); yg_i((unsigned int)ent->reserved1);
    yg_s("Reserved2"); yg_a_open();
    
    for (int i = 0; i < 4; i++)
        yg_i((unsigned int) ent->reserved2[i]);

    yg_a_close();
#endif

    yg_s("DimensionCount");   yg_i(ent->dimension_count);
    yg_s("DimensionEntries"); yg_a_open();

    for (uint32_t i = 0; i < ent->dimension_count; i++)
        czi_json_write_dim_entry(&ent->dim_entries[i]);
    
    yg_a_close();
    
    yg_m_close();
}

void czi_json_write_dim_entry(struct czi_subblock_dimentry *dim) {
    char dimension[5];

    memcpy(dimension, dim->dimension, 4);
    dimension[4] = '\0';
    
    yg_m_open();

    yg_s("Dimension");       yg_s(dimension);
    yg_s("Start");           yg_i(dim->start);
    yg_s("Size");            yg_i(dim->size);
    yg_s("StartCoordinate"); yajl_gen_double(gen, (double)dim->start_coordinate);
    yg_s("StoredSize");      yg_i(dim->stored_size);
    
    yg_m_close();
}

void czi_json_write_subblock(struct czi_subblock *sblk, char *mname, char *dname, char *aname) {
    yg_m_open();

    yg_s("MetadataSize");   yg_i(sblk->metadata_size);
    yg_s("AttachmentSize"); yg_i(sblk->attachment_size);
    yg_s("DataSize");       yg_i(sblk->data_size);
    yg_s("DirectoryEntry"); czi_json_write_dir_entry(&sblk->dir_entry);
    
    yg_s("Metadata");    if (czi_check_sblk_metadata(sblk)) yg_s(mname); else yg_s("empty");
    yg_s("Data");        if (sblk->data_size > 0)           yg_s(dname); else yg_s("empty");
    yg_s("Attachments"); if (sblk->attachment_size > 0)     yg_s(aname); else yg_s("empty");
    
    yg_m_close();
}

void czi_json_write_metadata(struct czi_metadata *data) {
    yg_m_open();

    yg_s("XmlSize");        yg_i(data->xml_size);
    yg_s("AttachmentSize"); yg_i(data->attachment_size);
    yg_s("XmlName");        yg_s("FILE-META-1.xml");
    
    yg_m_close();
}

void czi_json_write_attach_entry(struct czi_attach_entry *atte) {
    char schema[3];
    char ftype[9];
    char name[81];

    memcpy(schema, atte->schema_type, 2);      schema[2] = '\0';
    memcpy(ftype, atte->content_file_type, 8); ftype[8] = '\0';
    memcpy(name, atte->name, 80);              name[80] = '\0';

    yg_m_open();

    yg_s("SchemaType");      yg_s(schema);
    /* we don't output the reserved section */
    yg_s("FilePosition");    yg_i(atte->file_position);
    yg_s("FilePart");        yg_i(atte->file_part);
    yg_s("ContentGuid");     czi_json_write_uuid(atte->content_guid);
    yg_s("ContentFileType"); yg_s(ftype);
    yg_s("Name");            yg_s(name);
    
    yg_m_close();
}

void czi_json_write_attachment(struct czi_attach *att, char *fname) {
    yg_m_open();

    yg_s("DataSize");        yg_i(att->data_size);
    yg_s("AttachmentEntry"); czi_json_write_attach_entry(&att->att_entry);
    yg_s("Data");            yg_s(fname);
    
    yg_m_close();
}

void czi_json_write_attach_dir(struct czi_attach_dir *attd) {
    yg_m_open();

    yg_s("EntryCount"); yg_i(attd->entry_count);
    yg_s("Entry");      yg_a_open();

    for (uint32_t i = 0; i < attd->entry_count; i++)
        czi_json_write_attach_entry(&attd->att_entries[i]);
    
    yg_a_close();
    yg_m_close();
}

#else

/* handrolled JSON, because yajl gets unhappy */

#define j(...)          dprintf(jsonfd, __VA_ARGS__)
#define k(k)            j("\"%s\":", k)
#define vi(v)           j("%d", v)
#define vs(v)           j("\"%s\"", v)
#define kvu64(k, v)     j("\"%s\":%" PRIu64, k, v)
#define kvu32(k, v)     j("\"%s\":%" PRIu32, k, v)
#define kvi32(k, v)     j("\"%s\":%" PRIi32, k, v)
#define kvd(k, v)       j("\"%s\":%e", k, v)
#define kvs(k, v)       j("\"%s\":\"%s\"", k, v)
#define comma()         j(",")

void czi_json_setfd(int val) {
    jsonfd = val;
}

int czi_json_getfd() {
    return jsonfd;
}

void czi_json_start() {
    j("[");
}

void czi_json_finish() {
    j("]");
}

void czi_json_start_sh(struct czi_seg_header *header) {
    static int first = 0;

    if (first == 0) {
        j("{");
        first = 1;
    } else {
        j(",{");
    }

    kvs("Id", header->name); comma();
    kvu64("AllocatedSize", header->allocated_size); comma();
    kvu64("UsedSize", header->used_size); comma();
    k("Data");
}

void czi_json_finish_sh() {
    j("}");
}

void czi_json_write_zrf(struct czi_zrf *zrf) {
    j("{");

    kvu32("Major", zrf->major); comma();
    kvu32("Minor", zrf->minor); comma();
    k("PrimaryFileGuid"); czi_json_write_uuid(zrf->primary_file_guid); comma();
    k("FileGuid"); czi_json_write_uuid(zrf->file_guid); comma();
    kvu32("FilePart", zrf->file_part); comma();
    kvu64("DirectoryPosition", zrf->directory_position); comma();
    kvu64("MetadataPosition", zrf->metadata_position); comma();
    kvu32("UpdatePending", zrf->update_pending); comma();
    kvu64("AttachmentDirectoryPosition", zrf->attachment_directory_position);
    
    j("}");
}

void czi_json_write_uuid(uuid_t data) {
    j("[");

    vi(data[0]);
    for (int i = 1; i < 16; i++) {
        comma(); vi(data[i]);
    }
    
    j("]");
}

void czi_json_write_deleted() {
    j("{}");
}

void czi_json_write_directory(struct czi_directory *dir) {
    j("{");

    kvu32("EntryCount", dir->entry_count); comma();
    k("Entry"); j("[");

    czi_json_write_dir_entry(&dir->dir_entries[0]);
    for (uint32_t i = 1; i < dir->entry_count; i++) {
        comma(); czi_json_write_dir_entry(&dir->dir_entries[i]);
    }

    j("]");
    
    j("}");
}

void czi_json_write_dir_entry(struct czi_subblock_direntry *ent) {
    char schema_type[3] = {ent->schema[0], ent->schema[1], '\0'};

    j("{");
    
    kvs("SchemaType", schema_type); comma();
    kvu32("PixelType", ent->pixel_type); comma();
    kvu64("FilePosition", ent->file_position); comma();
    kvu32("FilePart", ent->file_part); comma();
    kvu32("Compression", ent->compression); comma();
    kvu32("PyramidType", ent->pyramid_type); comma();
    kvu32("DimensionCount", ent->dimension_count); comma();
    k("DimensionEntries"); j("[");

    czi_json_write_dim_entry(&ent->dim_entries[0]);
    for (uint32_t i = 1; i < ent->dimension_count; i++) {
        comma(); czi_json_write_dim_entry(&ent->dim_entries[i]);
    }

    j("]");
    j("}");
}

void czi_json_write_dim_entry(struct czi_subblock_dimentry *dim) {
    char dimension[5];

    memcpy(dimension, dim->dimension, 4);
    dimension[4] = '\0';

    j("{");

    kvs("Dimension", dimension); comma();
    kvi32("Start", dim->start); comma();
    kvu32("Size", dim->size); comma();
    kvd("StartCoordinate", (double) dim->start_coordinate); comma();
    kvu32("StoredSize", dim->stored_size);

    j("}");
}

void czi_json_write_subblock(struct czi_subblock *sblk, char *mname, char *dname, char *aname) {
    j("{");

    kvu32("MetadataSize", sblk->metadata_size); comma();
    kvu32("AttachmentSize", sblk->attachment_size); comma();
    kvu64("DataSize", sblk->data_size); comma();
    k("DirectoryEntry"); czi_json_write_dir_entry(&sblk->dir_entry); comma();

    k("Metadata");     (sblk->metadata_size > 0 ? vs(mname) : vs("empty")); comma();
    k("Data");         (sblk->data_size > 0 ? vs(dname) : vs("empty")); comma();
    k("Attachments");  (sblk->attachment_size > 0 ? vs(aname) : vs("empty"));
    
    j("}");
}

void czi_json_write_metadata(struct czi_metadata *data) {
    j("{");

    kvu32("XmlSize", data->xml_size); comma();
    kvu32("AttachmentSize", data->attachment_size);  comma();
    kvs("XmlName", "FILE-META-1.xml");
    
    j("}");
}

void czi_json_write_attach_entry(struct czi_attach_entry *atte) {
    char schema[3];
    char ftype[9];
    char name[81];

    memcpy(schema, atte->schema_type, 2);      schema[2] = '\0';
    memcpy(ftype, atte->content_file_type, 8); ftype[8] = '\0';
    memcpy(name, atte->name, 80);              name[80] = '\0';

    j("{");

    kvs("SchemaType", schema); comma();
    kvu64("FilePosition", atte->file_position); comma();
    kvu32("FilePart", atte->file_part); comma();
    k("ContentGuid"); czi_json_write_uuid(atte->content_guid); comma();
    kvs("ContentFileType", ftype); comma();
    kvs("Name", name); 
    
    j("}");
}

void czi_json_write_attachment(struct czi_attach *att, char *fname) {
    j("{");

    kvu32("DataSize", att->data_size);  comma();
    k("AttachmentEntry"); czi_json_write_attach_entry(&att->att_entry); comma();
    kvs("Data", fname);
    
    j("}");
}

void czi_json_write_attach_dir(struct czi_attach_dir *attd) {
    j("{");

    kvu32("EntryCount", attd->entry_count); comma();
    k("Entry"); j("[");

    czi_json_write_attach_entry(&attd->att_entries[0]);
    for (uint32_t i = 1; i < attd->entry_count; i++) {
        comma(); czi_json_write_attach_entry(&attd->att_entries[i]);
    }

    j("]");
    
    j("}");
}

#endif /* YAJL_BROKEN */