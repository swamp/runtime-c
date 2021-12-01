/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_debug_h
#define swamp_core_debug_h

#include <swamp-runtime/swamp.h>
#include <swamp-runtime/context.h>

void swampCoreDebugLog(const SwampString** result, SwampMachineContext* context, const struct SwampString** value);
void swampCoreDebugToString(const SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const void* value);
void* swampCoreDebugFindFunction(const char* fullyQualifiedName);

#endif
