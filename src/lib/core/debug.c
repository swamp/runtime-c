/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <string.h>
#include <swamp-dump/dump_ascii.h>
#include <swamp-dump/dump_ascii_no_color.h>
#include <swamp-dump/types.h>
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/ref_count.h>
#include <swamp-runtime/swamp.h>
#include <swamp-typeinfo/chunk.h>

SWAMP_FUNCTION_EXPOSE(swamp_core_debug_log)
{
    const swamp_string* output = swamp_value_string(arguments[0]);


    fprintf(stderr, "log: ");
    fputs(output->characters, stderr);
    fputs("\n\033[39m", stderr);

    fflush(stderr);

    INC_REF(arguments[0]);
    return (const swamp_value*) output;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_debug_to_string)
{
#define tempBufSize (32 * 1024)
    char buf[tempBufSize];
    buf[0] = 0;

    const swamp_value* any = arguments[0];
    swamp_int32 typeIndex;

    const swamp_value* v = swamp_value_any(any, &typeIndex);

    const SwtiType* foundType = swtiChunkTypeFromIndex(allocator->typeInfo, typeIndex);

    swampDumpToAsciiStringNoColor(v, foundType, swampDumpFlagAliasOnce, buf, tempBufSize);

    return swamp_allocator_alloc_string(allocator, buf);
}
