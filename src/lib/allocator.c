/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/ref_count.h>

#include <string.h>

typedef struct alloc_info {
    size_t totalSize;
} alloc_info;

static void* swamp_calloc(swamp_allocator* self, size_t count, size_t itemSize)
{
    size_t totalSize = count * itemSize;
    size_t totalWithOverhead = sizeof(alloc_info) + totalSize;
    self->allocatedSize += count * itemSize;
    void* alloc = calloc(1, totalWithOverhead);
    alloc_info* info = (alloc_info*) alloc;
    info->totalSize = totalSize;

    return ((uint8_t*) alloc + sizeof(alloc_info));
}

static void swamp_free(swamp_allocator* self, swamp_value* v)
{
    void* alloc = (void*) v;
    alloc_info* info = (alloc_info*) ((uint8_t*) alloc - sizeof(alloc_info));
    self->allocatedSize -= info->totalSize;
    free((void*) info);
}

static void setup_internal(swamp_allocator* self, swamp_internal* internal, swamp_type type)
{
    internal->ref_count = 1;
    internal->erase_code = 0xc0de;
    internal->type = type;
    internal->allocator = self;
}

void swamp_allocator_init(swamp_allocator* self)
{
    setup_internal(self, &self->s_true.internal, swamp_type_boolean);
    self->s_true.truth = 1;
    INC_REF(&self->s_true);
    setup_internal(self, &self->s_false.internal, swamp_type_boolean);
    self->s_false.truth = 0;
    INC_REF(&self->s_false);
    self->allocatedSize = 0;
}

const swamp_value* swamp_allocator_alloc_struct(swamp_allocator* self, size_t field_count)
{
    // TODO: Use P99_FCALLOC
    size_t octet_size = sizeof(swamp_struct) + field_count * sizeof(swamp_value*);
    swamp_struct* mutable = (swamp_struct*) swamp_calloc(self, 1, octet_size);
    setup_internal(self, &mutable->internal, swamp_type_struct);
    mutable->info.octet_count = octet_size;
    mutable->info.field_count = field_count;

    return (const swamp_value*) mutable;
}

const swamp_value* swamp_allocator_alloc_struct_create(swamp_allocator* self, const swamp_value** const items,
                                                       size_t field_count)
{
    const swamp_value* struct_value = swamp_allocator_alloc_struct(self, field_count);
    swamp_struct* s = (swamp_struct*) struct_value;
    for (size_t i = 0; i < field_count; ++i) {
        s->fields[i] = items[i];
        INC_REF(s->fields[i]);
    }

    return struct_value;
}

const swamp_value* swamp_allocator_alloc_curry(swamp_allocator* self, uint16_t typeInfoIndex, const swamp_func* func,
                                               const swamp_value** constant_parameters, size_t constant_parameter_count)
{
    const swamp_value* temp_resources[32];
    for (size_t i = 0; i < func->constant_count; ++i) {
        temp_resources[i] = func->constants[i];
    }
    for (size_t i = 0; i < constant_parameter_count; ++i) {
        temp_resources[i + func->constant_count] = constant_parameters[i];
    }

    size_t new_constant_count = func->constant_count + constant_parameter_count;
    if (func->parameter_count < constant_parameter_count) {
        SWAMP_ERROR("parameter count is less");
    }
    size_t new_parameter_count = func->parameter_count - constant_parameter_count;
    swamp_func* curry_func_value = (swamp_func*) swamp_allocator_alloc_function(
        self, func->opcodes, func->opcode_count, constant_parameter_count, new_parameter_count,
        func->total_register_count_used, temp_resources, new_constant_count, func->debug_name);

    curry_func_value->typeIndex = typeInfoIndex;

    return (const swamp_value*) curry_func_value;
}

const swamp_value* swamp_allocator_alloc_integer(swamp_allocator* self, swamp_int32 v)
{
    swamp_int* t = swamp_calloc(self, 1, sizeof(swamp_int));
    t->value = v;
    setup_internal(self, &t->internal, swamp_type_integer);

    return (const swamp_value*) t;
}

const swamp_value* swamp_allocator_alloc_fixed(swamp_allocator* self, swamp_fixed32 v)
{
    return swamp_allocator_alloc_integer(self, v);
}

const swamp_int* swamp_allocator_alloc_integer_ex(swamp_allocator* self, swamp_int32 v)
{
    return (const swamp_int*) swamp_allocator_alloc_integer(self, v);
}

