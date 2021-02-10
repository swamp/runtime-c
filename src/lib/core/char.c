/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/core/char.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/ref_count.h>

// Characters and Integers are the same thing

SWAMP_FUNCTION_EXPOSE(swamp_core_char_to_code)
{
#if CONFIGURATION_DEBUG
    if (!swamp_value_is_int(arguments[0])) {
        SWAMP_LOG_ERROR("character is not an integer");
        return 0;
    }
#endif

    INC_REF(arguments[0])

    return arguments[0];
}

SWAMP_FUNCTION_EXPOSE(swamp_core_char_from_code)
{
#if CONFIGURATION_DEBUG
    if (!swamp_value_is_int(arguments[0])) {
        SWAMP_LOG_ERROR("character is not an integer");
        return 0;
    }
#endif

    INC_REF(arguments[0])

    return arguments[0];
}
