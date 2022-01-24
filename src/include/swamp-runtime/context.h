/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H

#include <swamp-runtime/dynamic_memory.h>
#include <swamp-runtime/stack_memory.h>
#include <swamp-runtime/static_memory.h>
#include <monotonic-time/monotonic_time.h>

struct SwtiChunk;
struct SwampFunc;
struct SwampDebugInfoFiles;

typedef struct SwampCallStackEntry {
    const uint8_t* pc;
    const uint8_t* basePointer;
    const struct SwampFunc* func;
    MonotonicTimeNanoseconds debugBeforeTimeNs;
} SwampCallStackEntry;

typedef struct SwampCallStack {
    SwampCallStackEntry* entries;
    size_t count;
    size_t maxCount;
} SwampCallStack;


typedef struct SwampMachineContext {
    SwampStackMemory stackMemory;
    uint8_t* bp;
    SwampDynamicMemory* dynamicMemory;
    const SwampStaticMemory* constantStaticMemory;
    uint8_t* tempResult;
    size_t tempResultSize;
    const struct SwtiChunk* typeInfo;
    void* userData;
    SwampCallStack callStack;
    const struct SwampDebugInfoFiles* debugInfoFiles;
    const struct SwampMachineContext* parent;
    const char* debugString;
} SwampMachineContext;

void swampContextInit(SwampMachineContext* self, SwampDynamicMemory* memory, const SwampStaticMemory* constantStaticMemory,
                      const struct SwtiChunk* typeInfo, const char* debugString);
void swampContextReset(SwampMachineContext* self);
void swampContextDestroy(SwampMachineContext* self);
void swampContextCreateTemp(SwampMachineContext* target, const SwampMachineContext* context, const char* debugString);
void swampContextDestroyTemp(SwampMachineContext* self);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H
