#ifndef _EXTRACT_JXR_H
#define _EXTRACT_JXR_H

/* global values used by extractjxr.c and other code */

off_t czifileoffset;
long czifilesize;
int czifd;
int dirfd;
int jsonfd;

int doextract;
int dojson;

#endif /* _EXTRACT_JXR_H */