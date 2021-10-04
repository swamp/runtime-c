/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/ledger.h>
#include <swamp-runtime/types.h>
#include <swamp-runtime/fixup.h>
#include <clog/clog.h>

void swampLedgerInit(SwampLedger* self, const uint8_t* ledgerOctets, size_t ledgerSize, const uint8_t* constantStaticMemory)
{
    self->ledgerOctets =ledgerOctets;
    self->ledgerSize = ledgerSize;
    self->constantStaticMemory = constantStaticMemory;
}

const SwampFunc* swampLedgerFindFunction(const SwampLedger* self, const char* name)
{
    const uint8_t* const constantStaticMemory = self->constantStaticMemory;
    const SwampConstantLedgerEntry* entries = self->ledgerOctets;

    const SwampConstantLedgerEntry* entry = entries;
    while (entry->constantType != 0) {
        CLOG_INFO("= ledger: constant type:%d position:%d", entry->constantType, entry->offset);
        const uint8_t* p = (constantStaticMemory + entry->offset);
        switch (entry->constantType) {
            case LedgerTypeFunc: {
                const SwampFunc* func = (const SwampFunc*) p;
                CLOG_INFO("checking '%s'", func->debugName);
                if (tc_str_equal(func->debugName, name)) {
                    return func;
                }
            }
        }
        entry++;
    }

    return 0;
}
