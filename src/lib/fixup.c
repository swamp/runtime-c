/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <stddef.h>
#include <swamp-runtime/fixup.h>
#include <swamp-runtime/types.h>

#include <swamp-runtime/core/math.h>
#include <swamp-runtime/swamp_unpack.h>

void logMemory(const uint8_t* octets, size_t count) {
    const uint8_t* p = octets;
    for (size_t i=0; i<count; ++i) {
        uint8_t data = *p;
        CLOG_INFO("%zu %02X", i, data);
        p++;
    }
}

#define FIXUP_DYNAMIC_POINTER(field, type) field = (type) (dynamicMemoryOctets + (uintptr_t)field)
#define FIXUP_DYNAMIC_STRING(field) FIXUP_DYNAMIC_POINTER(field, const char*)

const SwampFunc* swampFixupLedger(const uint8_t* const dynamicMemoryOctets, SwampResolveExternalFunction bindFn, const SwampConstantLedgerEntry* entries) {
    const SwampFunc* entryFunc = 0;
    const SwampConstantLedgerEntry* entry = entries;
    while (entry->constantType != 0) {
        CLOG_INFO("= ledger: constant type:%d position:%d", entry->constantType, entry->offset);
        const uint8_t* p = (dynamicMemoryOctets + entry->offset);
        switch (entry->constantType) {
            case LedgerTypeFunc: {
                SwampFunc* func = (const SwampFunc*)p;
                FIXUP_DYNAMIC_POINTER(func->opcodes, const uint8_t *);
                FIXUP_DYNAMIC_STRING(func->debugName);
                CLOG_INFO("  func: '%s' opcode count %d first opcode: %02X", func->debugName, func->opcodeCount, *func->opcodes)
                if (tc_str_equal(func->debugName, "main")) {
                    entryFunc = func;
                }
            } break;
            case LedgerTypeExternalFunc: {
                SwampFunctionExternal* func = (const SwampFunctionExternal *)p;
                FIXUP_DYNAMIC_STRING(func->fullyQualifiedName);
                CLOG_INFO("looking up external function '%s'", func->fullyQualifiedName);
                void* resolvedFunctionPointer = bindFn(func->fullyQualifiedName);
                if (resolvedFunctionPointer == 0) {
                    CLOG_ERROR("you must provide pointer for function '%s'", func->fullyQualifiedName);
                }
                switch (func->parameterCount) {
                    case 1:
                        func->function1 = resolvedFunctionPointer;
                        break;
                    case 2:
                        func->function2 = resolvedFunctionPointer;
                        break;
                    case 3:
                        func->function3 = resolvedFunctionPointer;
                        break;
                    case 4:
                        func->function4 = resolvedFunctionPointer;
                        break;
                    default:
                        CLOG_ERROR("paramcount above 4 or below 1 is not supported (%d)", func->parameterCount)
                }
                CLOG_INFO("set now as parameter count %d", func->parameterCount);
                CLOG_INFO("  externalFunction: %s parameter count %d", func->fullyQualifiedName, func->parameterCount)
                CLOG_INFO("  externalFunction external: return pos %d range %d", func->returnValue.pos, func->returnValue.range);
                for (size_t i=0; i<func->parameterCount;++i) {
                    CLOG_INFO("  externalFunction: param %d pos %d range %d", i, func->parameters[i].pos, func->parameters[i].range);
                }


            } break;
            case LedgerTypeString: {
                SwampString* str = (const SwampString *)p;
                FIXUP_DYNAMIC_STRING(str->characters);
                CLOG_INFO("  str: characters: %s (%d)", str->characters, str->characterCount);
            } break;
            default: {
                CLOG_ERROR("Unknown ledger fixup")
            }
        }
        entry++;
    }

    return entryFunc;
}