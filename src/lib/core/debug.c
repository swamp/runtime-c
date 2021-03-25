/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <string.h>
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/ref_count.h>
#include <swamp-runtime/swamp.h>

SWAMP_FUNCTION_EXPOSE(swamp_core_debug_log)
{
    const swamp_string* output = swamp_value_string(arguments[0]);

    printf("log: %s\n", output->characters);
    fflush(stdout);

    INC_REF(arguments[0]);
    return (const swamp_value*) output;
}

static int toString(const swamp_value* v, char* buf, size_t maxCount)
{
    switch (v->internal.type) {
        case swamp_type_integer:
            snprintf(buf, maxCount, "%d", ((const swamp_int*) v)->value);
            break;
        case swamp_type_string: {
            snprintf(buf, maxCount, "%s", ((const swamp_string*) v)->characters);
        } break;
        case swamp_type_struct: {
            strcpy(buf, "{");
            const swamp_struct* s = swamp_value_struct(v);
            char temp[2048];
            for (size_t i=0; i<s->info.field_count; ++i) {
                if (i > 0) {
                    strcat(buf, ", ");
                }
                toString(s->fields[i], temp, 2048);
                strcat(buf, temp);
            }
            strcat(buf, "}");
        } break;
        case swamp_type_list: {
            char temp[2048];
            const swamp_list* list = swamp_value_list(v);
            strcpy(buf, "[");
            int index = 0;
            SWAMP_LIST_FOR_LOOP(list)
                if (index > 0) {
                    strcat(buf, ", ");
                }
                toString(value, temp, 2048);
                strncat(buf, temp, 2048);
                index++;
            SWAMP_LIST_FOR_LOOP_END()
            strcat(buf, "]");
        } break;
        case swamp_type_function: {
        } break;
        case swamp_type_external_function: {
        } break;
        case swamp_type_enum: {
            snprintf(buf, maxCount, "%d", ((const swamp_enum*) v)->enum_type);
        } break;
        case swamp_type_boolean: {
            const swamp_boolean* boolean = (const swamp_boolean*) v;
            snprintf(buf, maxCount, "%s", boolean->truth ? "True" : "False");
        } break;
        case swamp_type_unmanaged: {
            const swamp_unmanaged* unmanaged = (const swamp_unmanaged*) v;
            if (unmanaged->to_string != 0) {
                unmanaged->to_string(unmanaged, 0, buf, maxCount);
            } else {
                snprintf(buf, maxCount, "unmanaged %p", unmanaged->ptr);
            }
        } break;
        case swamp_type_blob: {
            const swamp_blob* blob = (const swamp_blob*) v;
            snprintf(buf, maxCount, "blob %zu", blob->octet_count);
        }
    }
}

SWAMP_FUNCTION_EXPOSE(swamp_core_debug_to_string)
{
    char buf[256];

    buf[0] = 0;

    const swamp_value* v = arguments[0];

    toString(v, buf, 256);

    // printf("debug to string %s (%d)\n", buf, v->internal.type);

    return swamp_allocator_alloc_string(allocator, buf);
}
