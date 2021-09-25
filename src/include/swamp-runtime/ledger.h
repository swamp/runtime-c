/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_LEDGER_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_LEDGER_H

#include <stdint.h>
#include <stddef.h>

struct SwampConstantLedgerEntry;
struct SwampFunc;

typedef struct SwampLedger {
    const uint8_t* ledgerOctets;
    size_t ledgerSize;
    const uint8_t* constantStaticMemory;
} SwampLedger;

void swampLedgerInit(SwampLedger* self, const uint8_t* ledgerOctets, size_t ledgerSize, const uint8_t* constantStaticMemory);
const struct SwampFunc* swampLedgerFindFunction(const SwampLedger* self, const char* name);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_LEDGER_H
