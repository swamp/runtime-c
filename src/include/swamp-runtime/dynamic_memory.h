/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DYNAMIC_MEMORY_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DYNAMIC_MEMORY_H

#include <stddef.h>
#include <stdint.h>

typedef struct SwampDynamicMemoryLedgerEntry {
    const char* debugName;
    size_t itemSize;
    size_t itemCount;
    size_t itemAlign;
} SwampDynamicMemoryLedgerEntry;

typedef struct SwampDynamicMemory {
    uint8_t* memory;
    size_t maxAllocatedSize;
    uint8_t* p;
    SwampDynamicMemoryLedgerEntry* ledgerEntries;
    size_t ledgerCount;
    size_t ledgerCapacity;
    size_t ownAlloc;
} SwampDynamicMemory;

void swampDynamicMemoryInit(SwampDynamicMemory* self, void* memory, size_t maxOctetSize);
void swampDynamicMemoryInitOwnAlloc(SwampDynamicMemory* self, size_t maxOctetSize);
void swampDynamicMemoryDestroy(SwampDynamicMemory* self);
void swampDynamicMemoryDebugOutput(const SwampDynamicMemory* self);
void swampDynamicMemoryReset(SwampDynamicMemory* self);

void* swampDynamicMemoryAlloc(SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t align);
void* swampDynamicMemoryAllocDebug(SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t align,
                                   const char* debug);
size_t swampDynamicMemoryAllocatedSize(const SwampDynamicMemory* self);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DYNAMIC_MEMORY_H
