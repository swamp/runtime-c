/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_transduce_h
#define swamp_transduce_h

#include <swamp-runtime/types.h>

struct swamp_value;
struct swamp_list;
struct swamp_function;
struct swamp_allocator;

typedef const struct swamp_value* (*swamp_transducer_stepper)(const struct swamp_value* predicate_value,
                                                              const struct swamp_value* item, swamp_bool* should_add_it,
                                                              swamp_bool* should_continue);
const struct swamp_value* swamp_transduce_internal(struct swamp_allocator* allocator, swamp_transducer_stepper stepper,
                                                   const struct swamp_function* predicate_fn_object,
                                                   const struct swamp_list* sequence, swamp_bool insert_index);
const struct swamp_value* swamp_transduce_internal_cast(struct swamp_allocator* allocator,
                                                        swamp_transducer_stepper stepper, swamp_bool insert_index,
                                                        const struct swamp_value** arguments, int argument_count);

#endif