const swamp_value* swamp_allocator_alloc_string(swamp_allocator* self, const char* str)
{
    swamp_string* t = swamp_calloc(self, 1, sizeof(swamp_string));
    size_t strbuf_size = sizeof(const char) * strlen(str) + 1;
    char* strbuf = swamp_calloc(self, 1, strbuf_size);
    memcpy(strbuf, str, strbuf_size);
    t->characters = strbuf;
    setup_internal(self, &t->internal, swamp_type_string);

    return (const swamp_value*) t;
}

const swamp_value* swamp_allocator_alloc_unmanaged(swamp_allocator* self, const void* something)
{
    swamp_unmanaged* t = swamp_calloc(self, 1, sizeof(swamp_unmanaged));
    t->ptr = something;
    setup_internal(self, &t->internal, swamp_type_unmanaged);
    return (const swamp_value*) t;
}

const swamp_value* swamp_allocator_alloc_blob(swamp_allocator* self, const uint8_t* str, size_t octet_count,
                                              uint32_t hash)
{
    swamp_blob* t = swamp_calloc(self, 1, sizeof(swamp_blob));
    if (octet_count > 0) {
        if (!str) {
            SWAMP_ERROR("can not pass null pointer for blob");
            return 0;
        }
        uint8_t* octets_copy = swamp_calloc(self, 1, octet_count);
        memcpy(octets_copy, str, octet_count);
        t->octets = octets_copy;
    } else {
        if (str) {
            SWAMP_LOG_INFO("small warning: should not pass non null pointer for blob with zero length");
        }
    }

    t->hash = hash;
    t->octet_count = octet_count;
    setup_internal(self, &t->internal, swamp_type_blob);

    return (const swamp_value*) t;
}

const swamp_blob* swamp_allocator_alloc_blob_ex(swamp_allocator* self, const uint8_t* str, size_t octet_count,
                                                uint32_t hash)
{
    return (const swamp_blob*) swamp_allocator_alloc_blob(self, str, octet_count, hash);
}

const swamp_list* swamp_allocator_alloc_list_empty(swamp_allocator* self)
{
    swamp_list* t = swamp_calloc(self, 1, sizeof(swamp_list));
    t->value = 0;
    t->next = 0;
    t->count = 0;
    setup_internal(self, &t->internal, swamp_type_list);

    return t;
}

const swamp_list* swamp_allocator_alloc_list_create(swamp_allocator* self, const swamp_value** constant_parameters,
                                                    size_t constant_parameter_count)
{
    const swamp_list* head = 0;
    if (constant_parameter_count == 0) {
        return swamp_allocator_alloc_list_empty(self);
    }

    for (size_t i = 0; i < constant_parameter_count; ++i) {
        head = swamp_allocator_alloc_list_conj(self, head, constant_parameters[(constant_parameter_count - 1) - i]);
    }

    return head;
}

const swamp_list* swamp_allocator_alloc_list_create_and_transfer(swamp_allocator* self,
                                                                 const swamp_value** constant_parameters,
                                                                 size_t constant_parameter_count)
{
    const swamp_list* head = 0;
    if (constant_parameter_count == 0) {
        return swamp_allocator_alloc_list_empty(self);
    }

    for (size_t i = 0; i < constant_parameter_count; ++i) {
        const swamp_value* item = constant_parameters[(constant_parameter_count - 1) - i];
        head = swamp_allocator_alloc_list_conj(self, head, item);
        DEC_REF(item);
    }

    return head;
}

const swamp_list* swamp_allocator_alloc_list_append(swamp_allocator* self, const swamp_list* a, const swamp_list* b)
{
    if (a->count == 0) {
        INC_REF(b);
        return b;
    }

    if (b->count == 0) {
        INC_REF(a);
        return a;
    }

    const swamp_value** tempA = malloc(sizeof(const swamp_value*) * a->count);
    int index = 0;
    SWAMP_LIST_FOR_LOOP(a)
    tempA[index++] = value;
    SWAMP_LIST_FOR_LOOP_END()

    swamp_list* created_list = swamp_allocator_alloc_list_create(self, tempA, a->count);

    free(tempA);

    swamp_list* last_in_list = created_list;
    while (last_in_list->next != 0) {
        last_in_list = last_in_list->next;
    }

    last_in_list->next = b;
    INC_REF(b);

    created_list->count += b->count;

    return created_list;
}

