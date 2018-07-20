#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cziSegmentHeaders.h"

void writeDataToFile(const char* filename, int fileCount, const char* optionalFileExtension, void* content, long contentSize);
char extractDirName[100];
long totalFileSize;
long lastPercentFileProgress = -1;
SegmentHeader* segHead;
SubBlockDirectory* zisDirectory;
SubBlockSegment* zisSubBlock;
AttachmentSegment* attachmentSegment;
AttachmentDirectory* attachmentDirectory;
MetadataSegment* metadataSegment;



void setFileOffset(FILE* streamPointer, long currentPosition, SegmentHeader* segment) {
    // if (segment->used_size == 0) {
        fseek(streamPointer, currentPosition + segment->allocated_size, SEEK_SET);
    // } else {
        // fseek(streamPointer, currentPosition + segment->used_size, SEEK_SET);
    // }
}

int findSegmentID() {

    if (strcmp(segHead->name, "ZISRAWSUBBLOCK") == 0) {
        return 3;
    }
    else if (strcmp(segHead->name, "ZISRAWATTACH") == 0) {
        return 5;
    }
    else if (strcmp(segHead->name, "ZISRAWATTDIR") == 0) {
        return 6;
    }
    else if (strcmp(segHead->name, "ZISRAWDIRECTORY") == 0) {
        return 2;
    }
    else if (strcmp(segHead->name, "DELETED") == 0) {
        return 7;
    }
    else if (strcmp(segHead->name, "ZISRAWMETADATA") == 0) {
        return 4;
    }
    else if (strcmp(segHead->name, "ZISRAWFILE") == 0) {
        return 1;
    }
    else {
        return -1;
    }
}


void loadMainHeader(FILE* imageFile, FILE* outputFile, long* fileOffset) {

    //initial thing.
    fprintf(outputFile,"{\n");
    writeSegmentHeader(segHead, outputFile);

    //main file descriptor
    ZisrawFile* zisFileHeader = malloc(sizeof(ZisrawFile));
    fread(zisFileHeader, sizeof(ZisrawFile), 1, imageFile);

    //print and write to file
//    printZisrawFile(zisFileHeader);
    writeZisrawFile(zisFileHeader, outputFile);
    free(zisFileHeader);
    fprintf(outputFile,"}\n");

    //correct for Zero Paddings
    setFileOffset(imageFile, *fileOffset, segHead);
}


void loadZisDirectories(SubBlockDirectory* zisrawDirectory, FILE* imageFile) {

    //Get the main directory;
    fread(zisrawDirectory, sizeof(SubBlockDirectory), 1, imageFile);
    //Take care for the fact that what we are using as a pointer value in struct size is actually just the first address.
    fseek(imageFile, -8, SEEK_CUR);
    //allocate space for each of the directory entries
    zisrawDirectory->directory_entries = malloc(sizeof(SubBlockDirectoryEntry) * zisrawDirectory->entry_count);

    //now get each Directory entry
    for (uint32_t i = 0; i < zisrawDirectory->entry_count; i++) {

        //read in the directory entry
        fread(&zisrawDirectory->directory_entries[i], sizeof(SubBlockDirectoryEntry), 1, imageFile);
        //Again cater for the struct pointer size difference
        fseek(imageFile, -8, SEEK_CUR);

        //malloc the dimension entry requirements into the above directory entries pointer
        zisrawDirectory->directory_entries[i].dimension_entries = malloc(sizeof(SubBlockDimentionEntry) * zisrawDirectory->directory_entries[i].dimension_count);

        //read in the dimension entries into the new malloc location
        fread(zisrawDirectory->directory_entries[i].dimension_entries, sizeof(SubBlockDimentionEntry), zisrawDirectory->directory_entries[i].dimension_count, imageFile);
    }
}
void freeZisDirectories(SubBlockDirectory* zisrawDirectory) {
    //now get each Directory entry
    for (uint32_t i = 0; i < zisrawDirectory->entry_count; i++) {
        //free the dimension entry requirements into the above directory entries pointer
        free(zisrawDirectory->directory_entries[i].dimension_entries);
    }
    //free space for each of the directory entries
    free(zisrawDirectory->directory_entries);
}


