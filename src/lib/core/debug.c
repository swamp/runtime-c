/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-dump/dump_ascii.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/debug.h>

void swampCoreDebugLog(SwampString** result, SwampMachineContext* context, const SwampString** value)
{
    CLOG_INFO((*value)->characters);
}

void swampCoreDebugLogRecord(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{

}


void swampCoreDebugToString(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{

}

void* swampCoreDebugFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Debug.log", swampCoreDebugLog},
        {"Debug.logRecord", swampCoreDebugLogRecord},
        {"Debug.toString", swampCoreDebugToString},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}