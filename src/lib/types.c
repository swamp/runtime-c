/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/log.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/types.h>

swamp_function swamp_value_function(const swamp_value* v)
{
    swamp_function result;

    if (!SWAMP_VALUE_IS_ALIVE(v)) {
        SWAMP_ERROR("function is dead");
    }
    if (v->internal.type == swamp_type_external_function) {
        result.external_function = (swamp_external_func*) v;
        result.type = swamp_function_type_external;
        return result;
    } else if (v->internal.type == swamp_type_function) {
        result.internal_function = (swamp_func*) v;
        result.type = swamp_function_type_internal;
        return result;
    }
    swamp_value_print(v, "should be function");
    SWAMP_ERROR("Illegal function");
}

const swamp_func* swamp_value_func(const swamp_value* v)
{
    if (v->internal.type == swamp_type_function) {
        return (swamp_func*) v;
    }
    if (!SWAMP_VALUE_IS_ALIVE(v)) {
        SWAMP_ERROR("func is dead");
    }
    swamp_value_print(v, "should be func");
    SWAMP_ERROR("Illegal func");
}

swamp_bool swamp_value_is_func(const swamp_value* v)
{
    return v->internal.type == swamp_type_function;
}

const swamp_list* swamp_value_list(const swamp_value* v)
{
    if (v->internal.type == swamp_type_list) {
        return (const swamp_list*) v;
    }
    SWAMP_ERROR("expected list");
}

swamp_bool swamp_value_is_list(const swamp_value* v)
{
    return v->internal.type == swamp_type_list;
}

swamp_bool swamp_value_bool(const swamp_value* v)
{
    if (v->internal.type == swamp_type_boolean) {
        return ((const swamp_boolean*) v)->truth != 0;
    }
    SWAMP_ERROR("expected bool");
}

swamp_bool swamp_value_is_int(const swamp_value* v)
{
    return v->internal.type == swamp_type_integer;
}

swamp_int32 swamp_value_int(const swamp_value* v)
{
    if (v->internal.type == swamp_type_integer) {
        return ((const swamp_int*) v)->value;
    }
    SWAMP_ERROR("expected int");
}

swamp_int32 swamp_value_fixed(const swamp_value* v)
{
    if (v->internal.type == swamp_type_integer) {
        return ((const swamp_int*) v)->value;
    }
    SWAMP_ERROR("expected int");
}

float swamp_value_fixed_to_float(const swamp_value* v)
{
    if (v->internal.type == swamp_type_integer) {
        return SWAMP_INT_FIXED_TO_FLOAT((const swamp_int_fixed*) v);
    }
    SWAMP_ERROR("expected int");
}

swamp_bool swamp_value_is_struct(const swamp_value* v)
{
    return v->internal.type == swamp_type_struct;
}

const swamp_struct* swamp_value_struct(const swamp_value* v)
{
    if (v->internal.type == swamp_type_struct) {
        return ((const swamp_struct*) v);
    }
    SWAMP_ERROR("expected struct");
}

const swamp_enum* swamp_value_enum(const swamp_value* v)
{
    if (v->internal.type == swamp_type_enum) {
        return ((const swamp_enum*) v);
    }
    SWAMP_ERROR("expected enum");
}

swamp_bool swamp_value_is_blob(const swamp_value* v)
{
    return v->internal.type == swamp_type_blob;
}

const swamp_blob* swamp_value_blob(const swamp_value* v)
{
    if (v->internal.type == swamp_type_blob) {
        return ((const swamp_blob*) v);
    }
    swamp_value_print(v, "expected blob");
    SWAMP_ERROR("expected blob but was %d", v->internal.type);
}

swamp_bool swamp_blob_is_empty(const swamp_blob* v)
{
    return v->octet_count == 0;
}

const swamp_string* swamp_value_string(const swamp_value* v)
{
    if (v->internal.type == swamp_type_string) {
        return ((const swamp_string*) v);
    }
    SWAMP_ERROR("expected string");
}

swamp_bool swamp_value_is_nothing(const swamp_value* v)
{
    if (v->internal.type == swamp_type_enum) {
        uint8_t enumValue = ((const swamp_enum*) v)->enum_type;
        if (enumValue != 0 && enumValue != 1) {
            SWAMP_ERROR("illegal Just or Nothing value %d", enumValue)
        }
        return enumValue == 0;
    }

    SWAMP_ERROR("expected Just or Nothing is_nothing");
}

swamp_bool swamp_value_is_just(const swamp_value* v)
{
    if (v->internal.type == swamp_type_enum) {
        uint8_t enumValue = ((const swamp_enum*) v)->enum_type;
        if (enumValue != 0 && enumValue != 1) {
            SWAMP_ERROR("illegal Just or Nothing value %d", enumValue)
        }
        return enumValue == 1;
    }

    SWAMP_ERROR("expected Just or Nothing is_just");
}

const swamp_value* swamp_value_just(const swamp_value* v)
{
    if (!swamp_value_is_just(v)) {
        SWAMP_ERROR("expected Just");
    }

    return ((const swamp_enum*) v)->fields[0];
}

int swamp_values_equal(const swamp_value* a, const swamp_value* b)
{
    if (a->internal.type != b->internal.type) {
        return 0;
    }

    switch (a->internal.type) {
        case swamp_type_integer: {
            const swamp_int* av = (const swamp_int*) a;
            const swamp_int* bv = (const swamp_int*) b;
            return av->value == bv->value;
        }
        case swamp_type_string:
        case swamp_type_struct:
        case swamp_type_list:
        case swamp_type_function:
        case swamp_type_external_function:
        case swamp_type_enum:
        case swamp_type_boolean:
        case swamp_type_blob:
        case swamp_type_unmanaged:
        default:
            return 0;
    }
}
