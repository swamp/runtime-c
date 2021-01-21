/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
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

SWAMP_FUNCTION_EXPOSE(swamp_core_debug_to_string)
{
    char buf[256];

    buf[0] = 0;

    const swamp_value* v = arguments[0];

    switch (v->internal.type) {
        case swamp_type_integer:
            snprintf(buf, 256, "%d", ((const swamp_int*) v)->value);
            break;
        case swamp_type_string: {
            snprintf(buf, 256, "%s", ((const swamp_string*) v)->characters);
        } break;
        case swamp_type_struct: {
        } break;
        case swamp_type_list: {
        } break;
        case swamp_type_function: {
        } break;
        case swamp_type_external_function: {
        } break;
        case swamp_type_enum: {
            snprintf(buf, 256, "%d", ((const swamp_enum*) v)->enum_type);
        } break;
        case swamp_type_boolean: {
            const swamp_boolean* boolean = (const swamp_boolean*) v;
            snprintf(buf, 256, "%s", boolean->truth ? "True" : "False");
        } break;
        case swamp_type_unmanaged: {
            const swamp_unmanaged* unmanaged = (const swamp_unmanaged*) v;
            snprintf(buf, 256, "unmanaged %p", unmanaged->ptr);

        } break;
        case swamp_type_blob: {
            const swamp_blob* blob = (const swamp_blob*) v;
            snprintf(buf, 256, "blob %zu", blob->octet_count);
        }
    }

    // printf("debug to string %s (%d)\n", buf, v->internal.type);

    return swamp_allocator_alloc_string(allocator, buf);
}
