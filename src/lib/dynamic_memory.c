/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/dynamic_memory.h>
#include <swamp-runtime/log.h>

void swampDynamicMemoryInit(SwampDynamicMemory* self, void* memory, size_t maxOctetSize)
{
    self->memory = memory;
    self->p = memory;
    self->maxAllocatedSize = maxOctetSize;

    self->ledgerCapacity = 512;
    self->ledgerCount = 0;
    self->ledgerEntries = tc_malloc_type_count(SwampDynamicMemoryLedgerEntry, self->ledgerCapacity);

}

void swampDynamicMemoryDestroy(SwampDynamicMemory* self)
{
    tc_memset_octets(self->memory, 0xbc, self->maxAllocatedSize);
    if (self->ledgerEntries != 0) {
        tc_free(self->ledgerEntries);
    }
}

void swampDynamicMemoryDebugOutput(const SwampDynamicMemory* self)
{
    for (size_t i=0; i<self->ledgerCount; ++i) {
        const SwampDynamicMemoryLedgerEntry* entry = &self->ledgerEntries[i];
        CLOG_INFO("ledger %d: %d '%s' (%dx%d align: %d)", i, entry->itemCount*entry->itemSize, entry->debugName, entry->itemCount, entry->itemSize,  entry->itemAlign);
    }
}

size_t swampDynamicMemoryAllocatedSize(const SwampDynamicMemory* self)
{
    return self->p - self->memory;
}

void* swampDynamicMemoryAlloc(SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t align)
{
    size_t pos = (uintptr_t)self->p - (uintptr_t)self->memory;
    size_t rest = pos % align;
    if (rest != 0) {
        self->p += align - rest;
        //pos += align - rest;
    }

    size_t total = itemCount * itemSize;
    if (self->p + (int)total - self->memory > (long)self->maxAllocatedSize) {
        SWAMP_LOG_ERROR("overrrun dynamic memory. Requested %d items at %d at %d of %d", itemCount, itemSize, self->p - self->memory, self->maxAllocatedSize);
        return 0;
    }

    void* allocated = self->p;

    self->p += total;


    return allocated;
}

void* swampDynamicMemoryAllocDebug(SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t align, const char* debug)
{
    if (self->ledgerCount == self->ledgerCapacity) {
        CLOG_ERROR("out of ledger space");
    }

    SwampDynamicMemoryLedgerEntry * entry = &self->ledgerEntries[self->ledgerCount];
    self->ledgerCount++;
    entry->debugName = debug;
    entry->itemSize = itemSize;
    entry->itemCount = itemCount;
    entry->itemAlign = align;

    return swampDynamicMemoryAlloc(self, itemCount, itemSize, align);
}