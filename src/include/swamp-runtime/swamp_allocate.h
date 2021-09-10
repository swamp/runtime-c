/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H

struct SwampDynamicMemory;
struct SwampString;
struct SwampFunc;

const struct SwampString* swampStringAllocate(struct SwampDynamicMemory* self, const char* s);
struct SwampFunc* swampFuncAllocate(struct SwampDynamicMemory* self, const uint8_t* opcodes, size_t opcodeCount,
                                    size_t parametersOctetSize, size_t returnOctetSize);

const SwampList* swampListEmptyAllocate(SwampDynamicMemory* self);
const SwampList* swampListAllocate(SwampDynamicMemory* self, const void* items, size_t itemCount, size_t itemSize);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H
