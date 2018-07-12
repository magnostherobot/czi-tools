#include <inttypes.h>
#include <uuid/uuid.h>

typedef uuid_t GUID;
extern char extractDirName[100];


//START FILE WRITER ////////////////////////////////////////////////////////////
void writeDataToFile(const char* filename, int fileCount, const char* optionalFileExtension, void* content, long contentSize) {
    FILE* outputFile;
    char outputFileName[150];
    sprintf(outputFileName, "%s%s%d%s", extractDirName, filename, fileCount, optionalFileExtension);
    if ((outputFile = fopen(outputFileName, "w"))==NULL){
        printf("Write error occurred, check file path.");
        exit(1);
    }
    fwrite(content, 1, contentSize, outputFile);
    fclose(outputFile);
}
//END FILE WRITER //////////////////////////////////////////////////////////////



//START SEGMENT HEADERS/////////////////////////////////////////////////////////
typedef struct {
    char name[16];
    uint64_t allocated_size;
    uint64_t used_size;
} __attribute__ ((__packed__)) SegmentHeader;

void printSegmentHeader(SegmentHeader* header) {
    printf("name:           %s\n", header->name);
    printf("allocated_size:  %" PRIu64 "\n", header->allocated_size);
    printf("used_size:      %" PRIu64 "\n", header->used_size);
    printf("\n");
}
void writeSegmentHeader(SegmentHeader* header, FILE* outputFile) {
    fprintf(outputFile, "\"Id\": \"%s\",\n", header->name);
    fprintf(outputFile, "\"AllocatedSize\": %" PRIu64 ",\n", header->allocated_size);
    fprintf(outputFile, "\"UsedSize\": %" PRIu64 ",\n", header->used_size);
    fprintf(outputFile, "\"Data\": \n");
}
//END SEGMENT HEADERS///////////////////////////////////////////////////////////



//START ZISRAWFILE /////////////////////////////////////////////////////////////
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t reserved1;
    uint32_t reserved2;
    GUID primary_file_guid;
    GUID file_guid;
    uint32_t file_part;
    uint64_t directory_position;
    uint64_t metadata_position;
    uint32_t update_pending;
    uint64_t attachment_directory_potition;
} __attribute__ ((__packed__)) ZisrawFile;

void printZisrawFile(ZisrawFile* header) {
    printf("major:                         %" PRIu32 "\n", header->major);
    printf("minor:                         %" PRIu32 "\n", header->minor);
    printf("reserved1:                     %" PRIu32 "\n", header->reserved1);
    printf("reserved2:                     %" PRIu32 "\n", header->reserved2);
    printf("primary_file_guid:             ");
    for (int i = 0; i < 16; i++) {
        printf("%u ",header->primary_file_guid[i]);
    }
    printf("\nfile_guid:                     ");
    for (int i = 0; i < 16; i++) {
        printf("%u ",header->file_guid[i]);
    }
    printf("\nfile_part:                     %" PRIu32 "\n", header->file_part);
    printf("directory_position:            %" PRIu64 "\n", header->directory_position);
    printf("metadata_position:             %" PRIu64 "\n", header->metadata_position);
    printf("update_pending:                %" PRIu32 "\n", header->update_pending);
    printf("attachment_directory_potition: %" PRIu64 "\n", header->attachment_directory_potition);
    printf("\n");
}
void writeZisrawFile(ZisrawFile* header, FILE* outputFile) {
    fprintf(outputFile, "{\n");
    fprintf(outputFile, "\"Major\": %" PRIu32 ",\n", header->major);
    fprintf(outputFile, "\"Minor\": %" PRIu32 ",\n", header->minor);
    fprintf(outputFile, "\"Reserved1\": %" PRIu32 ",\n", header->reserved1);
    fprintf(outputFile, "\"Reserved2\": %" PRIu32 ",\n", header->reserved2);
    fprintf(outputFile, "\"PrimaryFileGuid\": \n[");
    int i = 0;
    for (; i < 15; i++) {
        fprintf(outputFile, "%u, ",header->primary_file_guid[i]);
    }
    fprintf(outputFile, "%u],\n",header->primary_file_guid[i]);
    fprintf(outputFile, "\"FileGuid\": \n[");
    for (i = 0; i < 15; i++) {
        fprintf(outputFile, "%u, ",header->file_guid[i]);
    }
    fprintf(outputFile, "%u],\n",header->file_guid[i]);
    fprintf(outputFile, "\"FilePart\": %" PRIu32 ",\n", header->file_part);
    fprintf(outputFile, "\"DirectoryPosition\": %" PRIu64 ",\n", header->directory_position);
    fprintf(outputFile, "\"MetadataPosition\": %" PRIu64 ",\n", header->metadata_position);
    fprintf(outputFile, "\"UpdatePending\": %" PRIu32 ",\n", header->update_pending);
    fprintf(outputFile, "\"AttachmentDirectoryPosition\": %" PRIu64 "\n", header->attachment_directory_potition);
    fprintf(outputFile, "}\n");
}
//END ZISRAWFILE ///////////////////////////////////////////////////////////////



