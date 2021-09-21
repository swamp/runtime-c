/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H

#include <swamp-runtime/dynamic_memory.h>
#include <swamp-runtime/stack_memory.h>


typedef struct SwampMachineContext {
    SwampStackMemory stackMemory;
    uint8_t* bp;
    SwampDynamicMemory dynamicMemory;
    uint8_t* tempResult;
} SwampMachineContext;

void swampContextInit(SwampMachineContext* self, const SwampDynamicMemory* memory);
void swampContextReset(SwampMachineContext* self);
void swampContextDestroy(SwampMachineContext* self);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CONTEXT_H
