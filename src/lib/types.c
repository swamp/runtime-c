/*---------------------------------------------------------------------------------------------
*  Copyright (c) Peter Bjorklund. All rights reserved.
*  Licensed under the MIT License. See LICENSE in the project root for license information.
*--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>

int swampStringEqual(const SwampString* a, const SwampString* b)
{
    if (a->characterCount != b->characterCount) {
        return 0;
    }

    return tc_memcmp(a->characters, b->characters, a->characterCount) == 0;
}
