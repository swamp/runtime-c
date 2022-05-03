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
#include <swamp-runtime/debug_variables.h>

#define SWAMP_LOG_ENABLED 1
    //(CONFIGURATION_DEBUG)

#if SWAMP_LOG_ENABLED
#include <flood/out_stream.h>
#include <swamp-dump/dump_ascii_no_color.h>
#include <swamp-dump/types.h>
#include <tinge/tinge.h>
#endif


void swampCoreDebugLog(const SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    if (context->hackIsPredicting) {
        *result = 0;
        return;
    }

    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

#if SWAMP_LOG_ENABLED
    const char* filenameAndLocation;

#define SWAMP_LOG_BUF_SIZE (64*1024)
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

    const SwtiType* unAliasType = swtiUnalias(foundType);
    if (unAliasType->type == SwtiTypeTuple) {
        const SwtiTupleType* tuple = (const SwtiTupleType*) unAliasType;
        for (size_t i = 0; i < tuple->fieldCount; i++) {
            const SwtiTupleTypeField* field = &tuple->fields[i];
            int errorCode = swampDumpToAscii(value + field->memoryOffsetInfo.memoryOffset, field->fieldType, swampDumpFlagNoStringQuotesOnce,
                                             0, &stream);
            if (errorCode != 0) {
                CLOG_ERROR("tuple inline", errorCode);
            }
        }
    } else {
        int dumpResult = swampDumpToAscii(value, foundType, swampDumpFlagNoStringQuotesOnce, 0, &stream);
        if (dumpResult < 0) {
            CLOG_ERROR("could not dump result")
        }
    }

    tingeStateReset(&tinge);

#if 0
    const char* variableString;
    int variablesErr = swampDebugInfoFindVariablesInContextToString(context, &variableString);
    if (variablesErr < 0) {
        variableString = "";
    }

    fldOutStreamWritef(&stream, "\n%s", variableString);

#endif

    CLOG_OUTPUT_STDERR("%s", buf);
    *result = 0;
#else
    *result = 0;
#endif
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

    const char* outString;
    swampDebugInfoFindLinesInContextToString(context, &outString);

    CLOG_ERROR("raised from location:\n%s", outString);

    CLOG_BREAK;
}

static void swampCoreDebugPanic(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    static char buf[1024];
    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

    const char* str = swampDumpToAsciiString(value, foundType, 0, buf, 1024);
    swampPanic(context, str);

    *result = 0;
}

static void swampCoreDebugLogAny(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    if (context->hackIsPredicting) {
        *result = 0;
        return;
    }
#if SWAMP_LOG_ENABLED
    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

    const char* filenameAndLocation;
    int lookupErr = swampDebugInfoFindLinesInContextToStringSingleLine(context, &filenameAndLocation);
    if (lookupErr < 0) {
        filenameAndLocation = "";
    }

#define MaxBufSize (8*1024)
    char buf[MaxBufSize];

    CLOG_OUTPUT_STDERR("%s log: %s", filenameAndLocation, swampDumpToAsciiString(value, foundType, 0, buf, MaxBufSize));
#else
#endif
    *result = 0;
}


void swampCoreDebugToString(const SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{
    const SwtiType* foundType = swtiChunkTypeFromIndex(context->typeInfo, *typeIndex);

#define MaxBufSizeToString (32*1024)
    static char buf[MaxBufSizeToString];

    swampDumpToAsciiStringNoColor(value, foundType, 0, buf, MaxBufSizeToString);

    *result = swampStringAllocate(context->dynamicMemory, buf);
}

const void* swampCoreDebugFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Debug.log", SWAMP_C_FN(swampCoreDebugLog)},
        //{"Debug.logAny", SWAMP_C_FN(swampCoreDebugLogAny)},
        {"Debug.toString", SWAMP_C_FN(swampCoreDebugToString)},
        {"Debug.panic", SWAMP_C_FN(swampCoreDebugPanic)}
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
