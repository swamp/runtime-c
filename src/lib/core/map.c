/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/core/map.h>
#include <swamp-runtime/core/reduce.h>
#include <swamp-runtime/core/transduce.h>
#include <swamp-runtime/core/execute.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/swamp.h>

// TODO: List.repeat, List.range, List.indexedMap

static const swamp_value* do_map(const struct swamp_value* predicate_value, const struct swamp_value* item,
								 swamp_bool* should_add_it, swamp_bool* should_continue)
{
	*should_add_it = swamp_true;
	*should_continue = swamp_true;
	return predicate_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_map)
{
	return swamp_transduce_internal_cast(allocator, do_map, swamp_false, arguments, argument_count);
}

static const swamp_value* do_indexed_map(const struct swamp_value* predicate_value, const struct swamp_value* item,
    swamp_bool* should_add_it, swamp_bool* should_continue)
{
    *should_add_it = swamp_true;
    *should_continue = swamp_true;
    return predicate_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_indexed_map)
{
    return swamp_transduce_internal_cast(allocator, do_indexed_map, swamp_true, arguments, argument_count);
}


SWAMP_FUNCTION_EXPOSE(swamp_core_map2)
{
    const struct swamp_function fn = swamp_value_function(arguments[0]);
	const swamp_list* firstList = swamp_value_list(arguments[1]);
    const swamp_list* secondList = swamp_value_list(arguments[2]);
	int minCount = firstList->count < secondList->count ? firstList->count : secondList->count;

    const swamp_list* firstIterator = swamp_list_first(firstList);
    const swamp_list* secondIterator = swamp_list_first(secondList);

	const swamp_value* values[1024];

	for (int i=0; i < minCount; i++) {
        const swamp_value* firstValue = firstIterator->value;
		const swamp_value* secondValue = secondIterator->value;

        const swamp_value* result = swamp_execute_2(allocator, &fn, firstValue, secondValue);
        values[i] = result;

		firstIterator = firstIterator->next;
		secondIterator = secondIterator->next;
	}

    const swamp_list* resultList = swamp_allocator_alloc_list_create(allocator, values, minCount);

    return (const swamp_value*) resultList;
}


static const swamp_list* concat_helper(swamp_allocator* allocator, const swamp_list* attach, const swamp_list* sequences)
{
	SWAMP_LIST_FOR_LOOP(sequences)
	attach = swamp_allocator_alloc_list_conj(allocator, attach, value);
	SWAMP_LIST_FOR_LOOP_END()

	return attach;
}

static const swamp_list* core_concat(swamp_allocator* allocator, const swamp_list* sequences)
{
	const swamp_list* parent = 0;
	SWAMP_LIST_FOR_LOOP(sequences)
	const swamp_list* sublist = swamp_value_list(value);
	parent = concat_helper(allocator, parent, sublist);
	SWAMP_LIST_FOR_LOOP_END()
	swamp_list_finalize(parent);

	return parent;
}

static const swamp_value* core_concat_cast(swamp_allocator* allocator, const struct swamp_value* sequences)
{
	return (const swamp_value*) core_concat(allocator, swamp_value_list(sequences));
}

SWAMP_FUNCTION_EXPOSE(swamp_core_concat)
{
	return core_concat_cast(allocator, arguments[0]);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_mapcat)
{
	const swamp_value* sequences = swamp_core_map(allocator, arguments, argument_count);
	swamp_value_print(sequences, "sequences");
	return core_concat_cast(allocator, sequences);
}

static const swamp_value* do_some(swamp_allocator* allocator, const swamp_value* predicate_value, const swamp_value* item,
								  swamp_bool* should_continue)
{
	swamp_bool truth = swamp_value_bool(predicate_value);
	*should_continue = !truth;
	return predicate_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_any)
{
	return swamp_reducer_reduce_internal_single_fn(allocator, arguments, argument_count, do_some, "any");
}

static const swamp_value* do_filter(const struct swamp_value* predicate_value, const struct swamp_value* item,
									swamp_bool* should_add_it, swamp_bool* should_continue)
{
	*should_add_it = swamp_value_bool(predicate_value);
	*should_continue = swamp_true;
	return item;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_filter)
{
	return swamp_transduce_internal_cast(allocator, do_filter, swamp_false, arguments, argument_count);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_filter_map)
{
	const struct swamp_function maybeFn = swamp_value_function(arguments[0]);
	const swamp_list* sequence = swamp_value_list(arguments[1]);

	const swamp_value* values[1024];
	int resultIndex = 0;

	SWAMP_LIST_FOR_LOOP(sequence)
		const swamp_value* maybeResult = swamp_execute_1(allocator, &maybeFn, value);
		if (swamp_value_is_just(maybeResult)) {
			values[resultIndex++] = swamp_value_just(maybeResult);
		}
	SWAMP_LIST_FOR_LOOP_END()

	const swamp_list* resultList = swamp_allocator_alloc_list_create(allocator, values, resultIndex);

	return (const swamp_value*)resultList;
}

