/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/serialize.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/types.h>

static void write_u16(uint8_t** p, uint16_t v)
{
    *((uint16_t*) *p) = htons(v);
    *p += 2;
}

static void write_u32(uint8_t** p, uint32_t v)
{
    *((uint32_t*) *p) = htonl(v);
    *p += 4;
}

int swamp_serialize(const swamp_value* v, uint8_t** target, size_t max_size)
{
    uint8_t* p = *target;
    const uint8_t* endp = p + max_size;
    *p++ = v->internal.type;
    switch (v->internal.type) {
        case swamp_type_integer: {
            const swamp_int* i = (swamp_int*) v;
            SWAMP_LOG_INFO("integer %d", i->value);
            if (p + 4 >= endp) {
                SWAMP_LOG_SOFT_ERROR("wrote too long");
                return -1;
            }
            write_u32(&p, i->value);
            break;
        }
        case swamp_type_string: {
            const swamp_string* s = (swamp_string*) v;
            SWAMP_LOG_INFO("string '%s'", s->characters);
            size_t l = strlen(s->characters);
            *p++ = l;
            if (p + l >= endp) {
                SWAMP_LOG_SOFT_ERROR("wrote too long");
                return -1;
            }
            memcpy(p, s->characters, l);
            p += l;
            break;
        }
        case swamp_type_struct: {
            const swamp_struct* s = (swamp_struct*) v;
            SWAMP_LOG_INFO("struct count:%zu", s->info.field_count);
            size_t count = s->info.field_count;
            if (p + 2 >= endp) {
                SWAMP_LOG_SOFT_ERROR("wrote too long");
                return -1;
            }
            write_u16(&p, count); // arrays are implemented as struct
            for (size_t i = 0; i < count; ++i) {
                const swamp_value* f = s->fields[i];
                swamp_serialize(f, &p, endp - p);
            }
            break;
        }
        case swamp_type_enum: {
            const swamp_enum* e = (swamp_enum*) v;
            SWAMP_LOG_INFO("enum %d", e->enum_type);
            *p++ = e->enum_type;
            size_t count = e->info.field_count;
            if (p + 2 >= endp) {
                SWAMP_LOG_SOFT_ERROR("wrote too long");
                return -1;
            }
            write_u16(&p, count);
            for (size_t i = 0; i < count; ++i) {
                const swamp_value* f = e->fields[i];
                swamp_serialize(f, &p, endp - p);
            }
            break;
        }
        case swamp_type_list: {
            const swamp_list* l = (swamp_list*) v;
            size_t count = l->count;
            SWAMP_LOG_INFO("list %zu", count);
            if (p + 2 >= endp) {
                SWAMP_LOG_SOFT_ERROR("wrote too long");
                return -1;
            }
            write_u16(&p, l->count);
            const swamp_list* cur = l;
            for (size_t i = 0; i < count; ++i) {
                swamp_serialize(cur->value, &p, endp - p);
                cur = cur->next;
            }
            break;
        }
        case swamp_type_function:
            SWAMP_LOG_ERROR("functions are not supported");
        case swamp_type_external_function:
            SWAMP_LOG_ERROR("external function not supported");
        case swamp_type_boolean: {
            const swamp_boolean* b = (swamp_boolean*) v;
            SWAMP_LOG_INFO("bool %d", b->truth);
            *p++ = b->truth;
            break;
        }
        case swamp_type_blob:
            SWAMP_LOG_ERROR("blobs are not supported");
        default:
            SWAMP_LOG_ERROR("not supported");
            return 0;
    }

    *target = p;
    return 0;
}
