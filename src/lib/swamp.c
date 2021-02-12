/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/opcodes.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/ref_count.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/types.h>

#include <string.h> // memset

struct swamp_allocator;

#define MAX_REGISTER_COUNT_IN_CONTEXT (128)

typedef struct swamp_context {
    const swamp_value* registers[MAX_REGISTER_COUNT_IN_CONTEXT];

} swamp_context;

static inline void swamp_registers_clear(const swamp_value** registers, size_t count)
{
    memset(registers, 0, sizeof(swamp_value*) * count);
}

static void swamp_registers_print(const swamp_value** values, size_t count, const char* debug)
{
    SWAMP_LOG_INFO("%s : printing registers:%zu", debug, count);
    for (size_t i = 0; i < count; ++i) {
        const swamp_value* v = values[i];
        if (v != 0) {
            swamp_value_print_index(v, i);
        } else {
            SWAMP_LOG_INFO("  reg %02zx is null", i);
        }
    }
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
    if (v->internal.type == swamp_type_function) {
        swamp_release_function((swamp_func*) v);
    }

    if (v->internal.type == swamp_type_blob) {
        free((void*) (((swamp_blob*) v)->octets));
    }

    free((void*) v);
}
// -------------------------------------------------------------

void swamp_registers_release(const swamp_value** values, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        //		if (values[i]) {
        //			swamp_value_print(values[i], "releasing...");
        //		}
        DEC_REF_CHECK_NULL(values[i]);
    }
    swamp_registers_clear(values, count);
}

static int swamp_value_equal(const swamp_value* a, const swamp_value* b)
{
    if (a->internal.type != b->internal.type) {
        SWAMP_LOG_INFO("ERROR: can not compare of different type");
        return 0;
    }

    switch (a->internal.type) {
        case swamp_type_integer: {
            const swamp_int* int_a = (const swamp_int*) a;
            const swamp_int* int_b = (const swamp_int*) b;
            return int_a->value == int_b->value;
        }
        case swamp_type_string: {
            const swamp_string* str_a = (const swamp_string*) a;
            const swamp_string* str_b = (const swamp_string*) b;
            return strcmp(str_a->characters, str_b->characters) == 0;
        }
        case swamp_type_struct:
            return a == b;
        case swamp_type_list:
            return 0;
        case swamp_type_function:
            return a == b;
        case swamp_type_external_function:
            return a == b;
        case swamp_type_enum:
            return a == b;
        case swamp_type_boolean:
            return a == b;
        case swamp_type_blob:
            return a == b;
        case swamp_type_unmanaged: {
            const swamp_unmanaged* managed_a = (const swamp_unmanaged*) a;
            const swamp_unmanaged* managed_b = (const swamp_unmanaged*) b;
            return managed_a->ptr == managed_b->ptr;
        }
    }
}

static const char* g_swamp_opcode_names[] = {
    "nop",       "crs",     "upd",   "get",  "lr",  "conj", "case",         "brfa",  "jump", "call",
    "ret",       "calle",   "tail",  "add",  "sub", "mul",  "div",          "eql",   "ne",   "less",
    "lessequal", "greater", "gte",   "band", "bor", "bxor", "bnot",         "not",   "bt",   "n/a",
    "n/a",       "n/a",     "curry", "crl",  "lap", "cre",  "stringappend", "fxmul", "fxdiv"};

static const char* swamp_opcode_name(uint8_t opcode)
{
    return g_swamp_opcode_names[opcode];
}
#define SWAMP_CONFIG_DEBUG 1

// -------------------------------------------------------------
// Registers
// -------------------------------------------------------------
#define SET_REGISTER(context, target_register, instance)                                                               \
    DEC_REF_CHECK_NULL(context->registers[target_register]);                                                           \
    if (target_register >= MAX_REGISTER_COUNT_IN_CONTEXT) {                                                            \
        SWAMP_LOG_ERROR("register overrun %d", target_register);                                                       \
    }                                                                                                                  \
    context->registers[target_register] = instance;                                                                    \
    INC_REF(instance);

