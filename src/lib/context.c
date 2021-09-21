/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/context.h>
#include <memory.h>
#include <tiny-libc/tiny_libc.h>

void swampContextInit(SwampMachineContext* self, const SwampDynamicMemory* memory)
{
    uint8_t* copiedMemory = malloc(memory->maxAllocatedSize);
    uint32_t offset = memory->p - memory->memory;
    tc_memcpy_octets(copiedMemory, memory->memory, offset);

    swampDynamicMemoryInit(&self->dynamicMemory, copiedMemory, memory->maxAllocatedSize);
    self->dynamicMemory.p = self->dynamicMemory.memory + offset;

    uint8_t* stackMemory = malloc(32*1024);

    swampStackMemoryInit(&self->stackMemory, stackMemory, 32*1024);
    self->bp = &self->stackMemory.memory;
    self->tempResult = malloc(2 * 1024);
}

void swampContextReset(SwampMachineContext* self)
{
    self->bp = self->stackMemory.memory;
}

void swampContextDestroy(SwampMachineContext* self)
{
    free(self->dynamicMemory.memory);
    free(self->stackMemory.memory);
    free(self->tempResult);
}