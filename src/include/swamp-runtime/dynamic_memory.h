/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DYNAMIC_MEMORY_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DYNAMIC_MEMORY_H

#include <stdint.h>
#include <stddef.h>

typedef struct SwampDynamicMemory {
    uint8_t* memory;
    size_t maxAllocatedSize;
    uint8_t* p;
} SwampDynamicMemory;

void swampDynamicMemoryInit(SwampDynamicMemory* self, void* memory, size_t maxOctetSize);
void* swampDynamicMemoryAlloc(SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t align);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DYNAMIC_MEMORY_H
