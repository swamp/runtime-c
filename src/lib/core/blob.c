/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/core/blob.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/swamp.h>
#include <string.h>

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_is_empty)
{
    const swamp_blob* blob = swamp_value_blob(arguments[0]);

    return swamp_allocator_alloc_boolean(allocator, blob->octet_count == 0);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_from_list)
{
    const swamp_list* intList = swamp_value_list(arguments[0]);

    return swamp_allocator_alloc_blob(allocator, 0, 0, 0);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_map)
{
    const swamp_func* fn = swamp_value_func(arguments[0]);
    const swamp_blob* blob = swamp_value_blob(arguments[1]);

    uint8_t* copy = malloc(blob->octet_count);

    for (size_t i = 0; i < blob->octet_count; ++i) {
        const uint8_t* v = blob->octets[i];
        const swamp_int* intValue = swamp_allocator_alloc_integer_ex(allocator, v);
        const swamp_int* result = swamp_execute(allocator, fn, &intValue, 1, 0);
        copy[i] = (uint8_t) result->value;
    }

    const swamp_blob* blobCopy = swamp_allocator_alloc_blob(allocator, copy, blob->octet_count, 0);

    free(copy);

    return (const swamp_value*) blobCopy;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_indexed_map)
{
    const swamp_func* fn = swamp_value_func(arguments[0]);
    const swamp_blob* blob = swamp_value_blob(arguments[1]);

    uint8_t* copy = malloc(blob->octet_count);

    for (size_t i = 0; i < blob->octet_count; ++i) {
        const uint8_t* v = blob->octets[i];
        const swamp_int* indexValue = swamp_allocator_alloc_integer_ex(allocator, i);
        const swamp_int* intValue = swamp_allocator_alloc_integer_ex(allocator, v);
        const swamp_value* arguments[] = {indexValue, intValue};
        const swamp_int* result = swamp_execute(allocator, fn, arguments, 2, 0);
        copy[i] = (uint8_t) result->value;
    }

    const swamp_blob* blobCopy = swamp_allocator_alloc_blob_ex(allocator, copy, blob->octet_count, 0);

    free(copy);

    return (const swamp_value*) blobCopy;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_length)
{
    const swamp_blob* blob = swamp_value_blob(arguments[0]);

    const swamp_int* intValue = swamp_allocator_alloc_integer_ex(allocator, blob->octet_count);

    return intValue;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_get)
{
    const swamp_int* indexValue = swamp_value_int(arguments[0]);
    const swamp_blob* blob = swamp_value_blob(arguments[1]);
    int index = indexValue->value;
    if (index < 0 || index >= blob->octet_count) {
        return swamp_allocator_alloc_nothing(allocator);
    }

    const swamp_int* intValue = swamp_allocator_alloc_integer_ex(allocator, blob->octets[index]);

    return swamp_allocator_alloc_just(allocator, intValue);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_grab)
{
    const swamp_int* indexValue = swamp_value_int(arguments[0]);
    const swamp_blob* blob = swamp_value_blob(arguments[1]);
    int index = indexValue->value;
    if (index < 0 || index >= blob->octet_count) {
        SWAMP_ERROR("index is out of bounds %d %d", index, blob->octet_count);
        return 0;
    }

    const swamp_int* intValue = swamp_allocator_alloc_integer_ex(allocator, blob->octets[index]);

    return intValue;
}