//START SubBlockDirectory/////////////////////////////////////////////////////////
typedef struct {
    char dimension[4];
    int32_t start;
    uint32_t size;
    float start_coordinate;
    uint32_t stored_size;
} __attribute__ ((__packed__)) SubBlockDimentionEntry;

typedef struct {
    char schema[2];
    uint32_t pixel_type;
    uint64_t file_position;
    uint32_t file_part;
    uint32_t compression;
    char pyramidType;
    char spare1;
    char spare2[4];
    uint32_t dimension_count;
    SubBlockDimentionEntry* dimension_entries;
} __attribute__ ((__packed__)) SubBlockDirectoryEntry;

typedef struct {
    uint32_t entry_count;
    char reserved[124];
    SubBlockDirectoryEntry* directory_entries;
} __attribute__ ((__packed__)) SubBlockDirectory;

void printSubBlockDimentionEntry(SubBlockDimentionEntry* dimensionEntry) {
    // printf("        dimension:         ");
    // for (int i = 0; i < 4; i++) {
    //     printf("%c", dimensionEntry->dimension[i]);
    // }

    printf("        dimension:         %s\n", dimensionEntry->dimension);

    printf("        start:             %" PRIi32 "\n", dimensionEntry->start);
    printf("        size:              %" PRIu32 "\n", dimensionEntry->size);
    printf("        start_coordinate:  %f\n", dimensionEntry->start_coordinate);
    printf("        stored_size:       %" PRIu32 "\n", dimensionEntry->stored_size);
}
void writeSubBlockDimentionEntry(SubBlockDimentionEntry* dimensionEntry, FILE* outputFile) {
    fprintf(outputFile, "{\n");

    // fprintf(outputFile, "\"Dimension\": \"");
    // for(int i = 0; i < 4; i++) {
    //     fprintf(outputFile, "%c", dimensionEntry->dimension[i]);
    // }
    // fprintf(outputFile, "\",\n\"Start\": %" PRIi32 ",\n", dimensionEntry->start);

    fprintf(outputFile, "\"Dimension\": \"%s\",\n", dimensionEntry->dimension);
    fprintf(outputFile, "\"Start\": %" PRIi32 ",\n", dimensionEntry->start);

    fprintf(outputFile, "\"Size\": %" PRIu32 ",\n", dimensionEntry->size);
    fprintf(outputFile, "\"StartCoordinate\": %f,\n", dimensionEntry->start_coordinate);
    fprintf(outputFile, "\"StoredSize\": %" PRIu32 "\n", dimensionEntry->stored_size);
    fprintf(outputFile, "}\n");
}

