/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/dynamic_memory.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>

const SwampString* swampStringAllocateWithSize(SwampDynamicMemory* self, const char* s, size_t stringLength)
{
    SwampString* const string = (SwampString*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampString), 8);
    const char* characters = (char*) swampDynamicMemoryAlloc(self, 1, stringLength + 1, 1);
    tc_memcpy_octets((void*) characters, s, stringLength + 1);
    string->characters = characters;
    string->characterCount = stringLength;

    return string;
}

const SwampString* swampStringAllocate(SwampDynamicMemory* self, const char* s)
{
    size_t stringLength = tc_strlen(s);
    return swampStringAllocateWithSize(self, s, stringLength);
}

const SwampList* swampListEmptyAllocate(SwampDynamicMemory* self)
{
    SwampList* emptyList = (SwampList*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampList), 8);
    emptyList->count = 0;
    emptyList->value = 0;
    emptyList->itemAlign = 0;
    emptyList->itemSize = 0;

    return emptyList;
}

SwampList* swampListAllocatePrepare(SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t itemAlign)
{
    SwampList* newNode = (SwampList*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampList), 8);
    if (itemSize == 0) {
        CLOG_ERROR("itemSize can not be zero");
    }

    if (itemAlign == 0 || itemAlign > 8) {
        CLOG_ERROR("itemAlign can not be zero or more than eight");
    }

    uint8_t* itemMemory = swampDynamicMemoryAlloc(self, itemCount, itemSize, itemAlign);
    newNode->value = itemMemory;
    newNode->itemSize = itemSize;
    newNode->itemAlign = itemAlign;
    newNode->count = itemCount;

    return newNode;
}

SwampArray* swampArrayAllocatePrepare(SwampDynamicMemory* self, size_t itemCount, size_t itemSize, size_t itemAlign)
{
    SwampArray* newNode = (SwampArray*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampArray), 8);
    if (itemSize == 0) {
        CLOG_ERROR("itemSize can not be zero");
    }

    if (itemAlign == 0 || itemAlign > 8) {
        CLOG_ERROR("itemAlign can not be zero or more than eight");
    }

    uint8_t* itemMemory = swampDynamicMemoryAlloc(self, itemCount, itemSize, itemAlign);
    newNode->value = itemMemory;
    newNode->itemSize = itemSize;
    newNode->itemAlign = itemAlign;
    newNode->count = itemCount;

    return newNode;
}

SwampBlob* swampBlobAllocatePrepare(SwampDynamicMemory* self, size_t octetCount)
{
    SwampBlob* newNode = (SwampBlob*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampBlob), 8);

    uint8_t* octetMemory = swampDynamicMemoryAlloc(self, octetCount, 1, 1);
    newNode->octets = octetMemory;
    newNode->octetCount = octetCount;

    return newNode;
}

const SwampList* swampListAllocate(SwampDynamicMemory* self, const void* items, size_t itemCount, size_t itemSize,
                                   size_t itemAlign)
{
    SwampList* list = swampListAllocatePrepare(self, itemCount, itemSize, itemAlign);

    tc_memcpy_octets((void*)list->value, items, itemSize * itemCount);

    return list;
}

const SwampArray* swampArrayAllocate(SwampDynamicMemory* self, const void* items, size_t itemCount, size_t itemSize,
                                     size_t itemAlign)
{
    SwampArray* list = swampArrayAllocatePrepare(self, itemCount, itemSize, itemAlign);

    tc_memcpy_octets((void*)list->value, items, itemSize * itemCount);

    return list;
}

SwampBlob* swampBlobAllocate(SwampDynamicMemory* self, const uint8_t* octets, size_t octetCount)
{
    SwampBlob* blob = swampBlobAllocatePrepare(self, octetCount);

    tc_memcpy_octets((uint8_t *)blob->octets, octets, octetCount);

    return blob;
}

SwampUnmanaged* swampUnmanagedAllocate(SwampDynamicMemory* self)
{
    SwampUnmanaged* newUnmanaged = (SwampUnmanaged*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampUnmanaged), 8);
    newUnmanaged->ptr = 0;

    return newUnmanaged;
}


const SwampList* swampListAllocateNoCopy(SwampDynamicMemory* self, const void* itemMemory, size_t itemCount,
                                         size_t itemSize, size_t itemAlign)
{
    SwampList* newNode = (SwampList*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampList), 8);

    newNode->value = itemMemory;
    newNode->itemSize = itemSize;
    newNode->itemAlign = itemAlign;
    if (itemAlign == 0 || itemAlign > 8) {
        CLOG_ERROR("itemAlign can not be zero or more than eight");
    }

    newNode->count = itemCount;

    return newNode;
}

const SwampList* swampAllocateListAppendNoCopy(SwampDynamicMemory* self, const SwampList* a, const SwampList* b)
{
    if (a->itemSize != b->itemSize && a->count != 0 && b->count != 0) {
        SWAMP_ERROR("must have exactly same item size to be okay")
        return 0;
    }
    if (a->count == 0) {
        return b;
    }

    if (b->count == 0) {
        return a;
    }
    SwampList* newList = (SwampList*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampList), 8);
    uint8_t* itemArray = swampDynamicMemoryAlloc(self, a->count + b->count, a->itemSize, a->itemAlign);
    tc_memcpy_octets(itemArray, a->value, a->count * a->itemSize);
    uint8_t* nextPointer = itemArray + a->count * a->itemSize;
    size_t bTotalSize = b->count * b->itemSize;
    tc_memcpy_octets(nextPointer, b->value, bTotalSize);

    newList->itemSize = a->itemSize;
    newList->itemAlign = a->itemAlign;

    if (a->itemAlign == 0 || a->itemAlign > 8) {
        CLOG_ERROR("itemAlign can not be zero or more than eight");
    }

    newList->count = a->count + b->count;
    newList->value = itemArray;

    return newList;
}

static const uint8_t* swampAllocateOctets(SwampDynamicMemory* self, const uint8_t* octets, size_t octetCount)
{
    uint8_t* target = (uint8_t*) swampDynamicMemoryAlloc(self, 1, octetCount, 1);
    tc_memcpy_octets(target, octets, octetCount);
    return target;
}

SwampFunc* swampFuncAllocate(SwampDynamicMemory* self, const uint8_t* opcodes, size_t opcodeCount,
                             size_t parametersOctetSize, size_t returnOctetSize)
{
    SwampFunc* func = (SwampFunc*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampFunc), 8);
    func->func.type = SwampFunctionTypeInternal;
    func->opcodes = swampAllocateOctets(self, opcodes, opcodeCount);
    func->opcodeCount = opcodeCount;
    func->returnOctetSize = returnOctetSize;
    func->parametersOctetSize = parametersOctetSize;

    return func;
}

SwampCurryFunc* swampCurryFuncAllocate(SwampDynamicMemory* self, uint16_t typeIdIndex, uint8_t firstAlign,
                                       const SwampFunc* sourceFunc, const void* parameters, size_t parametersOctetSize)
{
    SwampCurryFunc* func = (SwampCurryFunc*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampCurryFunc), 8);
    func->func.type = SwampFunctionTypeCurry;
    func->curryFunction = sourceFunc;
    func->typeIdIndex = typeIdIndex;
    func->curryOctetSize = parametersOctetSize;
    func->firstParameterAlign = firstAlign;
    func->curryOctets = swampAllocateOctets(self, parameters, parametersOctetSize);

    return func;
}
