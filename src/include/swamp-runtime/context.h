/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H

#include <swamp-runtime/dynamic_memory.h>
#include <swamp-runtime/stack_memory.h>
#include <swamp-runtime/static_memory.h>

struct SwtiChunk;

typedef struct SwampMachineContext {
    SwampStackMemory stackMemory;
    uint8_t* bp;
    SwampDynamicMemory* dynamicMemory;
    const SwampStaticMemory* constantStaticMemory;
    uint8_t* tempResult;
    const struct SwtiChunk* typeInfo;
} SwampMachineContext;

void swampContextInit(SwampMachineContext* self, SwampDynamicMemory* memory, const SwampStaticMemory* constantStaticMemory, const struct SwtiChunk* typeInfo);
void swampContextReset(SwampMachineContext* self);
void swampContextDestroy(SwampMachineContext* self);
void swampContextCreateTemp(SwampMachineContext* target, const SwampMachineContext* context);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H
