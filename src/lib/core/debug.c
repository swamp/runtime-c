/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-dump/dump_ascii.h>
#include <clog/clog.h>
#include <swamp-runtime/swamp.h>

void swampCoreDebugLog(SwampString** result, SwampMachineContext* context, const SwampString** value)
{
    CLOG_INFO((*value)->characters);
}


void swampCoreDebugToString(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value)
{

}
