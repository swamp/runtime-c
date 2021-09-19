/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <stddef.h>
#include <swamp-runtime/fixup.h>
#include <swamp-runtime/types.h>

#include <swamp-runtime/core/math.h>

void logMemory(const uint8_t* octets, size_t count) {
    const uint8_t* p = octets;
    for (size_t i=0; i<count; ++i) {
        uint8_t data = *p;
        CLOG_INFO("%zu %02X", i, data);
        p++;
    }
}


const SwampFunc* swampFixupLedger(const uint8_t* const dynamicMemoryOctets, const SwampConstantLedgerEntry* entries) {
    const SwampFunc* entryFunc = 0;
    const SwampConstantLedgerEntry* entry = entries;
    while (entry->constantType != 0) {
        CLOG_INFO("ledger: %d %d", entry->constantType, entry->offset);
        const uint8_t* p = (dynamicMemoryOctets + entry->offset);
        switch (entry->constantType) {
            case LedgerTypeFunc: {
                SwampFunc* func = (const SwampFunc*)p;
                CLOG_INFO("func: opcode count %d", func->opcodeCount)
                CLOG_INFO("func: opcode offset %d", func->opcodes)
                func->opcodes =  (const uint8_t *) (dynamicMemoryOctets + (uintptr_t)func->opcodes);
                CLOG_INFO("func: first opcode: %02X", *func->opcodes);
                entryFunc = func;
            } break;
            case LedgerTypeExternalFunc: {
                SwampFunctionExternal* func = (const SwampFunctionExternal *)p;
                CLOG_INFO("func external: parameter count %d", func->parameterCount)
                func->function2 = swampCoreMathRemainderBy;
                func->returnValue.pos = 0;
                func->returnValue.range = 4;
                func->parameters[0].pos = 4;
                func->parameters[0].range = 4;
                func->parameters[1].pos = 8;
                func->parameters[1].range = 4;
            } break;
            case LedgerTypeString: {
                SwampString* str = (const SwampString *)p;
                CLOG_INFO("str: character count %d", str->characterCount)
                str->characters = (const char*) (dynamicMemoryOctets + (uintptr_t)str->characters);
                CLOG_INFO("str: characters: %s", str->characters);
            } break;
            default: {
                CLOG_ERROR("Unknown ledger fixup")
            }
        }
        entry++;
    }

    return entryFunc;
}
