/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_EXECUTE_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_EXECUTE_H

#include <swamp-runtime/types.h>

int swampGetFunc(const SwampFunction* func, const SwampFunc** outFn);
SwampMemoryPosition swampExecutePrepare(const SwampFunction* func, const void* bp, const SwampFunc** outFn);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_EXECUTE_H