static const swamp_value* do_remove(const struct swamp_value* predicate_value, const struct swamp_value* item,
									swamp_bool* should_add_it, swamp_bool* should_continue)
{
	*should_add_it = !swamp_value_bool(predicate_value);
	*should_continue = swamp_true;
	return item;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_remove)
{
	return swamp_transduce_internal_cast(allocator, do_remove, swamp_false, arguments, argument_count);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_empty)
{
	const swamp_list* list = swamp_value_list(arguments[0]);
	swamp_bool is_empty = swamp_list_is_empty(list);
	return swamp_allocator_alloc_boolean(allocator, is_empty);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_first)
{
	const swamp_list* list = swamp_value_list(arguments[0]);
	const swamp_list* iterator = swamp_list_first(list);
	if (!iterator) {
		return swamp_allocator_alloc_enum_no_value(allocator, 0);
	}

	return swamp_allocator_alloc_enum_single_value(allocator, 1, iterator->value);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_second)
{
	const swamp_list* list = swamp_value_list(arguments[0]);
	const swamp_list* iterator = swamp_list_first(list);
	if (!iterator) {
		SWAMP_ERROR("second: was empty list");
	}
	iterator = iterator->next;
	if (!iterator) {
		SWAMP_ERROR("second: was single item list");
	}

	return iterator->value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_range)
{
	if (argument_count < 2) {
		SWAMP_ERROR("range: wrong number of arguments");
	}
	const swamp_int32 start = swamp_value_int(arguments[0]);
	const swamp_int32 end = swamp_value_int(arguments[1]);
	//	const swamp_int32 step = swamp_value_int(arguments[2]);
	swamp_int32 step = 1;
	if (start > end) {
		step = -1;
	}

	const swamp_list* new_list = 0;
	size_t count = (end - start + 1) / step;
	if (count > 256) {
		SWAMP_ERROR("Too big range!");
	}
	for (size_t i = 0; i < count; ++i) {
		swamp_int32 v = end - (i * step);
		const swamp_value* int_object = swamp_allocator_alloc_integer(allocator, v);
		new_list = swamp_allocator_alloc_list_conj(allocator, new_list, int_object);
	}
	swamp_list_finalize(new_list);

	return (const swamp_value*) new_list;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_length)
{
	const swamp_list* list = swamp_value_list(arguments[0]);
	const swamp_value* int_object = swamp_allocator_alloc_integer(allocator, list->count);
	return int_object;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_nth)
{
	const swamp_list* list = swamp_value_list(arguments[0]);
	const swamp_int32 index = swamp_value_int(arguments[1]);

	const swamp_list* iterator = swamp_list_first(list);
	for (swamp_int32 i = 0; i < index; ++i) {
		if (!iterator) {
			SWAMP_ERROR("nth: index too great");
		}
		iterator = iterator->next;
	}
	if (!iterator) {
		SWAMP_ERROR("nth: index too great");
	}
	return iterator->value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_rest)
{
	const swamp_list* list = swamp_value_list(arguments[0]);
	const swamp_list* iterator = swamp_list_first(list);
	if (!iterator) {
		SWAMP_ERROR("rest: list is empty");
	}
	iterator = iterator->next;

	if (!iterator) {
		return (const swamp_value*) swamp_allocator_alloc_list_empty(allocator);
	}
	return (const swamp_value*) iterator->next;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_reduce)
{
	return 0;
}

static const swamp_value* do_foldl(swamp_allocator* allocator, const swamp_value* predicate_value, const swamp_value* item,
    swamp_bool* should_continue)
{
    *should_continue = 1;
    return predicate_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_foldl)
{
	return swamp_foldl_internal_single_fn(allocator, arguments, argument_count, do_foldl, "foldl");
}

SWAMP_FUNCTION_EXPOSE(swamp_core_reduce_stop)
{
	return swamp_reducer_reduce_internal_stop_single_fn(allocator, arguments, argument_count, "swamp_core_reduce_stop");
}
