/*---------------------------------------------------------------------------------------------
*  Copyright (c) Peter Bjorklund. All rights reserved.
*  Licensed under the MIT License. See LICENSE in the project root for license information.
*--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_STATIC_MEMORY_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_STATIC_MEMORY_H

#include <stdint.h>
#include <stddef.h>

typedef struct SwampStaticMemory {
   uint8_t* memory;
   size_t maxAllocatedSize;
} SwampStaticMemory;

void swampStaticMemoryInit(SwampStaticMemory* self, const void* memory, size_t maxOctetSize);
const void* swampStaticMemoryGet(const SwampStaticMemory* self, uint32_t position);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DYNAMIC_MEMORY_H
