/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_bind_h
#define swamp_core_bind_h

#include <swamp-runtime/types.h>

typedef struct SwampBindingInfo {
    const char* name;
    void* fn;
} SwampBindingInfo;

void* swampCoreFindFunction(const char* function_name);

#endif