const swamp_value* swamp_allocator_alloc_enum(swamp_allocator* self, uint8_t enum_type, size_t field_count)
{
    size_t octet_size = sizeof(swamp_enum) + field_count * sizeof(swamp_value*);
    swamp_enum* mutable = (swamp_enum*) swamp_calloc(self, 1, octet_size);
    setup_internal(self, &mutable->internal, swamp_type_enum);
    mutable->enum_type = enum_type;
    mutable->info.octet_count = octet_size;
    mutable->info.field_count = field_count;
    return (const swamp_value*) mutable;
}

const swamp_value* swamp_allocator_alloc_enum_single_value(swamp_allocator* self, uint8_t enum_type,
                                                           const swamp_value* value)
{
    swamp_enum* enum_value = (swamp_enum*) swamp_allocator_alloc_enum(self, enum_type, 1);
    enum_value->fields[0] = value;
    INC_REF(value);

    return (const swamp_value*) enum_value;
}

const swamp_value* swamp_allocator_alloc_enum_no_value(swamp_allocator* self, uint8_t enum_type)
{
    swamp_enum* enum_value = (swamp_enum*) swamp_allocator_alloc_enum(self, enum_type, 0);
    return (const swamp_value*) enum_value;
}

const swamp_value* swamp_allocator_alloc_nothing(swamp_allocator* self)
{
    return swamp_allocator_alloc_enum_no_value(self, 0);
}

const swamp_value* swamp_allocator_alloc_just(swamp_allocator* self, const swamp_value* v)
{
    return swamp_allocator_alloc_enum_single_value(self, 1, v);
}

const swamp_value* swamp_allocator_alloc_just_or_nothing(swamp_allocator* self, const swamp_value* v)
{
    if (v == 0) {
        return swamp_allocator_alloc_nothing(self);
    }

    return swamp_allocator_alloc_just(self, v);
}

const swamp_value* swamp_allocator_alloc_boolean(swamp_allocator* self, int truth)
{
    swamp_value* t;

    if (truth) {
        t = (swamp_value*) &self->s_true;
    } else {
        t = (swamp_value*) &self->s_false;
    }
    INC_REF(t);
    return (const swamp_value*) t;
}

const swamp_boolean* swamp_allocator_alloc_boolean_ex(swamp_allocator* self, int truth)
{
    return (const swamp_boolean*) swamp_allocator_alloc_boolean(self, truth);
}

const swamp_list* swamp_allocator_alloc_list_conj(swamp_allocator* self, const swamp_list* next_list,
                                                  const swamp_value* item)
{
    if (item == 0) {
        SWAMP_ERROR("can not add null item")
    }
#if CONFIGURATION_DEBUG
    if (next_list != 0) {
        SWAMP_ASSERT(swamp_list_validate(next_list), "next_list is broken")
    }
#endif

    swamp_list* t = swamp_calloc(self, 1, sizeof(swamp_list));
    t->value = item;
    INC_REF(item);
    if (next_list && next_list->count > 0) {
        t->next = next_list;
        INC_REF((swamp_value*) next_list);
        t->count = t->next->count + 1;
    } else {
        t->next = 0;
        t->count = 1;
    }
    setup_internal(self, &t->internal, swamp_type_list);
#if CONFIGURATION_DEBUG
    SWAMP_ASSERT(swamp_list_validate(t), "next_list is broken")
#endif
    return t;
}

static char* duplicate_string(swamp_allocator* self, const char* source)
{
    int source_size;
    char* target;
    char* target_ptr;

    source_size = strlen(source);
    target = (char*) swamp_calloc(self, 1, sizeof(char) * source_size + 1);
    if (target == 0) {
        return 0;
    }

    target_ptr = target;
    while (*source) {
        *target_ptr++ = *source++;
    }
    *target_ptr = '\0';

    return target;
}

