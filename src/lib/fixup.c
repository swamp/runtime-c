/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <stddef.h>
#include <swamp-runtime/fixup.h>
#include <swamp-runtime/types.h>
#include <swamp-runtime/swamp_unpack.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/debug.h>
#include <swamp-runtime/debug_variables.h>

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
    int detectedError = 0;
    while (entry->constantType != 0) {
        //CLOG_INFO("= ledger: constant type:%d position:%d", entry->constantType, entry->offset);
        const uint8_t* p = (dynamicMemoryOctets + entry->offset);
        switch (entry->constantType) {
            case LedgerTypeFunc: {
                SwampFunc* func = (SwampFunc*)p;
                FIXUP_DYNAMIC_STRING(func->debugName);
                FIXUP_DYNAMIC_POINTER(func->opcodes, const uint8_t *);
                FIXUP_DYNAMIC_POINTER(func->debugInfoLines, const SwampDebugInfoLines *);

                FIXUP_DYNAMIC_POINTER(((SwampDebugInfoLines *)func->debugInfoLines)->lines, SwampDebugInfoLinesEntry *);
                //swampDebugInfoLinesOutput(func->debugInfoLines);

                FIXUP_DYNAMIC_POINTER(func->debugInfoVariables, const SwampDebugInfoVariables *);
                FIXUP_DYNAMIC_POINTER(((SwampDebugInfoVariables *)func->debugInfoVariables)->variables, const SwampDebugInfoVariablesEntry *);
                for (size_t i=0; i<func->debugInfoVariables->count; ++i) {
                    SwampDebugInfoVariablesEntry* variablesEntry = (SwampDebugInfoVariablesEntry*) &func->debugInfoVariables->variables[i];
                    FIXUP_DYNAMIC_POINTER(variablesEntry->name, const char*);
                }
                //swampDebugInfoVariablesOutput(func->debugInfoVariables, func->debugName);
                //CLOG_INFO("  func: '%s' opcode count %d first opcode: %02X", func->debugName, func->opcodeCount, *func->opcodes)
                if (tc_str_equal(func->debugName, "main")) {
                    entryFunc = func;
                }
            } break;
            case LedgerTypeExternalFunc: {
                SwampFunctionExternal* func = (SwampFunctionExternal *)p;
                FIXUP_DYNAMIC_STRING(func->fullyQualifiedName);
                //CLOG_INFO("looking up external function '%s'", func->fullyQualifiedName);
                void* resolvedFunctionPointer = bindFn(func->fullyQualifiedName);
                if (resolvedFunctionPointer == 0) {
                    CLOG_SOFT_ERROR("you must provide pointer for function '%s'", func->fullyQualifiedName);
                    detectedError = 1;
                    entry++;
                    continue;
                }
                switch (func->parameterCount) {
                    case 0:
                        func->function0 = resolvedFunctionPointer;
                        break;
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
                    case 5:
                        func->function5 = resolvedFunctionPointer;
                        break;
                    default:
                        CLOG_ERROR("paramcount above 5 or below 0 is not supported (%d)", func->parameterCount)
                }
//                CLOG_INFO("set now as parameter count %d", func->parameterCount);
  //              CLOG_INFO("  externalFunction: %s parameter count %d", func->fullyQualifiedName, func->parameterCount)
    //            CLOG_INFO("  externalFunction external: return pos %d range %d", func->returnValue.pos, func->returnValue.range);
//                for (size_t i=0; i<func->parameterCount;++i) {
  //                  CLOG_INFO("  externalFunction: param %d pos %d range %d", i, func->parameters[i].pos, func->parameters[i].range);
    //            }


            } break;
            case LedgerTypeString: {
                SwampString* str = (SwampString *)p;
                FIXUP_DYNAMIC_STRING(str->characters);
                //CLOG_INFO("  str: characters: %s (%d)", str->characters, str->characterCount);
            } break;
            case LedgerTypeResourceNameChunk: {
                SwampResourceNameChunkEntry* resourceNameChunk = (SwampResourceNameChunkEntry*) p;
                FIXUP_DYNAMIC_POINTER(resourceNameChunk->resourceNames, const char**);
                for (size_t i=0; i<resourceNameChunk->resourceCount; ++i) {
                    const char* str = resourceNameChunk->resourceNames[i];
                    FIXUP_DYNAMIC_STRING(resourceNameChunk->resourceNames[i]);
                }
                //CLOG_INFO("first resource name is '%s'", *resourceNameChunk->resourceNames);
            } break;
            case LedgerTypeResourceName: {
                // Intentionally do nothing
            } break;
            case LedgerTypeDebugInfoFiles: {
                 SwampDebugInfoFiles *debugInfoFiles = (SwampDebugInfoFiles*) p;
                 FIXUP_DYNAMIC_POINTER(debugInfoFiles->filenames, const char**);
                for (size_t i=0; i<debugInfoFiles->count; ++i) {
                    FIXUP_DYNAMIC_STRING(debugInfoFiles->filenames[i]);
                    //CLOG_INFO("file:%s", debugInfoFiles->filenames[i]);
                }
            } break;

            default: {
                CLOG_ERROR("Unknown ledger fixup type %d", entry->constantType)
            }
        }
        entry++;
    }

    if (detectedError) {
        return 0;
    }
    return entryFunc;
}
