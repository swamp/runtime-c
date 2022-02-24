/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CORE_LIST_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CORE_LIST_H

#include <swamp-runtime/types.h>

struct SwampMachineContext;

const void* swampCoreListFindFunction(const char* fullyQualifiedName);

void swampCoreListHead(SwampMaybe* result, struct SwampMachineContext* context, const SwampList** list);



#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CORE_LIST_H
