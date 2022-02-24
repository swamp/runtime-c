/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_math_h
#define swamp_core_math_h

#include <swamp-runtime/swamp.h>

struct SwampMachineContext;

void swampCoreMathRemainderBy(SwampInt32* result, struct SwampMachineContext* context, const SwampInt32* divider, const SwampInt32* value);
const void* swampCoreMathFindFunction(const char* fullyQualifiedName);

#endif
