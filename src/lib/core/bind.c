/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/array.h>
#include <swamp-runtime/core/blob.h>
#include <swamp-runtime/core/debug.h>
#include <swamp-runtime/core/int.h>
#include <swamp-runtime/core/char.h>
#include <swamp-runtime/core/map.h>
#include <swamp-runtime/core/math.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-runtime/core/reduce.h>

#include <string.h> // memset

typedef struct binding_info {
    const char* name;
    swamp_external_fn fn;
} binding_info;

swamp_external_fn swamp_core_find_function(const char* function_name)
{
    binding_info info[] = {
        {"coreListMap", swamp_core_map},
        {"coreListMap2", swamp_core_map2},
        {"coreListIndexedMap", swamp_core_indexed_map},
        {"coreListAny", swamp_core_any},
        {"coreListFind", swamp_core_list_find},
        {"coreListMember", swamp_core_list_member},
        {"coreListFilter", swamp_core_filter},
        {"coreListFilterMap", swamp_core_filter_map},
        {"coreListRemove", swamp_core_remove},
        {"coreListMapcat", swamp_core_mapcat},
        {"coreListHead", swamp_core_first},
        {"coreListConcat", swamp_core_concat},
        {"coreListIsEmpty", swamp_core_empty},
        {"coreListRange", swamp_core_range},
        {"coreListLength", swamp_core_length},
        {"coreListNth", swamp_core_nth},
        {"coreListTail", swamp_core_rest},
        {"coreListReduce", swamp_core_reduce},
        {"coreListFoldl", swamp_core_foldl},
        {"coreListFoldlStop", swamp_core_reduce_stop},

        {"coreArrayFromList", swamp_core_array_from_list},
        {"coreArrayGet", swamp_core_array_get},
        {"coreArrayGrab", swamp_core_array_grab},
        {"coreArrayLength", swamp_core_array_length},
        {"coreArrayIsEmpty", swamp_core_array_is_empty},
        {"coreArrayRepeat", swamp_core_array_repeat},
        {"coreArrayEmpty", swamp_core_array_empty},
        {"coreArrayInitialize", swamp_core_array_initialize},
        {"coreArraySet", swamp_core_array_set},
        {"coreArrayPush", swamp_core_array_push},
        {"coreArrayToList", swamp_core_array_to_list},
        {"coreArraySlice", swamp_core_array_slice},

        {"coreMaybeWithDefault", swamp_core_maybe_with_default},

        {"coreIntToFixed", swamp_core_int_to_fixed},
        {"coreFixedToInt", swamp_core_fixed_to_int},

        {"coreCharOrd", swamp_core_char_to_code},
        {"coreCharToCode", swamp_core_char_to_code},
        {"coreCharFromCode", swamp_core_char_from_code},


        {"coreBlobIsEmpty", swamp_core_blob_is_empty},
        {"coreBlobFromList", swamp_core_blob_from_list},
        {"coreBlobMap", swamp_core_blob_map},
        {"coreBlobIndexedMap", swamp_core_blob_indexed_map},
        {"coreBlobLength", swamp_core_blob_length},
        {"coreBlobGet", swamp_core_blob_get},
        {"coreBlobSet", swamp_core_blob_set},
        {"coreBlobGrab", swamp_core_blob_grab},
        {"coreBlobGet2d", swamp_core_blob_get_2d},
        {"coreBlobGrab2d", swamp_core_blob_grab_2d},
        {"coreBlobSet2d", swamp_core_blob_set_2d},
        {"coreBlobToString2d", swamp_core_blob_to_string_2d},

        {"coreMathRemainderBy", swamp_core_math_remainder_by},
        {"coreMathSin", swamp_core_math_sin},
        {"coreMathCos", swamp_core_math_cos},
        {"coreMathRnd", swamp_core_math_rnd},
        {"coreMathAtan2", swamp_core_math_atan2},
        {"coreMathMid", swamp_core_math_mid},
        {"coreMathAbs", swamp_core_math_abs},
        {"coreMathSign", swamp_core_math_sign},
        {"coreMathClamp", swamp_core_math_clamp},
        {"coreMathLerp", swamp_core_math_lerp},
        {"coreMathMetronome", swamp_core_math_metronome},
        {"coreMathDrunk", swamp_core_math_drunk},
        {"coreMathMod", swamp_core_math_mod},

        {"coreDebugLog", swamp_core_debug_log},
        {"coreDebugToString", swamp_core_debug_to_string},

    };
    for (size_t i = 0; i < sizeof(info) / sizeof(binding_info); ++i) {
        if (strcmp(info[i].name, function_name) == 0) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
