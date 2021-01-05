/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_h
#define swamp_h

#include <swamp-runtime/types.h>

swamp_external_fn swamp_core_find_function(const char* function_name);

#define SWAMP_FUNCTION(NAME)                                                                                           \
    static const swamp_value* NAME(struct swamp_allocator* allocator, const swamp_value** arguments, int argument_count)
#define SWAMP_FUNCTION_EXPOSE(NAME)                                                                                    \
    const swamp_value* NAME(struct swamp_allocator* allocator, const swamp_value** arguments, int argument_count)
#define SWAMP_FUNCTION_EXPOSE_DECLARE(NAME)                                                                            \
    const swamp_value* NAME(struct swamp_allocator* allocator, const swamp_value** arguments, int argument_count)

const swamp_value* swamp_run(struct swamp_allocator* allocator, const swamp_func* f, const swamp_value** registers,
                             size_t register_count, int verbose_flag);

const swamp_value* swamp_execute(struct swamp_allocator* allocator, const swamp_func* f, const swamp_value** registers,
                                 size_t register_count, int verbose_flag);

#define swamp_execute_external(allocator, func, args, arg_count, verbose_flag) (func)->fn(allocator, args, arg_count)

void swamp_registers_release(const swamp_value** values, size_t count);

#endif
