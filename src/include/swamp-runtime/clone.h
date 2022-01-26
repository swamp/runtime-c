/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CLONE_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CLONE_H

struct SwampDynamicMemory;
struct SwtiType;

int swampClone(const void* state, const struct SwtiType* stateType, const struct SwampDynamicMemory* targetMemory,
                 void** clonedState);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CLONE_H
