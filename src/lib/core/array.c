/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/panic.h>
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-runtime/core/array.h>
#include <swamp-runtime/core/blob.h>
#include <swamp-runtime/core/int.h>
#include <swamp-runtime/core/char.h>
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>

// fromList : List a -> Array a
static void swampCoreArrayFromList(SwampArray** result, SwampMachineContext* context, const SwampList** listValue)
{
    *result = (SwampArray*)(*listValue);
}

// toList : Array a -> List a
static void swampCoreArrayToList(SwampList** result, SwampMachineContext* context, const SwampArray** arrayValue)
{
    *result = (SwampList*)(*arrayValue);
}

// length : Array a -> Int
static void swampCoreArrayLength(SwampInt32* result, SwampMachineContext* context, const SwampArray** array)
{
    *result = (*array)->count;
}

// repeat : Int -> a -> Array a
static void swampCoreArrayRepeat(SwampMaybe * result, SwampMachineContext* context, const SwampInt32* index, const SwampArray** _array)
{
    const SwampArray* array = *_array;

    if (*index < 0 || *index >= (SwampInt32) array->count) {
        swampMaybeNothing(result);
        return;
    }

    const void* ptr = ((const uint8_t*)array->value) + array->itemSize * *index;

    swampMaybeJust(result, array->itemAlign, ptr, array->itemSize);
}

// get : Int -> Array a -> Maybe a
static void swampCoreArrayGet(SwampMaybe * result, SwampMachineContext* context, const SwampInt32* index, const SwampArray** _array)
{
    const SwampArray* array = *_array;

    if (*index < 0 || *index >= (SwampInt32) array->count) {
        swampMaybeNothing(result);
        return;
    }

    const void* ptr = ((const uint8_t*)array->value) + array->itemSize * *index;

    swampMaybeJust(result, array->itemAlign, ptr, array->itemSize);
}

// grab : Int -> Array a -> a
static void swampCoreArrayGrab(void* result, SwampMachineContext* context, const SwampInt32* index, const SwampArray** _array)
{
    const SwampArray* array = *_array;

    if (array->count > 2048) {
        swampPanic(context, "suspicious array count %zu", array->count );
        return;
    }

    if (*index < 0 || *index >= (SwampInt32) array->count) {
        swampPanic(context, "illegal array index %d", *index);
        return;
    }

    const void* ptr = ((const uint8_t*)array->value) + array->itemSize * *index;

    tc_memcpy_octets(result, ptr, array->itemSize);
}

// set : Int -> a -> Array a -> Array a
static void swampCoreArraySet(void* result, SwampMachineContext* context, const SwampInt32* index, const SwampArray** _array)
{
    const SwampArray* array = *_array;

    if (*index < 0 || *index >= (SwampInt32) array->count) {
        swampPanic(context, "array set: illegal array index");
        return;
    }

    //const void* ptr = ((const uint8_t*)array->value) + array->itemSize * *index;
}

// slice : Int -> Int -> Array a -> Array a
static void swampCoreArraySlice(void* result, SwampMachineContext* context, const SwampInt32* index, const SwampArray** _array)
{
    const SwampArray* array = *_array;

    if (*index < 0 || *index >= (SwampInt32) array->count) {
        swampPanic(context, "array slice: illegal array index");
        return;
    }

    //const void* ptr = ((const uint8_t*)array->value) + array->itemSize * *index;
}

const void* swampCoreArrayFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Array.fromList", SWAMP_C_FN(swampCoreArrayFromList)},
        {"Array.toList", SWAMP_C_FN(swampCoreArrayToList)},
        {"Array.length", SWAMP_C_FN(swampCoreArrayLength)},
        {"Array.get", SWAMP_C_FN(swampCoreArrayGet)},
        {"Array.grab", SWAMP_C_FN(swampCoreArrayGrab)},
        {"Array.set", SWAMP_C_FN(swampCoreArraySet)},
        {"Array.slice", SWAMP_C_FN(swampCoreArraySlice)},
        {"Array.repeat", SWAMP_C_FN(swampCoreArrayRepeat)},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}

