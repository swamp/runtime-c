/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_debug_h
#define swamp_core_debug_h

#include <swamp-runtime/swamp.h>

void swampCoreDebugLog(SwampString** result, SwampMachineContext* context, const struct SwampString** value);
void swampCoreDebugToString(SwampString** result, SwampMachineContext* context, const SwampInt32* typeIndex, const struct SwampString** value);

#endif
