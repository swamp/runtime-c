/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_math_h
#define swamp_core_math_h

#include <swamp-runtime/swamp.h>

void swampCoreMathRemainderBy(SwampInt32* result, SwampMachineContext* context, const SwampInt32* divider, const SwampInt32* value);
void* swampCoreMathFindFunction(const char* fullyQualifiedName);

#endif
