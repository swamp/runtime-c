/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/types.h>
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

void swampUnmanagedMemoryInit(SwampUnmanagedMemory* self)
{
    self->count = 0;
    self->capacity = SWAMP_MACHINE_CONTEXT_UNMANAGED_CONTAINER_COUNT;
    for (size_t i=0; i<self->capacity; ++i) {
        SwampUnmanagedMemoryEntry* entry = &self->unmanaged[i];
        entry->unmanaged = 0;
    }
}

SwampUnmanaged* swampUnmanagedMemoryAllocate(SwampUnmanagedMemory* self, const char* debugName)
{
    if (self->count == self->capacity) {
        CLOG_ERROR("container add: out of space")
        return 0;
    }


    for (size_t i=0; i<self->capacity; ++i) {
        SwampUnmanagedMemoryEntry* entry = &self->unmanaged[i];
        if (!entry->unmanaged) {
            self->count++;
            SwampUnmanaged* newMutable = tc_malloc_type(SwampUnmanaged);
#if SWAMP_UNMANAGED_MEMORY_ENABLE_LOG
            CLOG_VERBOSE("allocating unmanaged '%s' (%p)", debugName, newMutable);
#endif
            newMutable->debugName = tc_str_dup(debugName);

            entry->unmanaged = newMutable;
            return newMutable;
        }
    }

    CLOG_ERROR("could not allocate unmanaged struct");

    return 0;
}

int swampUnmanagedMemoryAdd(SwampUnmanagedMemory* self, const SwampUnmanaged* unmanaged)
{
    if (self->count == self->capacity) {
        CLOG_ERROR("container add: out of space")
        return 0;
    }


    for (size_t i=0; i<self->capacity; ++i) {
        SwampUnmanagedMemoryEntry* entry = &self->unmanaged[i];
        if (!entry->unmanaged) {
            self->count++;
            entry->unmanaged = unmanaged;
#if SWAMP_UNMANAGED_MEMORY_ENABLE_LOG
            CLOG_VERBOSE("adding managed '%s' (%p)", entry->unmanaged->debugName, entry->unmanaged);
#endif
            return 0;
        }
    }

    CLOG_ERROR("could not add unmanaged struct");

    return 0;
}

int swampUnmanagedMemoryOwns(const SwampUnmanagedMemory* self, const SwampUnmanaged* unmanaged)
{
    for (size_t i=0; i<self->capacity; ++i) {
        const SwampUnmanagedMemoryEntry* entry = &self->unmanaged[i];
        if (entry->unmanaged == unmanaged) {
            return 1;
        }
    }

    return 0;
}

void swampUnmanagedMemoryForget(SwampUnmanagedMemory* self, const struct SwampUnmanaged* unmanaged)
{
    for (size_t i=0; i<self->capacity; ++i) {
        SwampUnmanagedMemoryEntry* entry = &self->unmanaged[i];
        if (entry->unmanaged == unmanaged) {
            entry->unmanaged = 0;
            return;
        }
    }

    CLOG_ERROR("did not own that pointer");
}



void swampUnmanagedMemoryMove(SwampUnmanagedMemory* target, SwampUnmanagedMemory* source, const struct SwampUnmanaged* unmanaged)
{
#if SWAMP_UNMANAGED_MEMORY_ENABLE_LOG
    CLOG_VERBOSE("moving unmanaged '%s' (%p)", unmanaged->debugName, unmanaged)
#endif
    swampUnmanagedMemoryForget(source, unmanaged);
    swampUnmanagedMemoryAdd(target, unmanaged);
}

void swampUnmanagedMemoryReset(SwampUnmanagedMemory* self)
{
    for (size_t i=0; i<self->capacity; ++i) {
        SwampUnmanagedMemoryEntry* entry = &self->unmanaged[i];
        if (!entry->unmanaged) {
            continue;
        }
        if (!entry->unmanaged->destroy) {
            CLOG_ERROR("you must specify a destroy function for unmanaged");
        }
#if SWAMP_UNMANAGED_MEMORY_ENABLE_LOG
        CLOG_VERBOSE("unmanaged memory: destroying unreferenced '%s' (%p)", entry->unmanaged->debugName, entry->unmanaged)
#endif
        int result = entry->unmanaged->destroy(entry->unmanaged->ptr);
        tc_free((void*)entry->unmanaged->debugName);
        tc_free((void*)entry->unmanaged);
        if (result < 0) {
            CLOG_ERROR("unmanaged destroy result was bad");
        }

        entry->unmanaged = 0;
    }

    self->count = 0;
}


void swampUnmanagedMemoryDestroy(SwampUnmanagedMemory* self)
{
    swampUnmanagedMemoryReset(self);
}

void swampContextInit(SwampMachineContext* self, SwampDynamicMemory* dynamicMemory,
                      const SwampStaticMemory* staticMemory, const struct SwtiChunk* typeInfo, SwampUnmanagedMemory* unmanagedMemory, const char* debugString)
{
    self->dynamicMemory = dynamicMemory;
    self->unmanagedMemory = unmanagedMemory;
    uint8_t* stackMemory = tc_malloc(32*1024);
    swampStackMemoryInit(&self->stackMemory, stackMemory, 32*1024);
    self->bp = self->stackMemory.memory;
    self->tempResultSize = 2 * 1024;
    self->tempResult = malloc(self->tempResultSize);
    self->typeInfo = typeInfo;
    self->constantStaticMemory = staticMemory;
    self->debugString = debugString;
    self->parent = 0;
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

void swampContextCreateTemp(SwampMachineContext* target, const SwampMachineContext* context, const char* debugString)
{
    uint8_t* stackMemory = tc_malloc(32*1024);
    swampStackMemoryInit(&target->stackMemory, stackMemory, 32*1024);

    if (!context->debugInfoFiles) {
        CLOG_ERROR("Must have debug info");
    }

    target->bp = target->stackMemory.memory;
    target->tempResult = context->tempResult;
    target->dynamicMemory = context->dynamicMemory;
    target->unmanagedMemory = context->unmanagedMemory;
    target->typeInfo = context->typeInfo;
    target->constantStaticMemory = context->constantStaticMemory;
    target->userData = context->userData;
    target->debugInfoFiles = context->debugInfoFiles;
    target->parent = context;
    target->debugString = debugString;
    swampCallstackAlloc(&target->callStack);
}
