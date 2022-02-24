/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/int.h>

#include <tiny-libc/tiny_libc.h>
#include <swamp-runtime/context.h>

static void swampCoreIntRound(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* fixed)
{
    *result = *fixed / SWAMP_FIXED_FACTOR;
}

static void swampCoreIntToFixed(SwampFixed32* result, SwampMachineContext* context, const SwampInt32 * intValue)
{
    *result = *intValue * SWAMP_FIXED_FACTOR;
}

const void* swampCoreIntFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Int.round", SWAMP_C_FN(swampCoreIntRound)},
        {"Int.toFixed", SWAMP_C_FN(swampCoreIntToFixed)},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
