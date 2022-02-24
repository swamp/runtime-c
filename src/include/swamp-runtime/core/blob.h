/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_blob_h
#define swamp_core_blob_h

#include <swamp-runtime/swamp.h>

#include <swamp-runtime/types.h>

struct SwampMachineContext;

const void* swampCoreBlobFindFunction(const char* fullyQualifiedName);

#endif
