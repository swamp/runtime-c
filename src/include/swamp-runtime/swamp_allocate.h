/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H

struct SwampDynamicMemory;
struct SwampString;
struct SwampFunc;
struct SwampCurryFunc;

const struct SwampString* swampStringAllocate(struct SwampDynamicMemory* self, const char* s);
struct SwampFunc* swampFuncAllocate(struct SwampDynamicMemory* self, const uint8_t* opcodes, size_t opcodeCount,
                                    size_t parametersOctetSize, size_t returnOctetSize);
struct SwampCurryFunc * swampCurryFuncAllocate(struct SwampDynamicMemory* self, uint16_t typeIdIndex, const struct SwampFunc* sourceFunc, const void* parameters, size_t parametersOctetSize);

const SwampList* swampListEmptyAllocate(struct SwampDynamicMemory* self);
const SwampList* swampListAllocate(struct SwampDynamicMemory* self, const void* items, size_t itemCount, size_t itemSize, size_t itemAlign);
SwampList* swampListAllocatePrepare(struct SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t itemAlign);
const SwampList* swampListAllocateNoCopy(struct SwampDynamicMemory* self, const void* itemMemory, size_t itemCount, size_t itemSize, size_t itemAlign);
const SwampList* swampAllocateListAppendNoCopy(struct SwampDynamicMemory* self, const SwampList* a, const SwampList* b);
SwampBlob* swampBlobAllocate(struct SwampDynamicMemory* self, const uint8_t* octets, size_t octetCount);
SwampBlob* swampBlobAllocatePrepare(struct SwampDynamicMemory* self, size_t octetCount);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H
