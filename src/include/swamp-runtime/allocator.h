/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_allocator_h
#define swamp_allocator_h

#include <swamp-runtime/types.h>

typedef struct swamp_allocator {
    swamp_boolean s_true;
    swamp_boolean s_false;
} swamp_allocator;

void swamp_allocator_init(swamp_allocator* self);
const swamp_value* swamp_allocator_alloc_struct(swamp_allocator* self, size_t field_count);
const swamp_value* swamp_allocator_alloc_struct_create(swamp_allocator* self, const swamp_value** registers,
                                                       size_t field_count);

const swamp_value* swamp_allocator_alloc_curry(swamp_allocator* self, uint16_t typeInfoIndex, const swamp_func* func,
                                               const swamp_value** registers, size_t field_count);
const swamp_value* swamp_allocator_alloc_integer(swamp_allocator* self, int32_t v);
const swamp_value* swamp_allocator_alloc_fixed(swamp_allocator* self, swamp_fixed32 v);
const swamp_int* swamp_allocator_alloc_integer_ex(swamp_allocator* self, int32_t v);
const swamp_value* swamp_allocator_alloc_string(swamp_allocator* self, const char* str);
const swamp_value* swamp_allocator_alloc_blob(swamp_allocator* self, const uint8_t* str, size_t octet_count,
                                              uint32_t hash);
const swamp_value* swamp_allocator_alloc_unmanaged(swamp_allocator* self, const void* something);
const swamp_blob* swamp_allocator_alloc_blob_ex(swamp_allocator* self, const uint8_t* str, size_t octet_count,
                                                uint32_t hash);
const swamp_list* swamp_allocator_alloc_list_empty(swamp_allocator* self);
const swamp_list* swamp_allocator_alloc_list_create(swamp_allocator* self, const swamp_value** registers,
                                                    size_t field_count);
const swamp_list* swamp_allocator_alloc_list_create_and_transfer(swamp_allocator* self, const swamp_value** constant_parameters,
    size_t constant_parameter_count);
const swamp_list* swamp_allocator_alloc_list_append(swamp_allocator* self, const swamp_list* target,
                                                    const swamp_list* source);
const swamp_value* swamp_allocator_alloc_enum(swamp_allocator* self, uint8_t enum_type, size_t field_count);
const swamp_value* swamp_allocator_alloc_enum_single_value(swamp_allocator* self, uint8_t enum_type,
                                                           const swamp_value* value);
const swamp_value* swamp_allocator_alloc_enum_no_value(swamp_allocator* self, uint8_t enum_type);
const swamp_value* swamp_allocator_alloc_nothing(swamp_allocator* self);
const swamp_value* swamp_allocator_alloc_just(swamp_allocator* self, const swamp_value* v);
const swamp_value* swamp_allocator_alloc_just_or_nothing(swamp_allocator* self, const swamp_value* v);
const swamp_value* swamp_allocator_alloc_boolean(swamp_allocator* self, int truth);
const swamp_boolean* swamp_allocator_alloc_boolean_ex(swamp_allocator* self, int truth);
const swamp_list* swamp_allocator_alloc_list_conj(swamp_allocator* self, const struct swamp_list* list_value,
                                                  const swamp_value* item);

void swamp_allocator_set_function(swamp_func* t, const uint8_t* opcodes, size_t opcode_count,
                                  size_t constant_parameters, size_t parameter_count, size_t register_count,
                                  const swamp_value** constants, size_t constant_count, const char* debug_name);

const swamp_value* swamp_allocator_alloc_function(swamp_allocator* self, const uint8_t* opcodes, size_t opcode_count,
                                                  size_t constant_parameters, size_t parameter_count,
                                                  size_t register_count, const swamp_value** constants,
                                                  size_t constant_count, const char* debug_name);
const swamp_value* swamp_allocator_alloc_external_function(swamp_allocator* self, swamp_external_fn fn,
                                                           size_t argument_count, const char* debug_name);

const swamp_value* swamp_allocator_copy_struct(swamp_allocator* self, const swamp_value* source);

#endif
