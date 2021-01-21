/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/swamp.h>

SWAMP_FUNCTION_EXPOSE(swamp_core_int_to_fixed)
{
    const swamp_int32 a = swamp_value_int(arguments[0]);

    return swamp_allocator_alloc_integer(allocator, a * SWAMP_FIXED_FACTOR);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_fixed_to_int)
{
    const swamp_int32 a = swamp_value_int(arguments[0]);

    return swamp_allocator_alloc_integer(allocator, a / SWAMP_FIXED_FACTOR);
}
