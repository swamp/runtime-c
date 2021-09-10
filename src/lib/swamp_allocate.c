/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/dynamic_memory.h>
#include <tiny-libc/tiny_libc.h>
#include <swamp-runtime/types.h>
#include <swamp-runtime/swamp_allocate.h>

const SwampString* swampStringAllocate( SwampDynamicMemory* self, const char* s)
{
    SwampString* const string = (SwampString*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampString));
    size_t stringLength = tc_strlen(s);
    const char* characters = (char*) swampDynamicMemoryAlloc(self, 1, stringLength + 1);
    memcpy((void*)characters, s, stringLength + 1);
    string->characters = characters;
    string->characterCount = stringLength;

    return string;
}

const SwampList* swampListEmptyAllocate(SwampDynamicMemory* self)
{
    SwampList* emptyList = (SwampList*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampList));
    emptyList->next = 0;
    emptyList->count = 0;
    emptyList->value = 0;
}

const SwampList* swampListAllocate(SwampDynamicMemory* self, const void* items, size_t itemCount, size_t itemSize)
{
    uint8_t* itemMemory = swampDynamicMemoryAlloc(self, itemCount, itemSize);
    memcpy(itemMemory, items, itemCount * itemSize);

    SwampList* previousNode = 0;
    for (size_t i = 0; i < itemCount; ++i) {
        SwampList* newNode = (SwampList*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampList));
        if (previousNode) {
            previousNode->next = newNode;
        }
        int index = itemCount - 1 - i;
        newNode->value = itemMemory + index * itemSize;
        newNode->count = i + 1;
        newNode->next = 0;

        previousNode = newNode;
    }

}


const uint8_t* swampAllocateOctets(SwampDynamicMemory* self, const uint8_t* octets, size_t octetCount)
{
    uint8_t* target = (uint8_t*) swampDynamicMemoryAlloc(self, 1, octetCount);
    tc_memcpy_octets(target, octets, octetCount);
    return target;
}

SwampFunc* swampFuncAllocate(SwampDynamicMemory* self, const uint8_t* opcodes, size_t opcodeCount, size_t parametersOctetSize, size_t returnOctetSize)
{
    SwampFunc* func = (SwampFunc*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampFunc));
    func->opcodes = swampAllocateOctets(self, opcodes, opcodeCount);
    func->opcodeCount = opcodeCount;
    func->returnOctetSize = returnOctetSize;
    func->parametersOctetSize = parametersOctetSize;

    return func;
}
