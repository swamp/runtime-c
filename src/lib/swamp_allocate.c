/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/dynamic_memory.h>
#include <tiny-libc/tiny_libc.h>
#include <swamp-runtime/types.h>

const SwampString* swampStringAllocate(struct SwampDynamicMemory* self, const char* s)
{
    SwampString* string = (SwampString*) swampDynamicMemoryAlloc(self, 1, sizeof(SwampString));
    size_t stringLength = tc_strlen(s);
    char* characters = (char*) swampDynamicMemoryAlloc(self, 1, stringLength + 1);
    memcpy(characters, s, stringLength + 1);
    string->characters = characters;
    string->characterCount = stringLength;

    return string;
}
