/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <arpa/inet.h>
#include <stdlib.h>
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/deserialize.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/types.h>

static void read_u16(const uint8_t** const p, uint16_t* v)
{
    *v = ntohs(*((uint16_t*) *p));
    *p += 2;
}

static void read_u32(const uint8_t** const p, uint32_t* v)
{
    *v = ntohl(*((uint32_t*) *p));
    *p += 4;
}

const swamp_value* swamp_deserialize(swamp_allocator* const allocator, const uint8_t** const source, size_t max_size)
{
    const uint8_t* p = *source;
    const uint8_t* endp = p + max_size;
    const swamp_value* value = 0;
    swamp_type t = (swamp_type) *p++;
    switch (t) {
        case swamp_type_integer: {
            if (p + 4 >= endp) {
                SWAMP_LOG_SOFT_ERROR("read too far");
                return 0;
            }
            uint32_t v;
            read_u32(&p, &v);
            value = swamp_allocator_alloc_integer(allocator, v);
            break;
        }
        case swamp_type_string: {
            size_t l = *p++;
            if (p + l >= endp) {
                SWAMP_LOG_SOFT_ERROR("read too far");
                return 0;
            }
            value = swamp_allocator_alloc_string(allocator, (const char*) p);
            p += l;
            break;
        }
        case swamp_type_struct: {
            uint16_t count;
            if (p + 2 >= endp) {
                SWAMP_LOG_SOFT_ERROR("read too far");
                return 0;
            }
            read_u16(&p, &count);
            value = swamp_allocator_alloc_struct(allocator, count);
            swamp_struct* sstruct = (swamp_struct*) value;
            for (uint16_t i = 0; i < count; ++i) {
                const swamp_value* f = swamp_deserialize(allocator, &p, endp - p);
                sstruct->fields[i] = f;
            }
            break;
        }
        case swamp_type_enum: {
            uint8_t enum_type = *p++;
            if (p + 2 >= endp) {
                SWAMP_LOG_SOFT_ERROR("read too far");
                return 0;
            }
            uint16_t count;
            read_u16(&p, &count);
            value = swamp_allocator_alloc_enum(allocator, enum_type, count);
            swamp_enum* eenum = (swamp_enum*) value;
            for (size_t i = 0; i < count; ++i) {
                const swamp_value* f = swamp_deserialize(allocator, &p, endp - p);
                eenum->fields[i] = f;
            }
            break;
        }
        case swamp_type_list: {
            if (p + 2 >= endp) {
                SWAMP_LOG_SOFT_ERROR("read too far");
                return 0;
            }
            uint16_t count;
            read_u16(&p, &count);
            const swamp_value** items = (const swamp_value**) malloc(sizeof(swamp_value*) * count);
            for (size_t i = 0; i < count; ++i) {
                const swamp_value* item = swamp_deserialize(allocator, &p, endp - p);
                items[i] = item;
            }
            value = (const swamp_value*) swamp_allocator_alloc_list_create(allocator, items, count);
            free(items);
            break;
        }
        case swamp_type_function:
            SWAMP_LOG_ERROR("functions are not supported");
        case swamp_type_external_function:
            SWAMP_LOG_ERROR("external function not supported");
        case swamp_type_boolean: {
            const int truthy = (*p++) != 0;
            value = swamp_allocator_alloc_boolean(allocator, truthy);
            break;
        }
        case swamp_type_blob:
            SWAMP_LOG_ERROR("blob not supported");
        default:
            SWAMP_LOG_ERROR("not supported");
            return 0;
    }

    swamp_value_print(value, "deserialize()");
    *source = p;

    return value;
}
