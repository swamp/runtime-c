/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_FIXUP_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_FIXUP_H

#include <swamp-runtime/swamp_unpack.h>

struct SwampConstantLedgerEntry;
struct SwampFunc;

#include <stdint.h>

typedef struct SwampResourceNameChunkEntry {
    const char** resourceNames;
    size_t resourceCount;
} SwampResourceNameChunkEntry;

typedef struct SwampConstantLedgerEntry {
    uint32_t constantType;
    uint32_t offset;
} SwampConstantLedgerEntry;

#define LedgerTypeExternalFunc (4)
#define LedgerTypeFunc (3)
#define LedgerTypeResourceName (2)
#define LedgerTypeString (1)
#define LedgerTypeResourceNameChunk (5)

const struct SwampFunc* swampFixupLedger(const uint8_t* const dynamicMemoryOctets, SwampResolveExternalFunction fn, const struct SwampConstantLedgerEntry* entries);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_FIXUP_H
