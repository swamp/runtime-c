/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_unpack_h
#define swamp_unpack_h

#include <swamp-runtime/types.h>
#include <swamp-typeinfo/chunk.h>

struct SwampFunc;

typedef struct SwampOctetStream {
    const uint8_t* octets;
    size_t octetCount;
    size_t position;
} SwampOctetStream;

void swampOctetStreamInit(SwampOctetStream* self, const uint8_t* octets, size_t octet_count);

typedef struct SwampUnpack {
    struct SwampFunc* entry;
    int verboseFlag;
    SwtiChunk typeInfoChunk;
    const uint8_t* dynamicMemoryOctets;
    size_t dynamicMemorySize;
    size_t dynamicMemoryMaxSize;

    const uint8_t* ledgerOctets;
    size_t ledgerSize;
} SwampUnpack;

void swampUnpackInit(SwampUnpack* self, int verboseFlag);
void swampUnpackFree(SwampUnpack* self);
int swampUnpackFilename(SwampUnpack* self, const char* packFilename, int verboseFlag);
int swampUnpackOctetStream(SwampUnpack* self, SwampOctetStream* s, int verboseFlag);
struct SwampFunc* swampUnpackEntryPoint(SwampUnpack* self);

#endif