#define SET_EMPTY_REGISTER(context, target_register, instance)                                                         \
    if (context->registers[target_register] != 0) {                                                                    \
        SWAMP_ERROR("register %d should be null", target_register);                                                    \
    }                                                                                                                  \
    context->registers[target_register] = instance;                                                                    \
    INC_REF(instance);

#define SET_REGISTER_INT(context, target_register, int_value)                                                          \
    SET_REGISTER(context, target_register, swamp_allocator_alloc_integer(allocator, int_value));

#define SET_REGISTER_BOOLEAN(context, target_register, boolean_value)                                                  \
    SET_REGISTER(context, target_register, swamp_allocator_alloc_boolean(allocator, boolean_value));

#define GET_REGISTER(context, register) context->registers[register];

#if SWAMP_CONFIG_DEBUG
    #define GET_REGISTER_INT(context, register) ((const swamp_int*) swamp_value_int(context->registers[register]))
#else
    #define GET_REGISTER_INT(context, register) ((const swamp_int*) context->registers[register])->value
#endif

#define GET_REGISTER_BOOLEAN(context, register) ((const swamp_boolean*) context->registers[register])->truth

// -------------------------------------------------------------
// Fields
// -------------------------------------------------------------
#define GET_FIELD(instance, field_index) ((swamp_struct*) instance)->fields[field_index]

#define SET_FIELD_FORCE_MUTABLE(instance, field_index, value) ((swamp_struct*) instance)->fields[field_index] = value;
#define SET_ENUM_PARAMETER_FORCE_MUTABLE(instance, field_index, value)                                                 \
    ((swamp_enum*) instance)->fields[field_index] = value;

#define INC_REF_FIELD(instance, field_index) INC_REF(GET_FIELD(instance, field_index))

// -------------------------------------------------------------
// Stack
// -------------------------------------------------------------

typedef struct swamp_call_stack_entry {
    const uint8_t* pc;
    swamp_context context;
    const swamp_func* func;
    uint8_t return_register;
} swamp_call_stack_entry;

#define MAX_CALL_STACK_COUNT (24)

typedef struct swamp_call_stack {
    swamp_call_stack_entry entries[MAX_CALL_STACK_COUNT];
    size_t count;
} swamp_call_stack;
/*

static void swamp_call_stack_entry_print(const swamp_call_stack_entry* entry)
{
    size_t relative_pc = entry->pc - entry->func->opcodes;
    SWAMP_LOG_INFO("stack pc:%zu", relative_pc);
}

*/

// -------------------------------------------------------------

#define GET_OPERATOR_INT()                                                                                             \
    uint8_t target_register = *pc++;                                                                                   \
    uint8_t a_register = *pc++;                                                                                        \
    uint8_t b_register = *pc++;                                                                                        \
    int32_t a = GET_REGISTER_INT(context, a_register);                                                                 \
    int32_t b = GET_REGISTER_INT(context, b_register);

#define GET_UNARY_OPERATOR_INT()                                                                                       \
    uint8_t target_register = *pc++;                                                                                   \
    uint8_t a_register = *pc++;                                                                                        \
    int32_t a = GET_REGISTER_INT(context, a_register);

#define SET_OPERATOR_RESULT_INT(int_value) SET_REGISTER_INT(context, target_register, int_value)
#define SET_OPERATOR_RESULT_BOOL(bool_value) SET_REGISTER_BOOLEAN(context, target_register, bool_value)

static void swamp_func_print(const swamp_func* f, const char* debug)
{
    SWAMP_LOG_INFO("func %s const:%zu constparams:%zu param:%zu", debug, f->constant_count, f->constant_parameter_count,
                   f->parameter_count);
}

