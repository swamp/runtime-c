/*---------------------------------------------------------------------------------------------
*  Copyright (c) Peter Bjorklund. All rights reserved.
*  Licensed under the MIT License. See LICENSE in the project root for license information.
*--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/static_memory.h>

void swampStaticMemoryInit(SwampStaticMemory* self, const void* memory, size_t maxOctetSize)
{
   self->memory = memory;
   self->maxAllocatedSize = maxOctetSize;
}

const void* swampStaticMemoryGet(const SwampStaticMemory* self, uint32_t position)
{
    if (position >= self->maxAllocatedSize) {
        CLOG_ERROR("position is invalid");
    }

    return self->memory + position;
}
