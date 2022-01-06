/*---------------------------------------------------------------------------------------------
*  Copyright (c) Peter Bjorklund. All rights reserved.
*  Licensed under the MIT License. See LICENSE in the project root for license information.
*--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>

int swampStringEqual(const SwampString* a, const SwampString* b)
{
    if (a->characterCount != b->characterCount) {
        return 0;
    }

    return tc_memcmp(a->characters, b->characters, a->characterCount) == 0;
}

void swampMemoryPositionAlign(SwampMemoryPosition* position, size_t align)
{
    if (align > 8) {
        CLOG_ERROR("align is wrong");
    }
    size_t rest = *position % align;
    if (rest != 0) {
        *position = *position + (align - rest);
    }
}

SWAMP_INLINE const void* swampListGetItem(const SwampList* list, size_t index)
{
    if (index >= list->count) {
        CLOG_ERROR("index in list doesnt exist");
        return 0;
    }

    return (const uint8_t *)list->value + list->itemSize * index;
}