/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/bind.h>

#include <tiny-libc/tiny_libc.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/maybe.h>

// withDefault : a -> Maybe a -> a
void swampCoreMaybeWithDefault(void* result, SwampMachineContext* context, const SwampUnknownType* defaultValue, const SwampMaybe** maybe)
{
    if (swampMaybeIsNothing(*maybe)) {
        tc_memcpy_octets(result, defaultValue->ptr, defaultValue->size);
    } else {
        tc_memcpy_octets(result, swampMaybeJustGetValue(*maybe, defaultValue->align), defaultValue->size);
    }
}

void* swampCoreMaybeFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Maybe.withDefault", swampCoreMaybeWithDefault},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
