/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_STACK_MEMORY_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_STACK_MEMORY_H

#include <stdint.h>
#include <stddef.h>

typedef struct SwampStackMemory {
    uint8_t* memory;
    size_t maximumStackMemory;
} SwampStackMemory;

void swampStackMemoryInit(SwampStackMemory* self, void* memory, size_t octetSize);



#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_STACK_MEMORY_H