void printSubBlockDirectoryEntry(SubBlockDirectoryEntry* directoryEntry) {
    printf("    schema:            %c%c\n", directoryEntry->schema[0], directoryEntry->schema[1]);
    printf("    pixel_type:        %" PRIu32 "\n", directoryEntry->pixel_type);
    printf("    file_position:     %" PRIu64 "\n", directoryEntry->file_position);
    printf("    file_part:         %" PRIu32 "\n", directoryEntry->file_part);
    printf("    compression:       %" PRIu32 "\n", directoryEntry->compression);
    printf("    pyramidType:       %u\n", directoryEntry->pyramidType);
    printf("    spare1:            %d\n", directoryEntry->spare1);
    printf("    spare2:            %d %d %d %d\n", directoryEntry->spare2[0], directoryEntry->spare2[1], directoryEntry->spare2[2], directoryEntry->spare2[3]);
    printf("    dimension_count:   %" PRIu32 "\n", directoryEntry->dimension_count);
    printf("    dimension_entries: \n");
    for (uint32_t i = 0; i < directoryEntry->dimension_count; i++) {
        printSubBlockDimentionEntry(&directoryEntry->dimension_entries[i]);
    }
    printf("\n");
}
void writeSubBlockDirectoryEntry(SubBlockDirectoryEntry* directoryEntry, FILE* outputFile) {
    fprintf(outputFile, "{\n");
    fprintf(outputFile, "\"SchemaType\": \"%c%c\",\n", directoryEntry->schema[0],directoryEntry->schema[1]);
    fprintf(outputFile, "\"PixelType\": %" PRIu32 ",\n", directoryEntry->pixel_type);
    fprintf(outputFile, "\"FilePosition\": %" PRIu64 ",\n", directoryEntry->file_position);
    fprintf(outputFile, "\"FilePart\": %" PRIu32 ",\n", directoryEntry->file_part);
    fprintf(outputFile, "\"Compression\": %" PRIu32 ",\n", directoryEntry->compression);
    fprintf(outputFile, "\"PyramidType\": \"%u\",\n", directoryEntry->pyramidType);
    fprintf(outputFile, "\"spare1\": %d,\n", directoryEntry->spare1);
    fprintf(outputFile, "\"spare2\": \"%d %d %d %d\",\n", directoryEntry->spare2[0], directoryEntry->spare2[1], directoryEntry->spare2[2], directoryEntry->spare2[3]);
    fprintf(outputFile, "\"DimensionCount\": %" PRIu32 ",\n", directoryEntry->dimension_count);
    fprintf(outputFile, "\"DimensionEntries\": \n[\n");
    uint32_t i = 0;
    for (; i < directoryEntry->dimension_count - 1; i++) {
        writeSubBlockDimentionEntry(&directoryEntry->dimension_entries[i], outputFile);
        fprintf(outputFile, ", ");
    }
    writeSubBlockDimentionEntry(&directoryEntry->dimension_entries[i], outputFile);
    fprintf(outputFile, "]}\n");
}

void printSubBlockDirectory(SubBlockDirectory* zisDirectory) {
    printf("entry_count:         %" PRIu32 "\n", zisDirectory->entry_count);
    printf("reserved:            %s\n", zisDirectory->reserved);
    printf("directory_entries:    \n");
    for (uint32_t i = 0; i < zisDirectory->entry_count; i++) {
        printSubBlockDirectoryEntry(&zisDirectory->directory_entries[i]);
    }
}
void writeSubBlockDirectory(SubBlockDirectory* zisDirectory, FILE* outputFile) {
    fprintf(outputFile, "{\n");
    fprintf(outputFile, "\"EntryCount\": %" PRIu32 ",\n", zisDirectory->entry_count);
    fprintf(outputFile, "\"Reserved\": \"%s\",\n", zisDirectory->reserved);
    fprintf(outputFile, "\"Entry\": \n[\n");
    uint32_t i = 0;
    for (; i < zisDirectory->entry_count - 1; i++) {
        writeSubBlockDirectoryEntry(&zisDirectory->directory_entries[i], outputFile);
        fprintf(outputFile, ", ");
    }
    writeSubBlockDirectoryEntry(&zisDirectory->directory_entries[i], outputFile);
    fprintf(outputFile, "]}\n");
}
//END SubBlockDirectory///////////////////////////////////////////////////////////



//START METADATASEGMENT/////////////////////////////////////////////////////////
typedef struct {
    uint32_t xml_size;
    uint32_t attachment_size;
    char spare[248];
}  __attribute__ ((__packed__)) MetadataSegment;

void printMetadataSegment(MetadataSegment* metaSegement) {
    printf("xml_size:         %" PRIu32 "\n", metaSegement->xml_size);
    printf("attachment_size:  %" PRIu32 "\n", metaSegement->attachment_size);
    printf("spare:            \n");
    printf("Document Name:    %s", "FILE-META-1.xml");
}
void writeMetadataSegment(MetadataSegment* metaSegement, FILE* outputFile) {
    fprintf(outputFile, "{\n");
    fprintf(outputFile, "\"XmlSize\": %" PRIu32 ",\n", metaSegement->xml_size);
    fprintf(outputFile, "\"AttachmentSize\": %" PRIu32 ",\n", metaSegement->attachment_size);
    fprintf(outputFile, "\"<spare>\": \"<256-8>\",\n");
    fprintf(outputFile, "\"XmlName\": \"FILE-META-1.xml\"\n");
    fprintf(outputFile, "}\n");
}
//END METADATASEGMENT///////////////////////////////////////////////////////////



//START SUBBLOCKSEGMENT/////////////////////////////////////////////////////////
typedef struct {
    uint32_t metadata_size;
    uint32_t attachment_size;
    uint64_t data_size;
    SubBlockDirectoryEntry directory_entry;
    char* metadata;
    void* data;
    void* attachments;
}  __attribute__ ((__packed__)) SubBlockSegment;

