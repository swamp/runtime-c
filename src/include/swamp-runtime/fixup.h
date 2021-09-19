/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_FIXUP_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_FIXUP_H

struct SwampConstantLedgerEntry;
struct SwampFunc;

#include <stdint.h>

typedef struct SwampConstantLedgerEntry {
    uint32_t constantType;
    uint32_t offset;
} SwampConstantLedgerEntry;

#define LedgerTypeExternalFunc (4)
#define LedgerTypeFunc (3)
#define LedgerTypeString (1)


const struct SwampFunc* swampFixupLedger(const uint8_t* const dynamicMemoryOctets, const struct SwampConstantLedgerEntry* entries);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_FIXUP_H
