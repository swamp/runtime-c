/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_types_h
#define swamp_types_h

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int swamp_bool;

#define swamp_true (1)
#define swamp_false (0)

#define SWAMP_FIXED_FACTOR (1000)

#define SWAMP_ERROR(...)                                                                                               \
    SWAMP_LOG_INFO(__VA_ARGS__);                                                                                       \
    abort();

#define SWAMP_ASSERT(expr, ...)                                                                                        \
    if (!(expr)) {                                                                                                     \
        SWAMP_LOG_INFO(__VA_ARGS__);                                                                                   \
        abort();                                                                                                       \
    }

/// BASIC INTERNALS
typedef enum swamp_type {
    swamp_type_integer,
    swamp_type_string,
    swamp_type_struct, // swamp_type_array
    swamp_type_list,
    swamp_type_function,
    swamp_type_external_function,
    swamp_type_enum,
    swamp_type_boolean,
    swamp_type_blob,
    swamp_type_unmanaged,
} swamp_type;

typedef struct swamp_internal {
    size_t ref_count;
    uint16_t erase_code;
    swamp_type type;
} swamp_internal;

typedef struct swamp_value {
    swamp_internal internal;
} swamp_value;

/// BASIC INTERNALS

struct swamp_allocator;

// -------------------------------------------------------------
// Define Values
// -------------------------------------------------------------

#define SWAMP_VALUE(name)                                                                                              \
    typedef struct name {                                                                                              \
        swamp_internal internal;

#define SWAMP_VALUE_END(name)                                                                                          \
    }                                                                                                                  \
    name;

#define SWAMP_STRUCT(name)                                                                                             \
    typedef struct name {                                                                                              \
        swamp_internal internal;                                                                                       \
        struct_info info;

#define SWAMP_STRUCT_END(name) SWAMP_VALUE_END(name)

typedef struct struct_info {
    size_t field_count;
    size_t octet_count;
} struct_info;

SWAMP_VALUE(swamp_struct)
struct_info info;
const struct swamp_value* fields[]; // Flexible array member. Need c99 or later.
SWAMP_VALUE_END(swamp_struct)

SWAMP_VALUE(swamp_enum)
uint8_t enum_type;
struct_info info;
const struct swamp_value* fields[]; // Flexible array member. Need c99 or later.
SWAMP_VALUE_END(swamp_enum)

typedef int32_t swamp_int32;
typedef int32_t swamp_fixed32;
typedef uint32_t swamp_resource_name_id;

#define SWAMP_FIXED32_TO_FLOAT(v) (v / (float) SWAMP_FIXED_FACTOR)
#define SWAMP_INT_FIXED_TO_FLOAT(v) SWAMP_FIXED32_TO_FLOAT((v)->value);

SWAMP_VALUE(swamp_int)
swamp_int32 value;
SWAMP_VALUE_END(swamp_int)

typedef swamp_int swamp_int_fixed;

SWAMP_VALUE(swamp_boolean)
int32_t truth;
SWAMP_VALUE_END(swamp_boolean)

SWAMP_VALUE(swamp_string)
const char* characters;
SWAMP_VALUE_END(swamp_string)

SWAMP_VALUE(swamp_blob)
size_t octet_count;
const uint8_t* octets;
uint32_t hash;
SWAMP_VALUE_END(swamp_blob)

SWAMP_VALUE(swamp_unmanaged)
const void* ptr;
SWAMP_VALUE_END(swamp_unmanaged)

SWAMP_VALUE(swamp_list)
const swamp_value* value;
const struct swamp_list* next;
size_t count;
SWAMP_VALUE_END(swamp_list)

#define swamp_list_first(LIST) (((LIST)->value == 0) ? 0 : (LIST))
#define swamp_list_finalize(LIST)                                                                                      \
    if (!(LIST)) {                                                                                                     \
        LIST = swamp_allocator_alloc_list_empty(allocator);                                                            \
    }

#define swamp_list_is_empty(LIST) (((LIST)->value == 0) && ((LIST)->next == 0))

#define SWAMP_VALUE_CAST(T, V) ((const T*) swamp_value_struct(V))

SWAMP_VALUE(swamp_func)
size_t parameter_count;
size_t constant_parameter_count;
const uint8_t* opcodes;
size_t opcode_count;
size_t total_register_count_used;
size_t total_register_and_constant_count_used;

const swamp_value** constants; // or frozen variables in closure
size_t constant_count;
const char* debug_name;
uint16_t typeIndex;
SWAMP_VALUE_END(swamp_func)

typedef const swamp_value* (*swamp_external_fn)(struct swamp_allocator* allocator, const swamp_value** arguments,
                                                int argument_count);

SWAMP_VALUE(swamp_external_func)
size_t parameter_count;
swamp_external_fn fn;
const char* debug_name;
SWAMP_VALUE_END(swamp_external_func)

typedef enum swamp_function_type {
    swamp_function_type_internal,
    swamp_function_type_external,
} swamp_function_type;

typedef struct swamp_function {
    const swamp_func* internal_function;
    const swamp_external_func* external_function;
    swamp_function_type type;
} swamp_function;

swamp_function swamp_value_function(const swamp_value* v);
const swamp_func* swamp_value_func(const swamp_value* v);
swamp_bool swamp_value_is_list(const swamp_value* v);
const swamp_list* swamp_value_list(const swamp_value* v);
swamp_bool swamp_value_bool(const swamp_value* v);
swamp_int32 swamp_value_int(const swamp_value* v);
swamp_resource_name_id swamp_value_resource_name(const swamp_value* v);
swamp_fixed32 swamp_value_fixed(const swamp_value* v);
float swamp_value_fixed_to_float(const swamp_value* v);
swamp_bool swamp_value_is_int(const swamp_value* v);
swamp_bool swamp_value_is_func(const swamp_value* v);
swamp_bool swamp_value_is_nothing(const swamp_value* v);
swamp_bool swamp_value_is_just(const swamp_value* v);
swamp_bool swamp_value_is_struct(const swamp_value* v);
const swamp_struct* swamp_value_struct(const swamp_value* v);
const swamp_enum* swamp_value_enum(const swamp_value* v);
swamp_bool swamp_value_is_blob(const swamp_value* v);
const swamp_blob* swamp_value_blob(const swamp_value* v);
const swamp_string* swamp_value_string(const swamp_value* v);
const swamp_value* swamp_value_any(const swamp_value* v, swamp_int32* typeIndex);
swamp_bool swamp_boolean_truth(const swamp_boolean* v);
swamp_bool swamp_blob_is_empty(const swamp_blob* v);
const swamp_value* swamp_value_just(const swamp_value* v);
swamp_bool swamp_list_validate(const swamp_list* list);

int swamp_values_equal(const swamp_value* a, const swamp_value* b);

#define SWAMP_VALUE_IS_ALIVE(v) ((v)->internal.erase_code == 0xc0de)

#define SWAMP_LIST_FOR_LOOP(LIST)                                                                                      \
    for (const swamp_list* __list = swamp_list_first(LIST); __list; __list = __list->next) {                           \
        const swamp_value* value = __list->value;

#define SWAMP_LIST_FOR_LOOP_END() }

// -------------------------------------------------------------

#endif
