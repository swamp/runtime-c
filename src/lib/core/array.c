/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/array.h>

#include <swamp-runtime/allocator.h>
#include <swamp-runtime/core/execute.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/ref_count.h>
#include <swamp-runtime/swamp.h>

SWAMP_FUNCTION_EXPOSE(swamp_core_array_from_list)
{
    const swamp_list* list = swamp_value_list(arguments[0]);
    swamp_struct* array = (swamp_struct*) swamp_allocator_alloc_struct(allocator, list->count);
    int i = 0;
    SWAMP_LIST_FOR_LOOP(list)
    array->fields[i] = value;
    i++;
    SWAMP_LIST_FOR_LOOP_END()
    return (const swamp_value*) array;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_get)
{
    const swamp_int32 index = swamp_value_int(arguments[0]);
    const swamp_struct* array = swamp_value_struct(arguments[1]);
    if (index < array->info.field_count && index >= 0) {
        const swamp_value* result = array->fields[index];
        return swamp_allocator_alloc_just(allocator, result);
    }
    return swamp_allocator_alloc_nothing(allocator);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_grab)
{
    const swamp_int32 index = swamp_value_int(arguments[0]);
    const swamp_struct* array = swamp_value_struct(arguments[1]);
    if (index < array->info.field_count && index >= 0) {
        const swamp_value* result = array->fields[index];
        INC_REF(result);
        return result;
    }

    SWAMP_LOG_SOFT_ERROR("array grab out of index");

    return 0;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_length)
{
    const swamp_struct* array = swamp_value_struct(arguments[0]);
    return swamp_allocator_alloc_integer(allocator, array->info.field_count);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_is_empty)
{
    const swamp_struct* array = swamp_value_struct(arguments[0]);
    return swamp_allocator_alloc_boolean(allocator, array->info.field_count == 0);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_repeat)
{
    const swamp_int32 count = swamp_value_int(arguments[0]);
    const swamp_value* item = arguments[1];
    swamp_struct* array = (swamp_struct*) swamp_allocator_alloc_struct(allocator, count);
    for (int32_t i = 0; i < count; i++) {
        array->fields[i] = item;
        INC_REF(item);
    }

    return (const swamp_value*) array;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_empty)
{
    const swamp_value* array = swamp_allocator_alloc_struct(allocator, 0);
    return array;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_initialize)
{
    const swamp_int32 count = swamp_value_int(arguments[0]);
    const swamp_function fn = swamp_value_function(arguments[1]);

    swamp_struct* array = (swamp_struct*) swamp_allocator_alloc_struct(allocator, count);
    for (int32_t i = 0; i < count; ++i) {
        const swamp_value* index = swamp_allocator_alloc_integer(allocator, i);
        const swamp_value* result = swamp_execute_1(allocator, &fn, index);
        DEC_REF(index);
        INC_REF(result)
        array->fields[i] = result;
    }

    return (const swamp_value*) array;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_set)
{
    const swamp_int32 index = swamp_value_int(arguments[0]);
    const swamp_value* item = arguments[1];
    const swamp_struct* array = swamp_value_struct(arguments[2]);
    if (index >= array->info.field_count || index < 0) {
        return (const swamp_value*) array;
    }

    swamp_struct* duplicate = (swamp_struct*) swamp_allocator_alloc_struct_create(
        allocator, (const swamp_value**) array->fields, array->info.field_count);
    duplicate->fields[index] = item;
    INC_REF(item);
    return (const swamp_value*) duplicate;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_push)
{
    const swamp_value* item = arguments[0];
    const swamp_struct* array = swamp_value_struct(arguments[1]);

    swamp_struct* duplicate = (swamp_struct*) swamp_allocator_alloc_struct_create(
        allocator, (const swamp_value**) array->fields, array->info.field_count + 1);
    duplicate->fields[array->info.field_count] = item;
    INC_REF(item);
    return (const swamp_value*) duplicate;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_to_list)
{
    const swamp_struct* array = swamp_value_struct(arguments[0]);
    const swamp_value* list = (const swamp_value*) swamp_allocator_alloc_list_create(
        allocator, (const swamp_value**) array->fields, array->info.field_count);

    return list;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_array_slice)
{
    swamp_int32 startIndex = swamp_value_int(arguments[0]);
    swamp_int32 endIndex = swamp_value_int(arguments[1]);
    const swamp_struct* array = swamp_value_struct(arguments[2]);

    int arrayCount = array->info.field_count;
    if (startIndex < 0) {
        startIndex += arrayCount;
    }

    if (endIndex < 0) {
        endIndex += arrayCount;
    }

    if (startIndex > arrayCount) {
        startIndex = arrayCount;
    }

    if (endIndex > arrayCount) {
        endIndex = arrayCount;
    }

    swamp_int32 count = endIndex - startIndex;
    if (count < 0) {
        count = 0;
    }

    const swamp_value* resultArray = (const swamp_value*) swamp_allocator_alloc_struct_create(
        allocator, (const swamp_value**) &array->fields[startIndex], count);

    return resultArray;
}
