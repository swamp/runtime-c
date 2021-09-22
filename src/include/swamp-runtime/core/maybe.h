/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_maybe_h
#define swamp_core_maybe_h

#include <swamp-runtime/swamp.h>
#include <tiny-libc/tiny_libc.h>

void* swampCoreMaybeFindFunction(const char* fullyQualifiedName);

#define swampMaybeNothing(result) *result = 0
#define swampMaybeJust(result, align, value, octetSize) *result = 1;  tc_memcpy_octets(result+align, value, octetSize)

#endif
