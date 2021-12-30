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
#include <swamp-runtime/debug.h>
#include <tinge/tinge.h>
#include <flood/out_stream.h>

void swampCoreDebugLog(const SwampString** result, SwampMachineContext* context, const SwampString** value)
{
    const char* filenameAndLocation;

#define SWAMP_LOG_BUF_SIZE (512)
    static char buf[SWAMP_LOG_BUF_SIZE];

    FldOutStream stream;

    fldOutStreamInit(&stream, buf, SWAMP_LOG_BUF_SIZE);

    TingeState tinge;

    int lookupErr = swampDebugInfoFindLinesInContextToStringSingleLine(context, &filenameAndLocation);
    if (lookupErr < 0) {
        filenameAndLocation = "";
    }


    tingeStateInit(&tinge, &stream);

    tingeStateFgColor(&tinge, 91);
    fldOutStreamWritef(&stream, "%s ", filenameAndLocation);

    tingeStateFgColor(&tinge, 98);
    fldOutStreamWrites(&stream, "log: ");

    tingeStateReset(&tinge);
    fldOutStreamWritef(&stream, "%s", (*value)->characters);

    CLOG_OUTPUT(buf);
}

void swampPanic(SwampMachineContext* context, const char* format, ...)
{
#define SWAMP_PANIC_BUF_SIZE (512)
    static char buf[SWAMP_PANIC_BUF_SIZE];
    va_list argp;
    va_start(argp, format);
    vsnprintf(buf, SWAMP_PANIC_BUF_SIZE, format, argp);

    va_end(argp);
    CLOG_SOFT_ERROR("panic: %s", buf);
}

void swampCoreDebugPanic(SwampString** result, SwampMachineContext* context, const SwampString** value)
{
    swampPanic(context, (*value)->characters);

    const char* outString;
    swampDebugInfoFindLinesInContextToString(context, &outString);

    CLOG_SOFT_ERROR("raised from location:\n%s", outString);

    CLOG_ERROR("panic");
}

void swampCoreDebugLogAny(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

    const char* filenameAndLocation;
    int lookupErr = swampDebugInfoFindLinesInContextToStringSingleLine(context, &filenameAndLocation);
    if (lookupErr < 0) {
        filenameAndLocation = "";
    }

#define MaxBufSize (8*1024)
    char buf[MaxBufSize];

    CLOG_OUTPUT("%s log: %s", filenameAndLocation, swampDumpToAsciiString(value, foundType, 0, buf, MaxBufSize));
}


void swampCoreDebugToString(const SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

#define MaxBufSizeToString (32*1024)
    static char buf[MaxBufSizeToString];

    swampDumpToAsciiString(value, foundType, 0, buf, MaxBufSizeToString);

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
