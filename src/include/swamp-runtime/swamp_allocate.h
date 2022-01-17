/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H

#include <stdint.h>
#include <stddef.h>

#include <swamp-runtime/types.h>
#include <swamp-runtime/dynamic_memory.h>

const struct SwampString* swampStringAllocate( SwampDynamicMemory* self, const char* s);
const SwampString* swampStringAllocateWithSize(SwampDynamicMemory* self, const char* s, size_t stringLength);

struct SwampFunc* swampFuncAllocate( SwampDynamicMemory* self, const uint8_t* opcodes, size_t opcodeCount,
                                    size_t parametersOctetSize, size_t returnOctetSize);
struct SwampCurryFunc * swampCurryFuncAllocate( SwampDynamicMemory* self, uint16_t typeIdIndex, uint8_t firstAlign, const SwampFunc* sourceFunc, const void* parameters, size_t parametersOctetSize);

const  SwampList* swampListEmptyAllocate( SwampDynamicMemory* self);
const  SwampList* swampListAllocate(SwampDynamicMemory* self, const void* items, size_t itemCount, size_t itemSize, size_t itemAlign);
const SwampArray* swampArrayAllocate(SwampDynamicMemory* self, const void* items, size_t itemCount, size_t itemSize,
                                   size_t itemAlign);
SwampArray* swampArrayAllocatePrepare(SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t itemAlign);
SwampList* swampListAllocatePrepare( SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t itemAlign);
const  SwampList* swampListAllocateNoCopy( SwampDynamicMemory* self, const void* itemMemory, size_t itemCount, size_t itemSize, size_t itemAlign);
const  SwampList* swampAllocateListAppendNoCopy( SwampDynamicMemory* self, const SwampList* a, const SwampList* b);
SwampBlob* swampBlobAllocate( SwampDynamicMemory* self, const uint8_t* octets, size_t octetCount);
SwampBlob* swampBlobAllocatePrepare( SwampDynamicMemory* self, size_t octetCount);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H
