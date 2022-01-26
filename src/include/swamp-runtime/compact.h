/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_COMPACT_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_COMPACT_H

struct SwampDynamicMemory;
struct SwtiType;

int swampCompact(const void* state, const struct SwtiType* stateType, struct SwampDynamicMemory* targetMemory,
                 void** compactedState);



#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_COMPACT_H
