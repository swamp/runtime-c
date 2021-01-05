/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_ref_count_h
#define swamp_ref_count_h

#include <swamp-runtime/log.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/types.h>

// -------------------------------------------------------------
// Reference counting
// -------------------------------------------------------------
#define INC_REF(v) ((swamp_value*) (v))->internal.ref_count++;

#define INC_REF_CHECK_NULL(v)                                                                                          \
    if (v) {                                                                                                           \
        ((swamp_value*) (v))->internal.ref_count++;                                                                    \
    }

void swamp_allocator_free(const swamp_value* v);

#define DEC_REF(v)                                                                                                     \
    ((swamp_value*) (v))->internal.ref_count--;                                                                        \
    if ((v)->internal.ref_count == 0) {                                                                                \
        if ((v)->internal.erase_code != 0xc0de) {                                                                      \
            SWAMP_ERROR("dealloc: overwrite");                                                                         \
        }                                                                                                              \
        ((swamp_value*) (v))->internal.erase_code = 0xdead;                                                            \
        swamp_allocator_free(v);                                                                                       \
    }

#define DEC_REF_CHECK_NULL(v)                                                                                          \
    if (v != 0) {                                                                                                      \
        DEC_REF(v);                                                                                                    \
    }

#endif
