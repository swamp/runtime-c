/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_reduce_h
#define swamp_reduce_h

#include <swamp-runtime/types.h>

struct swamp_value;
struct swamp_allocator;

typedef const struct swamp_value* (*swamp_c_fn_reducer_work_check)(struct swamp_allocator* allocator,
                                                                   const struct swamp_value* a,
                                                                   const struct swamp_value* b,
                                                                   swamp_bool* should_continue);

const struct swamp_value* swamp_foldl_internal_single_fn(struct swamp_allocator* allocator,
    const struct swamp_value** arguments,
    int argument_count,
    swamp_c_fn_reducer_work_check single_check,
    const char* debug);


const struct swamp_value* swamp_reducer_reduce_internal_single_fn(struct swamp_allocator* allocator,
                                                                  const struct swamp_value** arguments,
                                                                  int argument_count,
                                                                  swamp_c_fn_reducer_work_check single_check,
                                                                  const char* debug);

const struct swamp_value* swamp_reducer_reduce_internal_stop_single_fn(struct swamp_allocator* allocator,
                                                                       const swamp_value** arguments,
                                                                       int argument_count, const char* debug);

#endif
