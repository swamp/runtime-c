/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/log.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/types.h>

static void swamp_value_print_ex(FILE* output, const swamp_value* v, size_t tab_count)
{
    for (size_t tab_index = 0; tab_index < tab_count; ++tab_index) {
        SWAMP_LOG_FILE(output, "..");
    }
    switch (v->internal.type) {
        case swamp_type_integer:
            SWAMP_LOG_FILE_NL(output, "int: %d refcount:%zu", ((const swamp_int*) v)->value, v->internal.ref_count);
            break;
        case swamp_type_string: {
            SWAMP_LOG_FILE_NL(output, "string: '%s' refcount:%zu", ((const swamp_string*) v)->characters,
                              v->internal.ref_count);
        } break;
        case swamp_type_struct: {
            const swamp_struct* s = (const swamp_struct*) v;
            SWAMP_LOG_FILE_NL(output, "struct: field_count: %zu refcount:%zu", s->info.field_count,
                              s->internal.ref_count);
            for (uint8_t i = 0; i < s->info.field_count; i++) {
                const swamp_value* field = s->fields[i];
                if (field == 0) {
                    SWAMP_ERROR("bad field value in struct index: %d", i);
                }
                swamp_value_print_ex(output, s->fields[i], tab_count + 1);
            }
        } break;
        case swamp_type_list: {
            const swamp_list* l = (const swamp_list*) v;
            if (!l->value && !l->next) {
                SWAMP_LOG_FILE_NL(output, "emptylist");
                return;
            }
            SWAMP_LOG_FILE_NL(output, "list: refcount:%zu", v->internal.ref_count);
            while (l) {
                swamp_value_print_ex(output, l->value, tab_count + 1);
                l = l->next;
            }
        } break;

        case swamp_type_function: {
            swamp_func* func = (swamp_func*) v;
            SWAMP_LOG_FILE_NL(output, "function '%s' parameter_count:%zu constant:%zu (totalreg:%zu, everything:%zu)",
                              func->debug_name, func->parameter_count, func->constant_count,
                              func->total_register_count_used, func->total_register_and_constant_count_used);
            for (size_t i = 0; i < func->constant_count; ++i) {
                swamp_value_print_ex(output, func->constants[i], tab_count + 1);
            }
        } break;
        case swamp_type_external_function: {
            const swamp_external_func* func = (const swamp_external_func*) v;
            SWAMP_LOG_FILE_NL(output, "external function '%s' parameter_count:%zu", func->debug_name,
                              func->parameter_count);
        } break;

        case swamp_type_enum: {
            const swamp_enum* enum_info = (const swamp_enum*) v;
            SWAMP_LOG_FILE_NL(output, "enum: %d refcount:%zu", enum_info->enum_type, v->internal.ref_count);
            for (uint8_t i = 0; i < enum_info->info.field_count; i++) {
                swamp_value_print_ex(output, enum_info->fields[i], tab_count + 1);
            }
        } break;
        case swamp_type_boolean: {
            const swamp_boolean* boolean = (const swamp_boolean*) v;
            SWAMP_LOG_FILE_NL(output, "bool: %s", boolean->truth ? "True" : "False");
        } break;
        case swamp_type_blob: {
            const swamp_blob* blob = (const swamp_blob*) v;
            SWAMP_LOG_FILE_NL(output, "blob: count: %zu %04X", blob->octet_count, blob->hash);
        } break;
        case swamp_type_unmanaged: {
            const swamp_unmanaged* unmanaged = (const swamp_unmanaged*) v;
            SWAMP_LOG_FILE_NL(output, "unmanaged: %p", unmanaged->ptr);
        } break;
        default:
            SWAMP_ERROR("Unknown type:%d", v->internal.type);
    }
}

void swamp_value_print(const swamp_value* v, const char* prefix)
{
    SWAMP_LOG_INFO("%s: ", prefix);
    swamp_value_print_ex(stderr, v, 0);
}

void swamp_value_print_output(const swamp_value* v)
{
    swamp_value_print_ex(stdout, v, 0);
}

void swamp_value_print_index(const swamp_value* v, int index)
{
    char buf[128];
    sprintf(buf, "  index:%d > ", index);
    swamp_value_print(v, buf);
}
