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
#include <stdarg.h>


void swampCoreDebugLog(const SwampString** result, SwampMachineContext* context, const SwampString** value)
{
    CLOG_OUTPUT("log: %s", (*value)->characters);
}

void swampPanic(SwampMachineContext* context, const char* format, ...)
{
#define SWAMP_PANIC_BUF_SIZE (512)
    static char buf[SWAMP_PANIC_BUF_SIZE];
    va_list argp;
    va_start(argp, format);
    vsnprintf(buf, SWAMP_PANIC_BUF_SIZE, format, argp);

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


void swampCoreDebugToString(const SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

#define MaxBufSizeToString (256*1024)
    static char buf[MaxBufSizeToString];

    swampDumpToAsciiString(value, foundType, 0, buf, MaxBufSize);

    *result = swampStringAllocate(context->dynamicMemory, buf);
}

void* swampCoreDebugFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Debug.log", (void*)swampCoreDebugLog},
        {"Debug.logAny", (void*)swampCoreDebugLogAny},
        {"Debug.toString", (void*)swampCoreDebugToString},
        {"Debug.panic", (void*)swampCoreDebugPanic}
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
