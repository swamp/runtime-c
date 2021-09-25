/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/context.h>
#include <tiny-libc/tiny_libc.h>

void swampContextInit(SwampMachineContext* self, SwampDynamicMemory* dynamicMemory,
                      const SwampStaticMemory* staticMemory, const struct SwtiChunk* typeInfo)
{
    self->dynamicMemory = dynamicMemory;
    uint8_t* stackMemory = malloc(32*1024);
    swampStackMemoryInit(&self->stackMemory, stackMemory, 32*1024);
    self->bp = self->stackMemory.memory;
    self->tempResult = malloc(2 * 1024);
    self->typeInfo = typeInfo;
    self->constantStaticMemory = staticMemory;
}

void swampContextReset(SwampMachineContext* self)
{
    self->bp = self->stackMemory.memory;
}

void swampContextDestroy(SwampMachineContext* self)
{
    free(self->stackMemory.memory);
    free(self->tempResult);
}

void swampContextCreateTemp(SwampMachineContext* target, const SwampMachineContext* context)
{
    swampContextInit(target, context->dynamicMemory, context->constantStaticMemory, context->typeInfo);
}