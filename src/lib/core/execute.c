/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/core/execute.h>

const swamp_value* swamp_execute_1(swamp_allocator* allocator, const swamp_function* predicate_fn_object, const swamp_value* item)
{
	const int verbose_flag = 0;
	const swamp_value* result;

	if (predicate_fn_object->type == swamp_function_type_internal) {
		const swamp_func* fn = predicate_fn_object->internal_function;
		result = swamp_execute(allocator, fn, &item, 1, verbose_flag);
	} else {
		if (predicate_fn_object->external_function == 0) {
			SWAMP_ERROR("external function is zero...");
		}
		result = swamp_execute_external(allocator, predicate_fn_object->external_function, &item, 1, verbose_flag);
	}

	return result;
}

const swamp_value* swamp_execute_2(swamp_allocator* allocator, const swamp_function* predicate_fn_object,
								   const swamp_value* index, const swamp_value* item)
{
	const int verbose_flag = 0;
	const swamp_value* result;
	const swamp_value* arguments[2];
	arguments[0] = index;
	arguments[1] = item;

	if (predicate_fn_object->type == swamp_function_type_internal) {
		result = swamp_run(allocator, predicate_fn_object->internal_function, arguments, 2, verbose_flag);
	} else {
		result = swamp_execute_external(allocator, predicate_fn_object->external_function, arguments, 2, verbose_flag);
	}
	return result;
}