void printSubBlockSegment(SubBlockSegment* subBlockSegment) {
    static int META_COUNT = 0;
    static int DATA_COUNT = 0;
    static int ATTACH_COUNT = 0;
    printf("metadata_size:    %" PRIu32 "\n", subBlockSegment->metadata_size);
    printf("attachment_size:  %" PRIu32 "\n", subBlockSegment->attachment_size);
    printf("data_size:        %" PRIu64 "\n", subBlockSegment->data_size);
    printf("spare:            \n");
    printSubBlockDirectoryEntry(&subBlockSegment->directory_entry);
    if (strncmp(subBlockSegment->metadata, "<METADATA />", subBlockSegment->metadata_size) != 0) {
        printf("metadata:         \"metadata-%d\"\n", META_COUNT++);
    } else {
        printf("metadata:         \"empty\"\n");
    }
    printf("data:             \"data-%d\"\n", DATA_COUNT++);
    if (subBlockSegment->attachment_size != 0) {
        printf("attatchments:     \"attach-%d\"\n", ATTACH_COUNT++);
    } else {
        printf("attatchments:     \"empty\"\n");
    }
}

int findEqualDimension(SubBlockDirectoryEntry* directoryEntry, const char* dimensionName) {

    for (uint32_t i = 0; i < directoryEntry->dimension_count; i++) {
        if (strcmp(directoryEntry->dimension_entries[i].dimension, dimensionName) == 0) {
            if (directoryEntry->dimension_entries[i].size == directoryEntry->dimension_entries[i].stored_size) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0;
}

void writeSubBlockSegment(SubBlockSegment* subBlockSegment, FILE* outputFile) {

    static int META_COUNT = 0;
    static int DATA_COUNT = 0;
    static int ATTACH_COUNT = 0;

    fprintf(outputFile, "{\n");
    fprintf(outputFile, "\"MetadataSize\": %" PRIu32 ",\n", subBlockSegment->metadata_size);
    fprintf(outputFile, "\"AttachmentSize\": %" PRIu32 ",\n", subBlockSegment->attachment_size);
    fprintf(outputFile, "\"DataSize\": %" PRIu64 ",\n", subBlockSegment->data_size);
    fprintf(outputFile, "\"DirectoryEntry\": ");
    writeSubBlockDirectoryEntry(&subBlockSegment->directory_entry, outputFile);
    fprintf(outputFile, ",\n\"Fill\": \"\",\n");

    /* open file for Output */
    if (strncmp(subBlockSegment->metadata, "<METADATA />", subBlockSegment->metadata_size) != 0) {
        writeDataToFile("metadata-", META_COUNT, ".xml", subBlockSegment->metadata, subBlockSegment->metadata_size);
        fprintf(outputFile, "\"Metadata\": \"metadata-%d.xml\",\n", META_COUNT++);
    } else {
        fprintf(outputFile, "\"Metadata\": \"empty\",\n");
    }

    if (findEqualDimension(&subBlockSegment->directory_entry, "X") && findEqualDimension(&subBlockSegment->directory_entry, "Y")) {
        writeDataToFile("data-", DATA_COUNT, ".jxr", subBlockSegment->data, subBlockSegment->data_size);
        fprintf(outputFile, "\"Data\": \"data-%d.png\",\n", DATA_COUNT++);
    } else {
        fprintf(outputFile, "\"Data\": \"empty\",\n");
    }

    if (subBlockSegment->attachment_size != 0) {
        writeDataToFile("attach-", ATTACH_COUNT, "", subBlockSegment->attachments, subBlockSegment->attachment_size);
        fprintf(outputFile, "\"Attachments\": \"attach-%d\"\n", ATTACH_COUNT++);
    } else {
        fprintf(outputFile, "\"Attachments\": \"empty\"\n");
    }
    fprintf(outputFile, "}\n");
}
//END SUBBLOCKSEGMENT///////////////////////////////////////////////////////////







//START ATTACHMENTENTRY/////////////////////////////////////////////////////////
typedef struct {
    char schema_type[2];
    char reserved[10];
    uint64_t file_position;
    int file_part;
    GUID content_guid;
    char content_file_type[8];
    char name[80];
}  __attribute__ ((__packed__)) AttachmentEntry;

void printAttachmentEntry(AttachmentEntry* attachmentSegment){
    printf("    schema_type:       \"%c%c\"\n", attachmentSegment->schema_type[0],attachmentSegment->schema_type[1]);
    printf("    reserved:          %s\n", attachmentSegment->reserved);
    printf("    file_position:     %" PRIu64 "\n", attachmentSegment->file_position);
    printf("    file_part:         %d\n", attachmentSegment->file_part);
    printf("    ContentGuid:       [");
    int i = 0;
    for (; i < 15; i++) {
        printf("%u, ", attachmentSegment->content_guid[i]);
    }
    printf("%u]\n", attachmentSegment->content_guid[i]);
    printf("    content_file_type: \"%s\"\n", attachmentSegment->content_file_type);
    printf("    name:              \"%s\"\n", attachmentSegment->name);
}
void writeAttachmentEntry(AttachmentEntry* attachmentSegment, FILE* outputFile){
    fprintf(outputFile, "{\n");
    fprintf(outputFile, "\"SchemaType\": \"%c%c\",\n",  attachmentSegment->schema_type[0],attachmentSegment->schema_type[1]);
    fprintf(outputFile, "\"Reserved\": \"%s\",\n", attachmentSegment->reserved);
    fprintf(outputFile, "\"FilePosition\": %" PRIu64 ",\n", attachmentSegment->file_position);
    fprintf(outputFile, "\"FilePart\": %d,\n", attachmentSegment->file_part);
    fprintf(outputFile, "\"ContentGuid\": \n[");
    int i = 0;
    for (; i < 15; i++) {
        fprintf(outputFile, "%u, ", attachmentSegment->content_guid[i]);
    }
    fprintf(outputFile, "%u],\n", attachmentSegment->content_guid[i]);
    fprintf(outputFile, "\"ContentFileType\": \"%s\",\n", attachmentSegment->content_file_type);
    fprintf(outputFile, "\"Name\": \"%s\"\n", attachmentSegment->name);
    fprintf(outputFile, "}\n");
}

//END ATTACHMENTENTRY///////////////////////////////////////////////////////////






//START ATTACHMENTSEGMENT///////////////////////////////////////////////////////
typedef struct {
    uint32_t data_size;
    char spare1[12];
    AttachmentEntry attachment_entry;
    char spare2[112];
    void* data;
}  __attribute__ ((__packed__)) AttachmentSegment;

void printAttachmentSegment(AttachmentSegment* attachmentSegment){

    static int ATTACH_DATA_COUNT = 0;

    printf("data_size:    %" PRIu32 "\n", attachmentSegment->data_size);
    printAttachmentEntry(&attachmentSegment->attachment_entry);
    printf("data:             \"attach-data-%d\"\n", ATTACH_DATA_COUNT++);
}
void writeAttachmentSegment(AttachmentSegment* attachmentSegment, FILE* outputFile){

    static int ATTACH_DATA_COUNT = 0;

    fprintf(outputFile, "{\n");
    fprintf(outputFile, "\"DataSize\": %" PRIu32 ",\n", attachmentSegment->data_size);
    fprintf(outputFile, "\"AttachmentEntry\": ");
    writeAttachmentEntry(&attachmentSegment->attachment_entry, outputFile);

    /* open file for Output */
    writeDataToFile("attach-data-", ATTACH_DATA_COUNT, "", attachmentSegment->data, attachmentSegment->data_size);

    fprintf(outputFile, ",\n\"Data\": \"attach-data-%d\"\n", ATTACH_DATA_COUNT++);
    fprintf(outputFile, "}\n");
}
//END ATTACHMENTSEGMENT/////////////////////////////////////////////////////////



//END ATTACHMENTDIRECTORY///////////////////////////////////////////////////////
typedef struct {
    int entry_count;
    char reserved[252];
    AttachmentEntry* attachment_entries;
}  __attribute__ ((__packed__)) AttachmentDirectory;

void printAttachmentDirectory(AttachmentDirectory* attachmentDirectory){
    printf("entry_count:    %d\n", attachmentDirectory->entry_count);
    for (int i = 0; i < attachmentDirectory->entry_count; i++) {
        printAttachmentEntry(&attachmentDirectory->attachment_entries[i]);
    }
}
void writeAttachmentDirectory(AttachmentDirectory* attachmentDirectory, FILE* outputFile){
    fprintf(outputFile, "{\n");
    fprintf(outputFile, "\"EntryCount\": %d,\n", attachmentDirectory->entry_count);
    fprintf(outputFile, "\"Entry\": [\n");
    int i = 0;
    for (; i < attachmentDirectory->entry_count - 1; i++) {
        writeAttachmentEntry(&attachmentDirectory->attachment_entries[i], outputFile);
        fprintf(outputFile, ", ");
    }
    writeAttachmentEntry(&attachmentDirectory->attachment_entries[i], outputFile);
    fprintf(outputFile, "]}\n");
}
//END ATTACHMENTDIRECTORY///////////////////////////////////////////////////////
