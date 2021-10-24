/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/context.h>
#include <tiny-libc/tiny_libc.h>

void swampCallstackAlloc(SwampCallStack * self)
{
    self->maxCount = 1024;
    self->entries = tc_malloc_type_count(SwampCallStackEntry, self->maxCount);
    self->count = 0;
}

void swampCallstackDestroy(SwampCallStack* self)
{
    self->maxCount = 0;
    self->count = 0;
    tc_free(self->entries);
}

void swampContextInit(SwampMachineContext* self, SwampDynamicMemory* dynamicMemory,
                      const SwampStaticMemory* staticMemory, const struct SwtiChunk* typeInfo)
{
    self->dynamicMemory = dynamicMemory;
    uint8_t* stackMemory = tc_malloc(32*1024);
    swampStackMemoryInit(&self->stackMemory, stackMemory, 32*1024);
    self->bp = self->stackMemory.memory;
    self->tempResult = malloc(2 * 1024);
    self->typeInfo = typeInfo;
    self->constantStaticMemory = staticMemory;
    swampCallstackAlloc(&self->callStack);
}



void swampContextReset(SwampMachineContext* self)
{
    //self->bp = self->stackMemory.memory;
}

void swampContextDestroy(SwampMachineContext* self)
{
    tc_free(self->dynamicMemory->memory);
    tc_free(self->dynamicMemory);
    tc_free(self->stackMemory.memory);
    tc_free(self->tempResult);
    swampCallstackDestroy(&self->callStack);
}

void swampContextDestroyTemp(SwampMachineContext* self)
{
    tc_free(self->stackMemory.memory);
    swampCallstackDestroy(&self->callStack);
}

void swampContextCreateTemp(SwampMachineContext* target, const SwampMachineContext* context)
{
    uint8_t* stackMemory = tc_malloc(32*1024);
    swampStackMemoryInit(&target->stackMemory, stackMemory, 32*1024);
    target->bp = target->stackMemory.memory;
    target->tempResult = context->tempResult;
    target->dynamicMemory = context->dynamicMemory;
    target->typeInfo = context->typeInfo;
    target->constantStaticMemory = context->constantStaticMemory;
    target->userData = context->userData;
    swampCallstackAlloc(&target->callStack);
}