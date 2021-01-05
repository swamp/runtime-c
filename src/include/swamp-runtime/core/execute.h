/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_execute_h
#define swamp_core_execute_h

struct swamp_value;
struct swamp_allocator;
struct swamp_function;

const struct swamp_value* swamp_execute_1(struct swamp_allocator* allocator,
                                          const struct swamp_function* predicate_fn_object,
                                          const struct swamp_value* item);

const struct swamp_value* swamp_execute_2(struct swamp_allocator* allocator,
                                          const struct swamp_function* predicate_fn_object,
                                          const struct swamp_value* index, const struct swamp_value* item);

#endif
