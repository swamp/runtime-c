/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/core/execute.h>
#include <swamp-runtime/core/transduce.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/ref_count.h>

const struct swamp_value* swamp_transduce_internal(swamp_allocator* allocator, swamp_transducer_stepper stepper,
                                                   const swamp_function* predicate_fn_object,
                                                   const swamp_list* sequence, int insert_index)
{
    const swamp_list* iterator = swamp_list_first(sequence);
    const swamp_list* result_list = 0;
    int index = 0;

    const swamp_list* tempItems[512];
    size_t itemCount = 0;

    while (iterator) {
        const swamp_value* item = iterator->value;
        const struct swamp_value* predicate_value;
        if (insert_index) {
            const swamp_value* index_object = swamp_allocator_alloc_integer(allocator, index);
            index++;
            predicate_value = swamp_execute_2(allocator, predicate_fn_object, index_object, item);
            DEC_REF(index_object);
        } else {
            predicate_value = swamp_execute_1(allocator, predicate_fn_object, item);
        }
        swamp_bool should_add_it;
        swamp_bool should_continue;
        const swamp_value* object_to_add = stepper(predicate_value, item, &should_add_it, &should_continue);
        if (should_add_it) {
            result_list = tempItems[itemCount++] = object_to_add;
        } else {
            // SWAMP_LOG_INFO("we shouldn't add it, ignoring...");
        }
        if (!should_continue) {
            break;
        }
        iterator = iterator->next;
    }

    if (itemCount == 0) {
        return swamp_allocator_alloc_list_empty(allocator);
    }

    return swamp_allocator_alloc_list_create(allocator, tempItems, itemCount);
}

const struct swamp_value* swamp_transduce_internal_cast(struct swamp_allocator* allocator,
                                                        swamp_transducer_stepper stepper, swamp_bool insert_index,
                                                        const struct swamp_value** arguments, int argument_count)
{
    const swamp_function predicate_function = swamp_value_function(arguments[0]);
    const swamp_list* sequence_value = swamp_value_list(arguments[1]);

    return swamp_transduce_internal(allocator, stepper, &predicate_function, sequence_value, insert_index);
}
