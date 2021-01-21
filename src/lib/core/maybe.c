/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/maybe.h>

#include <swamp-runtime/allocator.h>

#include <swamp-runtime/ref_count.h>
#include <swamp-runtime/swamp.h>

SWAMP_FUNCTION_EXPOSE(swamp_core_maybe_with_default)
{
    if (swamp_value_is_just(arguments[1])) {
        return swamp_value_just(arguments[1]);
    }

    INC_REF(arguments[0]);
    return arguments[0];
}
