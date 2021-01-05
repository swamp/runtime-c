/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/core/blob.h>

SWAMP_FUNCTION_EXPOSE(swamp_core_blob_is_empty)
{
	const swamp_blob* blob = swamp_value_blob(arguments[0]);

    return swamp_allocator_alloc_boolean(allocator, blob->octet_count == 0);
}
