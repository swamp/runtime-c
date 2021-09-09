/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/stack_memory.h>

void swampStackMemoryInit(SwampStackMemory* self, void* memory, size_t maxSize)
{
    self->memory = memory;
    self->maximumStackMemory = maxSize;
}

