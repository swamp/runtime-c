/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef TURMOIL_DEPS_SWAMP_RUNTIME_C_SRC_INCLUDE_SWAMP_RUNTIME_PANIC_H
#define TURMOIL_DEPS_SWAMP_RUNTIME_C_SRC_INCLUDE_SWAMP_RUNTIME_PANIC_H

struct SwampMachineContext;

void swampPanic(struct SwampMachineContext* context, const char* message);

#endif // TURMOIL_DEPS_SWAMP_RUNTIME_C_SRC_INCLUDE_SWAMP_RUNTIME_PANIC_H
