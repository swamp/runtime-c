/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H

struct SwampDynamicMemory;
struct SwampString;

const struct SwampString* swampStringAllocate(struct SwampDynamicMemory* self, const char* s);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_SWAMP_ALLOCATE_H
