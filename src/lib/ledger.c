/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/ledger.h>
#include <swamp-runtime/types.h>
#include <swamp-runtime/fixup.h>
#include <clog/clog.h>
#include <swamp-runtime/debug.h>

void swampLedgerInit(SwampLedger* self, const uint8_t* ledgerOctets, size_t ledgerSize, const uint8_t* constantStaticMemory)
{
    self->ledgerOctets =ledgerOctets;
    self->ledgerSize = ledgerSize;
    self->constantStaticMemory = constantStaticMemory;
}

const SwampFunc* swampLedgerFindFunction(const SwampLedger* self, const char* name)
{
    const uint8_t* const constantStaticMemory = self->constantStaticMemory;
    const SwampConstantLedgerEntry* entries = ( const SwampConstantLedgerEntry*) self->ledgerOctets;

    const SwampConstantLedgerEntry* entry = entries;
    while (entry->constantType != 0) {
        const uint8_t* p = (constantStaticMemory + entry->offset);
        switch (entry->constantType) {
            case LedgerTypeFunc: {
                const SwampFunc* func = (const SwampFunc*) p;
                if (tc_str_equal(func->debugName, name)) {
                    return func;
                }
            }
        }
        entry++;
    }

    return 0;
}

const SwampDebugInfoFiles* swampLedgerGetDebugInfoFiles(const SwampLedger* self)
{
    const uint8_t* const constantStaticMemory = self->constantStaticMemory;
    const SwampConstantLedgerEntry* entries = ( const SwampConstantLedgerEntry*)  self->ledgerOctets;

    const SwampConstantLedgerEntry* entry = entries;
    while (entry->constantType != 0) {
        const uint8_t* p = (constantStaticMemory + entry->offset);
        switch (entry->constantType) {
            case LedgerTypeDebugInfoFiles: {
                const SwampDebugInfoFiles* debugInfoFiles = (const SwampDebugInfoFiles*) p;
                    return debugInfoFiles;
            }
        }
        entry++;
    }

    return 0;
}

const SwampResourceNameChunkEntry* swampLedgerFindResourceNames(const SwampLedger* self)
{
    const uint8_t* const constantStaticMemory = self->constantStaticMemory;
    const SwampConstantLedgerEntry* entries = ( const SwampConstantLedgerEntry*)  self->ledgerOctets;

    const SwampConstantLedgerEntry* entry = entries;
    while (entry->constantType != 0) {
        const uint8_t* p = (constantStaticMemory + entry->offset);
        switch (entry->constantType) {
            case LedgerTypeResourceNameChunk: {
                const SwampResourceNameChunkEntry* resourceNames = (const SwampResourceNameChunkEntry*) p;
                //CLOG_INFO("resource name count %d first is '%s'", resourceNames->resourceCount, *resourceNames->resourceNames);
                return resourceNames;
            }
        }
        entry++;
    }

    return 0;
}
