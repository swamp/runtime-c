/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/dynamic_memory.h>
#include <swamp-runtime/log.h>

void swampDynamicMemoryInit(SwampDynamicMemory* self, void* memory, size_t maxOctetSize)
{
    self->memory = memory;
    self->p = memory;
    self->maxAllocatedSize = maxOctetSize;
}

void* swampDynamicMemoryAlloc(SwampDynamicMemory* self, size_t itemCount, size_t itemSize)
{
    size_t total = itemCount *itemSize;
    if (self->p + total - self->memory > self->maxAllocatedSize) {
        SWAMP_LOG_ERROR("overrrun");
        return 0;
    }
    void* allocated = self->p;
    self->p += itemCount * itemSize;

    return allocated;
}