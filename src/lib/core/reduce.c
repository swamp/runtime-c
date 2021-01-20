/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/core/execute.h>
#include <swamp-runtime/core/reduce.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/ref_count.h>
#include <swamp-runtime/swamp.h>



const struct swamp_value* swamp_foldl_internal_single_fn(swamp_allocator* allocator, const swamp_value** arguments,
    int argument_count, swamp_c_fn_reducer_work_check single_check,
    const char* debug)
{
    if (argument_count < 2) {
        SWAMP_ERROR("Must have at least one arguments");
    }

    swamp_function predicate_fn_object = swamp_value_function(arguments[0]);
    const swamp_value* accumulator = arguments[1];
    const swamp_list* seq_object = swamp_value_list(arguments[2]);

    const swamp_value* a = accumulator;
    swamp_bool should_continue;

    SWAMP_LIST_FOR_LOOP(seq_object)
        const swamp_value* predicate_value = swamp_execute_2(allocator, &predicate_fn_object, value, a);
        a = single_check(allocator, predicate_value, value, &should_continue);
        if (!should_continue) {
            break;
        }
    SWAMP_LIST_FOR_LOOP_END()

    return a;
}


const struct swamp_value* swamp_reducer_reduce_internal_single_fn(swamp_allocator* allocator, const swamp_value** arguments,
																  int argument_count, swamp_c_fn_reducer_work_check single_check,
																  const char* debug)
{
	if (argument_count < 2) {
		SWAMP_ERROR("Must have at least one arguments");
	}

	swamp_function predicate_fn_object = swamp_value_function(arguments[0]);
	const swamp_list* seq_object = swamp_value_list(arguments[1]);

	const swamp_value* a = 0;
	swamp_bool should_continue;

	SWAMP_LIST_FOR_LOOP(seq_object)
	const swamp_value* predicate_value = swamp_execute_1(allocator, &predicate_fn_object, value);
	a = single_check(allocator, predicate_value, value, &should_continue);
	if (!should_continue) {
		break;
	}
	SWAMP_LIST_FOR_LOOP_END()
	if (!a) {
		a = (const swamp_value*) swamp_allocator_alloc_list_empty(allocator);
	}
	return a;
}



const struct swamp_value* swamp_reducer_reduce_internal_stop_single_fn(swamp_allocator* allocator, const swamp_value** arguments,
																	   int argument_count, const char* debug)
{
	if (argument_count < 3) {
		SWAMP_ERROR("Must have at least three arguments");
	}

	swamp_function predicate_fn_object = swamp_value_function(arguments[0]);
	const swamp_value* accumulator = arguments[1];
	const swamp_list* seq_object = swamp_value_list(arguments[2]);

	swamp_bool should_continue;
	// swamp_value_print((const swamp_value*) seq_object, "list");
	// swamp_value_print(predicate_fn_object.internal_function, "predicate fn");
	// swamp_value_print(accumulator, "accumulator");

	//	int index = -1;
	SWAMP_LIST_FOR_LOOP(seq_object)
	const swamp_value* predicate_value = swamp_execute_2(allocator, &predicate_fn_object, value, accumulator);
	should_continue = swamp_value_is_just(predicate_value);
	if (!should_continue) {
		break;
	}
	accumulator = swamp_value_just(predicate_value);
	//	index++;
	SWAMP_LIST_FOR_LOOP_END()

	//	swamp_value_print(accumulator, "resulting accumulator");

	return accumulator;
}
