/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_unpack_h
#define swamp_unpack_h

#include <swamp-runtime/types.h>
#include <swamp-typeinfo/chunk.h>
#include <swamp-runtime/ledger.h>

struct SwampFunc;

typedef struct SwampOctetStream {
    const uint8_t* octets;
    size_t octetCount;
    size_t position;
} SwampOctetStream;


typedef struct SwampUnpack {
    const struct SwampFunc* entry;
    int verboseFlag;
    SwtiChunk typeInfoChunk;
    const uint8_t* constantStaticMemoryOctets;
    size_t constantStaticMemorySize;
    size_t constantStaticMemoryMaxSize;

    SwampLedger ledger;

} SwampUnpack;

typedef void* (*SwampResolveExternalFunction)(const char* fullyQualifiedName);

void swampUnpackInit(SwampUnpack* self, int verboseFlag);
void swampUnpackFree(SwampUnpack* self);
int swampUnpackFilename(SwampUnpack* self, const char* packFilename, SwampResolveExternalFunction bindFn, int verboseFlag);
int swampUnpackOctetStream(SwampUnpack* self, SwampOctetStream* s, SwampResolveExternalFunction bindFn, int verboseFlag);
struct SwampFunc* swampUnpackEntryPoint(SwampUnpack* self);

#endif
