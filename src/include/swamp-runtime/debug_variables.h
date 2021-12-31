/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DEBUG_VARIABLES_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DEBUG_VARIABLES_H

#include <stdint.h>

struct SwampCallStack;
struct SwampCallStackEntry;
struct SwampMachineContext;
struct SwtiChunk;
struct FldOutStream;


typedef struct SwampDebugInfoVariablesEntry {
    uint16_t startOpcodePosition;
    uint16_t endOpcodePosition;
    uint16_t typeId;
    uint16_t scopeId;
    uint16_t stackPosition;
    uint16_t stackRange;
    const char* name;
} SwampDebugInfoVariablesEntry;

typedef struct SwampDebugInfoVariables {
    uint32_t count;
    const SwampDebugInfoVariablesEntry* variables;
} SwampDebugInfoVariables;

int swampDebugInfoVariablesInCallStackEntryToString(const struct SwampCallStackEntry* callStackEntry, const struct SwtiChunk* typeInformation, struct FldOutStream* stream);
int swampDebugInfoFindVariablesInContextToString(const struct SwampMachineContext* machineContext, const char** outString);
void swampDebugInfoVariablesOutput(const SwampDebugInfoVariables* variables, const char* description);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DEBUG_VARIABLES_H