SWAMP_FUNCTION_EXPOSE(swamp_core_blob_get_2d)
{
    const vec2* position = SWAMP_VALUE_CAST(vec2, arguments[0]);
    const sz2* size = SWAMP_VALUE_CAST(sz2, arguments[1]);
    const swamp_blob* blob = swamp_value_blob(arguments[2]);

    // These checks are not really needed. You can just check the index, but this
    // gives more valuable information about what went wrong.
    if (position->x->value < 0 || position->x->value >= size->width->value) {
        // SWAMP_LOG_SOFT_ERROR("position X is out of bounds %d %d", position->x->value, size->width->value);
        return swamp_allocator_alloc_nothing(allocator);
    }

    if (position->y->value < 0 || position->y->value >= size->height->value) {
        // SWAMP_LOG_SOFT_ERROR("position Y is out of bounds %d %d", position->y->value, size->height->value);
        return swamp_allocator_alloc_nothing(allocator);
    }

    int index = position->y->value * size->width->value + position->x->value;

    if (index < 0 || index >= blob->octet_count) {
        // SWAMP_LOG_SOFT_ERROR("position is out of octet count bounds %d %d", index, blob->octet_count);
        return swamp_allocator_alloc_nothing(allocator);
    }

    const swamp_int* intValue = swamp_allocator_alloc_integer_ex(allocator, blob->octets[index]);

    return swamp_allocator_alloc_just(allocator, intValue);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_grab_2d)
{
    const vec2* position = SWAMP_VALUE_CAST(vec2, arguments[0]);
    const sz2* size = SWAMP_VALUE_CAST(sz2, arguments[1]);
    const swamp_blob* blob = swamp_value_blob(arguments[2]);

    // These checks are not really needed. You can just check the index, but this
    // gives more valuable information about what went wrong.
    if (position->x->value < 0 || position->x->value > size->width->value) {
        SWAMP_LOG_ERROR("position X is out of bounds %d %d", position->x->value, size->width->value);
        return 0;
    }

    if (position->y->value < 0 || position->y->value > size->height->value) {
        SWAMP_LOG_ERROR("position Y is out of bounds %d %d", position->y->value, size->height->value);
        return 0;
    }

    int index = position->y->value * size->width->value + position->x->value;

    if (index < 0 || index >= blob->octet_count) {
        SWAMP_LOG_ERROR("position is out of octet count bounds %d %d", index, blob->octet_count);
        return 0;
    }

    const swamp_int* intValue = swamp_allocator_alloc_integer_ex(allocator, blob->octets[index]);

    return intValue;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_set)
{
    const swamp_int* indexValue = swamp_value_int(arguments[0]);
    const swamp_int32 newValue = swamp_value_int(arguments[1]);
    const swamp_blob* blob = swamp_value_blob(arguments[2]);
    int index = indexValue->value;
    if (index < 0 || index >= blob->octet_count) {
        return blob;
    }
    uint8_t* copy = malloc(blob->octet_count);
    memcpy(copy, blob->octets, blob->octet_count);
    copy[index] = newValue;

    const swamp_blob* blobCopy = swamp_allocator_alloc_blob_ex(allocator, copy, blob->octet_count, 0);

    free(copy);

    return blobCopy;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_set_2d)
{
    const vec2* position = SWAMP_VALUE_CAST(vec2, arguments[0]);
    const sz2* size = SWAMP_VALUE_CAST(sz2, arguments[1]);
    swamp_int32 newValue = swamp_value_int(arguments[2]);
    const swamp_blob* blob = swamp_value_blob(arguments[3]);

    // These checks are not really needed. You can just check the index, but this
    // gives more valuable information about what went wrong.
    if (position->x->value < 0 || position->x->value >= size->width->value) {
        SWAMP_LOG_SOFT_ERROR("position X is out of bounds %d %d", position->x->value, size->width->value);
        return blob;
    }

    if (position->y->value < 0 || position->y->value >= size->height->value) {
        SWAMP_LOG_SOFT_ERROR("position Y is out of bounds %d %d", position->y->value, size->height->value);
        return blob;
    }

    int index = position->y->value * size->width->value + position->x->value;

    if (index < 0 || index >= blob->octet_count) {
        SWAMP_LOG_SOFT_ERROR("position is out of octet count bounds %d %d", index, blob->octet_count);
        return blob;
    }

    uint8_t* copy = malloc(blob->octet_count);
    memcpy(copy, blob->octets, blob->octet_count);
    copy[index] = newValue;

    const swamp_blob* blobCopy = swamp_allocator_alloc_blob_ex(allocator, copy, blob->octet_count, 0);

    free(copy);

    return blobCopy;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_to_string_2d)
{
    const sz2* size = SWAMP_VALUE_CAST(sz2, arguments[0]);
    const swamp_blob* blob = swamp_value_blob(arguments[1]);

    int area = size->height->value * size->width->value;
    if (area < 0 || area > blob->octet_count) {
        return swamp_allocator_alloc_string(allocator, "");
    }
    char* result = malloc(size->height->value * (size->width->value + 1) + 2);

    int index = 0;
    for (size_t y = 0; y < size->height->value; ++y) {
        result[index++] = '\n';
        for (size_t x = 0; x < size->width->value; ++x) {
            uint8_t ch = blob->octets[size->width->value * y + x];
            result[index++] = ch;
        }
    }
    result[index++] = '\n';
    result[index] = 0;

    const swamp_int* stringValue = swamp_allocator_alloc_string(allocator, result);

    free(result);

    return stringValue;
}