void loadSubBlockSegment(SubBlockSegment* subBlockSegment, FILE* imageFile) {

    long offsetCorrection = ftell(imageFile);
    //Get the fixed size content;
    fread(subBlockSegment, sizeof(SubBlockDirectoryEntry) + 8, 1, imageFile);
    //read in the directoryEntry
    subBlockSegment->directory_entry.dimension_entries = malloc(subBlockSegment->directory_entry.dimension_count * sizeof(SubBlockDimentionEntry));
    //read in the dimensions
    fread(subBlockSegment->directory_entry.dimension_entries, sizeof(SubBlockDimentionEntry), subBlockSegment->directory_entry.dimension_count, imageFile);

    //Deal with the "fill" block
    offsetCorrection = ftell(imageFile) - offsetCorrection;
    offsetCorrection = 256 - offsetCorrection;
    if (offsetCorrection > 0) {
        fseek(imageFile, offsetCorrection, SEEK_CUR);
    }

    // metadata block
    subBlockSegment->metadata = malloc(subBlockSegment->metadata_size);
    fread(subBlockSegment->metadata, 1, subBlockSegment->metadata_size, imageFile);

    // data block
    subBlockSegment->data = malloc(subBlockSegment->data_size);
    fread(subBlockSegment->data, 1, subBlockSegment->data_size, imageFile);

    // attach block
    subBlockSegment->attachments = malloc(subBlockSegment->attachment_size);
    fread(subBlockSegment->attachments, 1, subBlockSegment->attachment_size, imageFile);
}
void freeSubBlockSegment(SubBlockSegment* subBlockSegment) {
    //free the directoryEntry
    free(subBlockSegment->directory_entry.dimension_entries);

    // metadata block
    free(subBlockSegment->metadata);

    // data block
    free(subBlockSegment->data);

    // attach block
    free(subBlockSegment->attachments);
}


void loadAttachmentSegment(AttachmentSegment* attachmentSegment, FILE* imageFile) {

    //Get the fixed size content;
    fread(attachmentSegment, sizeof(AttachmentSegment), 1, imageFile);
    //account for the pointer space
    fseek(imageFile, -8, SEEK_CUR);

    //make space for the data element;
    attachmentSegment->data = malloc(attachmentSegment->data_size);

    //read in the data
    fread(attachmentSegment->data, attachmentSegment->data_size, 1, imageFile);
}
void freeAttachmentSegment(AttachmentSegment* attachmentSegment) {
    //free the data area
    free(attachmentSegment->data);
}


void loadAttachmentDirectory(AttachmentDirectory* attachmentDirectory, FILE* imageFile) {

    //Get the fixed size content;
    fread(attachmentDirectory, sizeof(AttachmentDirectory), 1, imageFile);
    //account for the pointer space
    fseek(imageFile, -8, SEEK_CUR);
    //malloc the space for the attachments;
    attachmentDirectory->attachment_entries = malloc(sizeof(AttachmentEntry) * attachmentDirectory->entry_count);
    //read in the content;
    fread(attachmentDirectory->attachment_entries, sizeof(AttachmentEntry), attachmentDirectory->entry_count, imageFile);
}
void freeAttachmentDirectory(AttachmentDirectory* attachmentDirectory) {
    //free the entries area
    free(attachmentDirectory->attachment_entries);
}

void loadMetadataSegment(MetadataSegment* metadataSegment, FILE* imageFile) {
    fread(metadataSegment, sizeof(MetadataSegment), 1, imageFile);
}
void loadAndWriteMetadataData(MetadataSegment* metadataSegment, FILE* imageFile) {
    void* rawMetadata = malloc(metadataSegment->xml_size);
    fread(rawMetadata, 1, metadataSegment->xml_size, imageFile);
    writeDataToFile("FILE-META-", 1, ".xml", rawMetadata, metadataSegment->xml_size);
    free(rawMetadata);
}


