/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-dump/dump_ascii.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/debug.h>
#include <swamp-typeinfo/chunk.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/panic.h>

void swampCoreDebugLog(SwampString** result, SwampMachineContext* context, const SwampString** value)
{
    CLOG_OUTPUT("log: %s", (*value)->characters);
}

void swampPanic(SwampMachineContext* context, const char* format, ...)
{
    static char buf[512];
    va_list argp;
    va_start(argp, format);
    vsnprintf(buf, 512, format, argp);

    CLOG_ERROR("panic: %s", buf);
    va_end(argp);
}

void swampCoreDebugPanic(SwampString** result, SwampMachineContext* context, const SwampString** value)
{
    swampPanic(context, (*value)->characters);
}

void swampCoreDebugLogAny(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

#define MaxBufSize (8*1024)
    char buf[MaxBufSize];

    CLOG_OUTPUT("log: %s", swampDumpToAsciiString(value, foundType, 0, buf, MaxBufSize));
}


void swampCoreDebugToString(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

#define MaxBufSize (128*1024)
    static char buf[MaxBufSize];

    swampDumpToAsciiString(value, foundType, 0, buf, MaxBufSize);

    *result = swampStringAllocate(context->dynamicMemory, buf);
}

void* swampCoreDebugFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Debug.log", swampCoreDebugLog},
        {"Debug.logAny", swampCoreDebugLogAny},
        {"Debug.toString", swampCoreDebugToString},
        {"Debug.panic", swampCoreDebugPanic}
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}