const swamp_value* swamp_run(swamp_allocator* allocator, const swamp_func* f, const swamp_value** run_parameters,
                             size_t run_parameter_count, swamp_bool verbose_flag)
{
    const uint8_t* pc = f->opcodes;
    swamp_call_stack temp_stack;
    temp_stack.count = 0;
    swamp_call_stack* stack = &temp_stack;
    swamp_call_stack_entry* call_stack_entry = &stack->entries[0];
    if (!SWAMP_VALUE_IS_ALIVE(f)) {
        SWAMP_LOG_ERROR("func is not alive");
        return 0;
    }
    call_stack_entry->func = f;
    INC_REF(f);
    call_stack_entry->pc = pc;
    swamp_context* context = &call_stack_entry->context;
    if (run_parameter_count != f->parameter_count) {
        // ERROR
        SWAMP_LOG_INFO("mismatch! param count %zu vs function says %zu (curry %zu) constant count:%zu (%p, %s)",
                       run_parameter_count, f->parameter_count, f->constant_parameter_count, f->constant_count,
                       (const void*) f, f->debug_name);
        swamp_value_print((const swamp_value*) f, "swamp_run()");
        return 0;
    }

    for (size_t i = 0; i < MAX_CALL_STACK_COUNT; ++i) {
        swamp_registers_clear(stack->entries[i].context.registers, MAX_REGISTER_COUNT_IN_CONTEXT);
    }
    const size_t reserved_return_register_count = 1;

    size_t register_to_set = reserved_return_register_count;
    for (size_t i = 0; i < f->constant_parameter_count; ++i) {
        size_t source_constant_index = i + f->constant_count - f->constant_parameter_count;
        SET_EMPTY_REGISTER(context, register_to_set, f->constants[source_constant_index])
        register_to_set++;
    }

    for (size_t i = 0; i < run_parameter_count; ++i) {
        SET_EMPTY_REGISTER(context, register_to_set, run_parameters[i]);
        register_to_set++;
    }

    if (verbose_flag) {
        SWAMP_LOG_INFO("RUN: total_register_count:%zu\n with constants:%zu opcodecount:%zu",
                       f->total_register_count_used, f->total_register_and_constant_count_used, f->opcode_count);
    }
    for (size_t i = 0; i < f->constant_count - f->constant_parameter_count; ++i) {
        register_to_set = i + f->total_register_count_used + reserved_return_register_count;
        if (i >= f->constant_count) {
            SWAMP_LOG_ERROR("can not reach constant %zu", i);
        }
        SET_REGISTER(context, register_to_set, f->constants[i])
    }

    if (verbose_flag) {
        swamp_registers_print(context->registers, 25, "init");
    }

    while (1) {
#if SWAMP_CONFIG_DEBUG
        if (verbose_flag) {
            uint16_t addr = pc - call_stack_entry->pc;
            SWAMP_LOG_INFO("--- %04X  %s 0x%02x", addr, swamp_opcode_name(*pc), *pc);
            swamp_registers_print(context->registers, 14, "opcode");
        }
#endif
        switch (*pc++) {
            case swamp_opcode_create_struct: {
                uint8_t target_register = *pc++;
                uint8_t field_count = *pc++;
                const swamp_value* instance = swamp_allocator_alloc_struct(allocator, field_count);
                for (uint8_t i = 0; i < field_count; ++i) {
                    uint8_t copy_from_reg = *pc++;
                    SET_FIELD_FORCE_MUTABLE(instance, i, context->registers[copy_from_reg]);
                    INC_REF(context->registers[copy_from_reg])
                }
                SET_REGISTER(context, target_register, instance);
            } break;

            case swamp_opcode_update_struct: {
                uint8_t target_register = *pc++;
                uint8_t source_register = *pc++;
                uint8_t field_count = *pc++;
                const swamp_value* source_instance = GET_REGISTER(context, source_register);
                const swamp_value* instance = swamp_allocator_copy_struct(allocator, source_instance);
                for (uint8_t i = 0; i < field_count; ++i) {
                    uint8_t target_field = *pc++;
                    uint8_t copy_from_register = *pc++;
                    SET_FIELD_FORCE_MUTABLE(instance, target_field, GET_REGISTER(context, copy_from_register))
                }
                for (uint8_t i = 0; i < ((const swamp_struct*) instance)->info.field_count; ++i) {
                    INC_REF_FIELD(instance, i);
                }
                SET_REGISTER(context, target_register, instance);
            } break;

            case swamp_opcode_struct_get: {
                uint8_t target_register = *pc++;
                uint8_t source_register = *pc++;
                uint8_t drill_count = *pc++;
                const swamp_value* source_instance = GET_REGISTER(context, source_register);
                if (source_instance == 0) {
                    SWAMP_ERROR("couldn't lookup")
                }
                for (uint8_t i = 0; i < drill_count; ++i) {
                    uint8_t field_index = *pc++;
#if SWAMP_CONFIG_DEBUG
                if (!swamp_value_is_struct(source_instance)) {
                        SWAMP_LOG_ERROR("can not get, source is not a struct");
                    }
#endif
                    source_instance = GET_FIELD(source_instance, field_index);
#if SWAMP_CONFIG_DEBUG
                    if (source_instance == 0) {
                        SWAMP_ERROR("couldn't lookup")
                    }
#endif
                }
                SET_REGISTER(context, target_register, source_instance);
            } break;

            case swamp_opcode_enum_case: {
                uint8_t source_register = *pc++;
                const swamp_value* source_instance = GET_REGISTER(context, source_register);
#if SWAMP_CONFIG_DEBUG
                if (!source_instance) {
                    SWAMP_LOG_INFO("error nothing at %d", source_register);
                    return 0;
                }

                if (source_instance->internal.type != swamp_type_enum) {
                    // ERROR
                    SWAMP_LOG_INFO("error. Not enum type");
                    return 0;
                }
#endif
                const swamp_enum* enum_info = (swamp_enum*) source_instance;
                uint8_t case_count = *pc++;
                const uint8_t* jump_to_use = 0;
                int found = 0;
                const uint8_t* previous_jump_target_pc = 0;
                uint8_t found_argument_registers[8];
                uint8_t found_argument_count = 0;
                uint8_t found_default = 0;
                for (uint8_t case_index = 0; case_index < case_count; ++case_index) {
                    uint8_t enum_type = *pc++;
                    uint8_t arg_count = *pc++;
                    uint8_t args[8];
                    for (uint8_t arg_index = 0; arg_index < arg_count; ++arg_index) {
                        args[arg_index] = *pc++;
                    }
                    uint8_t rel_jump_case = *pc++;
                    if (!previous_jump_target_pc) {
                        previous_jump_target_pc = pc;
                    }
                    previous_jump_target_pc += rel_jump_case;
                    uint16_t absolute_jump_addr = previous_jump_target_pc - call_stack_entry->pc;

                    if (!found) {
                        if (enum_type == enum_info->enum_type || enum_type == 0xff) {
                            jump_to_use = previous_jump_target_pc;
                            found_argument_count = arg_count;
                            for (uint8_t arg_index = 0; arg_index < arg_count; ++arg_index) {
                                found_argument_registers[arg_index] = args[arg_index];
                            }
                            found = 1;
                            found_default = (enum_type == 0xff);
                        }
                    }
                }
#if SWAMP_CONFIG_DEBUG
                if (!found) {
                    // ERROR
                    SWAMP_LOG_INFO("enum error! No case matching 0x%02X. This should "
                                   "not be "
                                   "possible.",
                                   enum_info->enum_type);
                    return 0;
                }
                if (!found_default && (enum_info->info.field_count != found_argument_count)) {
                    SWAMP_LOG_INFO("Wrong number of arguments provided");
                    return 0;
                }
#endif
                for (uint8_t arg_index = 0; arg_index < found_argument_count; ++arg_index) {
                    const swamp_value* real_value = enum_info->fields[arg_index];
                    SET_REGISTER(context, found_argument_registers[arg_index], real_value);
                }
                pc = jump_to_use;
            } break;

            case swamp_opcode_enum_case_pattern_matching: {
                uint8_t source_register = *pc++;
                const swamp_value* source_instance = GET_REGISTER(context, source_register);
#if SWAMP_CONFIG_DEBUG
                if (!source_instance) {
                    SWAMP_LOG_INFO("error nothing at %d", source_register);
                    return 0;
                }
#endif
                const swamp_value* source_value = source_instance;
                uint8_t case_count = *pc++;
                int found = 0;
                const uint8_t* jump_to_use = 0;
                const uint8_t* previous_jump_target_pc = 0;
                for (uint8_t case_index = 0; case_index < case_count; ++case_index) {
                    uint8_t literal_register = *pc++;
                    const swamp_value* literal_value = 0;

                    uint8_t rel_jump_case = *pc++;

                    if (!previous_jump_target_pc) {
                        previous_jump_target_pc = pc;
                    }
                    previous_jump_target_pc += rel_jump_case;
                    uint16_t absolute_jump_addr = previous_jump_target_pc - call_stack_entry->pc;

                    if (literal_register != 0x00) {
                        literal_value = GET_REGISTER(context, literal_register);
                    }
                    if (literal_register == 0x00 || swamp_value_equal(literal_value, source_value)) {
                        jump_to_use = previous_jump_target_pc;
                        uint8_t rest = case_count - case_index - 1;
                        pc += rest * 2;
                        found = 1;
                        break;
                    }
                }
#if SWAMP_CONFIG_DEBUG
                if (!found) {
                    // ERROR
                    SWAMP_LOG_INFO("enum error! No case matching 0x%02X. This should "
                                   "not be "
                                   "possible.", source_register);
                    return 0;
                }
#endif
                pc = jump_to_use;
            } break;

            case swamp_opcode_reg_to_reg: {
                uint8_t target_register_index = *pc++;
                uint8_t source_register_index = *pc++;
                SET_REGISTER(context, target_register_index, context->registers[source_register_index]);
            } break;

            case swamp_opcode_tail_call: {
                swamp_call_stack_entry* entry = &stack->entries[stack->count];
                const swamp_func* func = entry->func;
                for (uint8_t i = 0; i < func->parameter_count; ++i) {
                    uint8_t copy_from_reg = *pc++;
                    SET_REGISTER(context, i + 1, context->registers[copy_from_reg])
                }
                pc = func->opcodes;
            } break;

            case swamp_opcode_curry: {
                uint8_t target_register = *pc++;
                uint8_t function_from_register = *pc++;
                uint8_t arg_count = *pc++;
                const swamp_value* curry_args[32];

                for (uint8_t i = 0; i < arg_count; ++i) {
                    uint8_t copy_from_reg = *pc++;
                    const swamp_value* arg_value = GET_REGISTER(context, copy_from_reg);
                    curry_args[i] = arg_value;
                }

                const swamp_value* swamp_func_value = GET_REGISTER(context, function_from_register);
                const swamp_func* func = (swamp_func*) swamp_func_value;
                const swamp_value* instance = swamp_allocator_alloc_curry(allocator, func, curry_args, arg_count);
                SET_REGISTER(context, target_register, instance);
            } break;

            case swamp_opcode_call: {
                uint8_t target_register = *pc++;
                uint8_t function_from_register = *pc++;

                const swamp_value* swamp_func_value = GET_REGISTER(context, function_from_register);
                const swamp_func* func = swamp_value_func(swamp_func_value);
                const swamp_context* old_context = context;

                swamp_call_stack_entry* old_entry = &stack->entries[stack->count];
#if SWAMP_CONFIG_DEBUG
                if (stack->count >= 31) {
                    SWAMP_LOG_ERROR("stack push overrun %zu", stack->count);
                }
#endif

                call_stack_entry = &stack->entries[++stack->count];
                call_stack_entry->func = func;
                call_stack_entry->pc = func->opcodes;
                call_stack_entry->return_register = target_register;
                context = &call_stack_entry->context;

#if SWAMP_CONFIG_DEBUG
                uint8_t arg_count = *pc++;
                if (verbose_flag) {
                    SWAMP_LOG_INFO(" **** call '%s' expected parameter_count:%zu actual parameter_count:%zu "
                                   "expected constant_count:%zu",
                                   func->debug_name, func->parameter_count, (size_t) arg_count, func->constant_count);
                }

                if (verbose_flag) {
                    swamp_registers_print(context->registers, 9, "before call");
                    SWAMP_LOG_INFO("call function param count %zu arg count:%d totalregister:%d  called %d %s",
                                   func->parameter_count, (size_t) arg_count, func->total_register_count_used,
                                   func->constant_parameter_count, func->debug_name);
                }

                if (arg_count != func->parameter_count) {
                    swamp_func_print(func, "parameter count mismatch");
                    SWAMP_LOG_INFO("ERROR: arg count mismatch %zu vs called %d+%d %s", func->parameter_count, arg_count,
                                   func->constant_parameter_count, func->debug_name);
                    return 0;
                }

#else
                pc++;
#endif
                const size_t reserved_return_register_count = 1;
                uint8_t source_constant_index;

                for (uint8_t i = 0; i < func->constant_parameter_count; ++i) {
                    target_register = i + reserved_return_register_count;
                    source_constant_index = i + func->constant_count - func->constant_parameter_count;

                    SET_EMPTY_REGISTER(context, target_register, func->constants[source_constant_index]);
                }

                for (uint8_t i = 0; i < func->parameter_count; ++i) {
                    uint8_t copy_from_reg = *pc++;
                    SET_EMPTY_REGISTER(context, i + reserved_return_register_count + func->constant_parameter_count,
                                       old_context->registers[copy_from_reg]);
                }
                for (uint8_t i = 0; i < func->constant_count - func->constant_parameter_count; ++i) {
                    uint8_t target_index = i + func->total_register_count_used + reserved_return_register_count;
                    SET_EMPTY_REGISTER(context, target_index, func->constants[i]);
                }
                old_entry->pc = pc;
                pc = func->opcodes;
            } break;

            case swamp_opcode_return: {
                uint8_t register_to_return = 0;
                const swamp_value* return_value = GET_REGISTER(context, register_to_return);
                INC_REF(return_value);
                size_t total = call_stack_entry->func->total_register_and_constant_count_used +
                               reserved_return_register_count;
#if SWAMP_CONFIG_DEBUG
                if (total > MAX_REGISTER_COUNT_IN_CONTEXT) {
                    SWAMP_ERROR("Illegal total %zu", total);
                }
#endif
                swamp_registers_release(context->registers, total);
                if (stack->count == 0) {
                    return return_value;
                }

                uint8_t return_register = call_stack_entry->return_register;
                call_stack_entry = &stack->entries[--stack->count];
                pc = call_stack_entry->pc;
                context = &call_stack_entry->context;
                SET_REGISTER(context, return_register, return_value);
                DEC_REF(return_value);
            } break;

            case swamp_opcode_call_external: {
                uint8_t destination_register = *pc++;
                uint8_t function_from_register = *pc++;
                uint8_t field_count = *pc++;

                const swamp_value* arguments[8];

                const swamp_value* swamp_func_value = GET_REGISTER(context, function_from_register);
                const swamp_external_func* func = (swamp_external_func*) swamp_func_value;
#if SWAMP_CONFIG_DEBUG
                if (verbose_flag) {
                    SWAMP_LOG_INFO("call external '%s' function_reg:%d destination:%d "
                                   "arg_count:%d",
                                   func->debug_name, function_from_register, destination_register, field_count);
                }
                if (field_count != func->parameter_count) {
                    // ERROR
                    SWAMP_LOG_INFO("mismatch! read %hhu vs function says %zu (%s)", field_count, func->parameter_count,
                                   func->debug_name);
                    return 0;
                }
#endif
                for (uint8_t i = 0; i < field_count; ++i) {
                    uint8_t copy_from_reg = *pc++;
                    arguments[i] = context->registers[copy_from_reg];
                }

                const swamp_value* return_value = func->fn(allocator, arguments, field_count);
                SET_REGISTER(context, destination_register, return_value);
                DEC_REF(return_value);
            } break;
            case swamp_opcode_create_list: {
                uint8_t target_register = *pc++;
                uint8_t field_count = *pc++;
                const swamp_value* temp_values[64];
                for (uint8_t i = 0; i < field_count; ++i) {
                    uint8_t copy_from_reg = *pc++;
                    temp_values[i] = context->registers[copy_from_reg];
                }
                const swamp_value* instance = (const swamp_value*) swamp_allocator_alloc_list_create(
                    allocator, temp_values, field_count);
                SET_REGISTER(context, target_register, instance);
            } break;
            case swamp_opcode_create_enum: {
                uint8_t target_register = *pc++;
                uint8_t enum_value = *pc++;
                uint8_t field_count = *pc++;
                const swamp_value* instance = (const swamp_value*) swamp_allocator_alloc_enum(allocator, enum_value,
                                                                                              field_count);
                for (uint8_t i = 0; i < field_count; ++i) {
                    uint8_t copy_from_reg = *pc++;
                    SET_ENUM_PARAMETER_FORCE_MUTABLE(instance, i, context->registers[copy_from_reg]);
                    INC_REF(context->registers[copy_from_reg])
                }
                SET_REGISTER(context, target_register, instance);
            } break;
            case swamp_opcode_list_append: {
                uint8_t target_register = *pc++;
                uint8_t list_from_register = *pc++;
                uint8_t list_to_append = *pc++;

                const swamp_value* swamp_list_value = GET_REGISTER(context, list_from_register);

#if SWAMP_CONFIG_DEBUG
                if (swamp_list_value->internal.type != swamp_type_list) {
                    SWAMP_LOG_ERROR("error!! append");
                    return 0;
                }
#endif
                const swamp_value* list_to_append_value = GET_REGISTER(context, list_to_append);
                const swamp_list* swamp_new_list_value = swamp_allocator_alloc_list_append(
                    allocator, (const swamp_list*) swamp_list_value, (const swamp_list*) list_to_append_value);
                SET_REGISTER(context, target_register, (const swamp_value*) swamp_new_list_value);
            } break;
            case swamp_opcode_string_append: {
                uint8_t target_register = *pc++;
                uint8_t string_from_register = *pc++;
                uint8_t string_to_append = *pc++;

                const swamp_value* swamp_string_value = GET_REGISTER(context, string_from_register);

#if SWAMP_CONFIG_DEBUG
                if (swamp_string_value->internal.type != swamp_type_string) {
                    SWAMP_LOG_ERROR("error!! append string");

                    return 0;
                }
#endif
                const swamp_value* string_to_append_value = GET_REGISTER(context, string_to_append);
#if SWAMP_CONFIG_DEBUG
                if (string_to_append_value->internal.type != swamp_type_string) {
                    SWAMP_LOG_ERROR("error!! append string2");

                    return 0;
                }
#endif
                static char buf[512];
                strncpy(buf, ((const swamp_string*) swamp_string_value)->characters, 512);
                strncat(buf, ((const swamp_string*) string_to_append_value)->characters, 512);
                const swamp_value* swamp_new_string = swamp_allocator_alloc_string(allocator, buf);
                SET_REGISTER(context, target_register, swamp_new_string);
            } break;
            case swamp_opcode_list_conj: {
                uint8_t target_register = *pc++;
                uint8_t list_from_register = *pc++;
                uint8_t item_register = *pc++;

                const swamp_value* swamp_list_value = GET_REGISTER(context, list_from_register);
#if SWAMP_CONFIG_DEBUG
                if (swamp_list_value->internal.type != swamp_type_list) {
                    // ERROR
                    SWAMP_LOG_INFO("error!! conj");
                    return 0;
                }
#endif
                const swamp_value* item = GET_REGISTER(context, item_register);
#if SWAMP_CONFIG_DEBUG
                SWAMP_ASSERT(swamp_list_validate(swamp_list_value), "source list is broken");
#endif
                const swamp_list* swamp_new_list_value = swamp_allocator_alloc_list_conj(
                    allocator, (const swamp_list*) swamp_list_value, item);
#if SWAMP_CONFIG_DEBUG
                SWAMP_ASSERT(swamp_list_validate(swamp_new_list_value), "new list is broken");
#endif
                SET_REGISTER(context, target_register, (const swamp_value*) swamp_new_list_value);
            } break;

            case swamp_opcode_jump: {
                uint8_t jump = *pc++;
                pc += jump;
            } break;

            case swamp_opcode_branch_false: {
                uint8_t source_register = *pc++;
                uint8_t jump = *pc++;
                const swamp_value* item = GET_REGISTER(context, source_register);
#if SWAMP_CONFIG_DEBUG
                if (item == 0) {
                    swamp_registers_print(context->registers, 8, "branch false");
                    SWAMP_LOG_INFO("error: register %02X is null!", source_register);
                    return 0;
                }
                if (item->internal.type != swamp_type_boolean) {
                    // ERROR
                    SWAMP_LOG_INFO("bool error");
                    return 0;
                }
#endif
                const swamp_boolean* b = (const swamp_boolean*) item;
                if (!b->truth) {
                    pc += jump;
                }
            } break;
            case swamp_opcode_branch_true: {
                uint8_t source_register = *pc++;
                uint8_t jump = *pc++;
                const swamp_value* item = GET_REGISTER(context, source_register);
#if SWAMP_CONFIG_DEBUG
                if (item == 0) {
                    swamp_registers_print(context->registers, 8, "branch false");
                    SWAMP_LOG_INFO("error: register %02X is null!", source_register);
                    return 0;
                }
                if (item->internal.type != swamp_type_boolean) {
                    // ERROR
                    SWAMP_LOG_INFO("bool error");
                    return 0;
                }
#endif
                const swamp_boolean* b = (const swamp_boolean*) item;
                if (b->truth) {
                    pc += jump;
                }
            } break;

            case swamp_opcode_int_add: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a + b);
            } break;
            case swamp_opcode_int_sub: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a - b);
            } break;
            case swamp_opcode_int_div: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a / b);
            } break;
            case swamp_opcode_int_mul: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * b);
            } break;
            case swamp_opcode_fixed_div: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * SWAMP_FIXED_FACTOR / b);
            } break;
            case swamp_opcode_fixed_mul: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * b / SWAMP_FIXED_FACTOR);
            } break;
            case swamp_opcode_int_greater: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a > b);
            } break;
            case swamp_opcode_int_gte: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a >= b);
            } break;
            case swamp_opcode_int_less: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a < b);
            } break;
            case swamp_opcode_int_lessequal: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a <= b);
            } break;
            case swamp_opcode_int_eql: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a == b);
            } break;
            case swamp_opcode_int_neql: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a != b);
            } break;
            case swamp_opcode_int_and: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a & b);
            } break;

            case swamp_opcode_int_or: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a | b);
            } break;
            case swamp_opcode_int_xor: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a ^ b);
            } break;
            case swamp_opcode_int_shl: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a << b);
            } break;
            case swamp_opcode_int_shr: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a >> b);
            } break;
            case swamp_opcode_int_mod: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a % b);
            } break;
            case swamp_opcode_int_not: {
                GET_UNARY_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(~a);
            } break;
            case swamp_opcode_cmp_equal: {
                uint8_t target_register = *pc++;
                uint8_t a_register = *pc++;
                uint8_t b_register = *pc++;
                const swamp_value* a_item = GET_REGISTER(context, a_register);
                const swamp_value* b_item = GET_REGISTER(context, b_register);
                int truth = swamp_value_equal(a_item, b_item);
                SET_REGISTER_BOOLEAN(context, target_register, truth);
            } break;
            case swamp_opcode_cmp_not_equal: {
                uint8_t target_register = *pc++;
                uint8_t a_register = *pc++;
                uint8_t b_register = *pc++;
                const swamp_value* a_item = GET_REGISTER(context, a_register);
                const swamp_value* b_item = GET_REGISTER(context, b_register);
                int truth = swamp_value_equal(a_item, b_item);
                SET_REGISTER_BOOLEAN(context, target_register, !truth);
            } break;
            case swamp_opcode_bool_not: {
                uint8_t target_register = *pc++;
                uint8_t source_register = *pc++;
                int truth = GET_REGISTER_BOOLEAN(context, source_register);
                SET_REGISTER_BOOLEAN(context, target_register, !truth);
            } break;
            default:
                SWAMP_LOG_INFO("Unknown opcode: %02x", *(pc - 1));
                return 0;
        }
    }
}

const swamp_value* swamp_execute(swamp_allocator* allocator, const swamp_func* f, const swamp_value** arguments,
                                 size_t argument_count, int verbose_flag)
{
    return swamp_run(allocator, f, arguments, argument_count, verbose_flag);
}