int functionRouter(FILE* imageFile, FILE* jsonOutput, long* fileOffset) {

    //next segment header
    long newPercent = (100 * ftell(imageFile) / totalFileSize);
    if (newPercent > lastPercentFileProgress) {
        lastPercentFileProgress = newPercent;
        printf("> Amount of CZI file extracted: %3ld%%\n", lastPercentFileProgress);
    }

    if (fread(segHead, sizeof(SegmentHeader), 1, imageFile) != 1) {
        return 0;
    }
    *fileOffset = ftell(imageFile);

    //"prettify + print lol"
//    printSegmentHeader(segHead);
    fprintf(jsonOutput,"{\n");
    writeSegmentHeader(segHead, jsonOutput);

    //deal with the specific kind of segement.
    switch (findSegmentID()) {

        //FileTitle
        case 1:
            loadMainHeader(imageFile, jsonOutput, fileOffset);
            break;

        //Directory
        case 2:
            loadZisDirectories(zisDirectory, imageFile);
            writeSubBlockDirectory(zisDirectory, jsonOutput);
            freeZisDirectories(zisDirectory);
            break;

        //Subblock
        case 3:
            loadSubBlockSegment(zisSubBlock, imageFile);
//            printSubBlockSegment(zisSubBlock);
            writeSubBlockSegment(zisSubBlock, jsonOutput);
            freeSubBlockSegment(zisSubBlock);
            break;

        //MetaData
        case 4:
            loadMetadataSegment(metadataSegment, imageFile);
//            printMetadataSegment(metadataSegment);
            writeMetadataSegment(metadataSegment, jsonOutput);
            loadAndWriteMetadataData(metadataSegment, imageFile);
            break;

        //Attseg
        case 5:
            loadAttachmentSegment(attachmentSegment, imageFile);
//            printAttachmentSegment(attachmentSegment);
            writeAttachmentSegment(attachmentSegment, jsonOutput);
            freeAttachmentSegment(attachmentSegment);
            break;

        //Attdir
        case 6:
            loadAttachmentDirectory(attachmentDirectory, imageFile);
//            printAttachmentDirectory(attachmentDirectory);
            writeAttachmentDirectory(attachmentDirectory, jsonOutput);
            freeAttachmentDirectory(attachmentDirectory);
            break;

        //DELETED
        case 7:
            fprintf(jsonOutput, "{}");
            break;

        default:
            fprintf(stderr, "FAILED TO PARSE SEGMENT HEADER: %s", segHead->name);
            exit(1);
    }

    setFileOffset(imageFile, *fileOffset, segHead);
    fprintf(jsonOutput,"},\n");
    return 1;
}

int main(int argc, char* argv[]) {

    //Check program input arguments.
    if (argc < 2 || argc > 3) {
        printf("Invalid number of arguments passed in.\nUsage:\n\n./prog <input file> <outputjson>\n\n");
        exit(1);
    }

    //create file objects;
    FILE* imageFile;
    FILE* jsonOutput;
    long fileOffset = 0;
    strcpy(extractDirName, argv[2]);

    /* open file for input */
    if ((imageFile = fopen(argv[1], "r"))==NULL){
        printf("Read error occurred, check file path.");
        exit(1);
    }
    fseek(imageFile, 0, SEEK_END);
    totalFileSize = ftell(imageFile);
    fseek(imageFile, 0, SEEK_SET);

    /* open file for Output */
    char jsonName[90];
    strcpy(jsonName, extractDirName);
    strcat(jsonName, "outputjson.json");
    if ((jsonOutput = fopen(jsonName, "w"))==NULL){
        printf("Write error occurred, check file path.");
        exit(1);
    }
    fprintf(jsonOutput,"{ \"czi\": [\n");


    //create heap space for a header and directory stuff.
    segHead = malloc(sizeof(SegmentHeader));
    zisDirectory = malloc(sizeof(SubBlockDirectory));
    zisSubBlock = malloc(sizeof(SubBlockSegment));
    attachmentSegment = malloc(sizeof(AttachmentSegment));
    attachmentDirectory = malloc(sizeof(AttachmentDirectory));
    metadataSegment = malloc(sizeof(MetadataSegment));


    printf("\n=========== BEGIN CZI EXTRACTION ===========\n\n");
    while (functionRouter(imageFile, jsonOutput, &fileOffset));
    //get rid of the last comma
    fseek(jsonOutput, -2, SEEK_CUR);
    //close stuff
    free(segHead);
    free(zisDirectory);
    free(zisSubBlock);
    free(attachmentSegment);
    free(attachmentDirectory);
    free(metadataSegment);
    fclose(imageFile);
    fprintf(jsonOutput,"\n]}\n");
    fclose(jsonOutput);

    printf("\n=========== FINISH CZI EXTRACTION ===========\n\n\n\n");
    return 0;
}