void swamp_allocator_set_function(swamp_allocator* self, swamp_func* t, const uint8_t* opcodes, size_t opcode_count,
                                  size_t constant_parameters, size_t parameter_count, size_t register_count,
                                  const swamp_value** constants, size_t constant_count, const char* debug_name)
{
    setup_internal(self, &t->internal, swamp_type_function);
    t->opcodes = swamp_calloc(self, 1, opcode_count);
    memcpy((uint8_t*) t->opcodes, opcodes, opcode_count);
    t->opcode_count = opcode_count;

    t->constant_parameter_count = constant_parameters;
    t->parameter_count = parameter_count;
    size_t constant_octet_size = sizeof(swamp_value*) * constant_count;
    t->constants = swamp_calloc(self, 1, constant_octet_size);
    t->constant_count = constant_count;
    memcpy(t->constants, constants, constant_octet_size);
    for (size_t i = 0; i < constant_count; ++i) {
        INC_REF(t->constants[i]);
    }
    t->total_register_count_used = register_count; // Always allocate for return variable
    t->total_register_and_constant_count_used = t->total_register_count_used + constant_count;
    const char* name_copy = duplicate_string(self, debug_name);
    t->debug_name = name_copy;
}

const swamp_value* swamp_allocator_alloc_function(swamp_allocator* self, const uint8_t* opcodes, size_t opcode_count,
                                                  size_t constant_parameters, size_t parameter_count,
                                                  size_t register_count, const swamp_value** constants,
                                                  size_t constant_count, const char* debug_name)
{
    swamp_func* t = swamp_calloc(self, 1, sizeof(swamp_func));
    swamp_allocator_set_function(self, t, opcodes, opcode_count, constant_parameters, parameter_count, register_count,
                                 constants, constant_count, debug_name);
    return (const swamp_value*) t;
}

const swamp_value* swamp_allocator_alloc_external_function(swamp_allocator* self, swamp_external_fn fn,
                                                           size_t parameter_count, const char* debug_name)
{
    swamp_external_func* t = swamp_calloc(self, 1, sizeof(swamp_external_func));
    setup_internal(self, &t->internal, swamp_type_external_function);
    t->fn = fn;
    t->parameter_count = parameter_count;
    const char* name_copy = duplicate_string(self, debug_name);
    t->debug_name = name_copy;

    return (const swamp_value*) t;
}

const swamp_value* swamp_allocator_copy_struct(swamp_allocator* self, const swamp_value* source)
{
    // size_t octet_size = sizeof(swamp_value) + field_count *
    // sizeof(swamp_value*);
    if (source->internal.type != swamp_type_struct) {
        // ERROR
        SWAMP_LOG_INFO("error!");
        return 0;
    }
    const swamp_struct* source_struct = (const swamp_struct*) source;
    size_t field_count = source_struct->info.field_count;
    size_t octet_size_to_copy = field_count * sizeof(swamp_value*);
    const swamp_value* copy = swamp_allocator_alloc_struct(self, field_count);
    swamp_struct* struct_copy = (swamp_struct*) copy;
    void* target_pointer = (swamp_value**) &struct_copy->fields;
    memcpy(target_pointer, &source_struct->fields, octet_size_to_copy);
    return (const swamp_value*) copy;
}

static inline void swamp_release_function(swamp_func* f)
{
    for (size_t i = 0; i < f->constant_count; ++i) {
        DEC_REF(f->constants[i]);
    }
}

void swamp_allocator_free(const swamp_value* v)
{
    // swamp_value_print(v, "destroying");
    switch (v->internal.type) {
        case swamp_type_function:
            swamp_release_function((swamp_func*) v);
            break;
        case swamp_type_blob:
            swamp_free(v->internal.allocator, (void*) (((swamp_blob*) v)->octets));
            break;
        case swamp_type_list:
            DEC_REF_CHECK_NULL(((swamp_list*) v)->next);
            break;
        case swamp_type_struct: {
            const swamp_struct* s = swamp_value_struct(v);
            for (size_t i = 0; i < s->info.field_count; ++i) {
                DEC_REF(s->fields[i]);
            }
            break;
        }
        case swamp_type_unmanaged: {
            swamp_unmanaged* unmanaged = swamp_value_unmanaged(v);
            unmanaged->destroy(unmanaged->ptr);
            break;
        }
        case swamp_type_enum: {
            swamp_enum* enumValue = swamp_value_enum(v);
            for (size_t i = 0; i < enumValue->info.field_count; ++i) {
                DEC_REF(enumValue->fields[i]);
            }
            break;
        }
        case swamp_type_string: {
            const swamp_string* stringValue = swamp_value_string(v);
            swamp_free(v->internal.allocator, stringValue->characters);
            break;
        }

        default:
            v = v;
            break;
    }

    swamp_free(v->internal.allocator, (void*) v);
}