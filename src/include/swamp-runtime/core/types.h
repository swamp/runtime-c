/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CORE_TYPES_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CORE_TYPES_H

#include <swamp-runtime/types.h>

typedef struct SwampCoreSize2i {
    SwampInt32 height;
    SwampInt32 width;
} SwampCoreSize2i;

typedef struct SwampCorePosition2i {
    SwampInt32 x;
    SwampInt32 y;
} SwampCorePosition2i;

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_CORE_TYPES_H
