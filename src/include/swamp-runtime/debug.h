/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DEBUG_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DEBUG_H

#include <stdint.h>

struct SwampCallStack;
struct SwampMachineContext;

typedef struct SwampDebugInfoSourceLineLocation {
    uint16_t line;
    uint16_t column;
} SwampDebugInfoSourceLineLocation;

typedef struct SwampDebugInfoLinesEntry {
    uint16_t opcodePosition;
    uint16_t sourceFileId;
    SwampDebugInfoSourceLineLocation startLocation;
} SwampDebugInfoLinesEntry;

typedef struct SwampDebugInfoLines {
    uint32_t count;
    const SwampDebugInfoLinesEntry* lines;
} SwampDebugInfoLines;



typedef struct SwampDebugInfoFiles{
    uint32_t count;
    const char** filenames;
} SwampDebugInfoFiles;


const SwampDebugInfoLinesEntry* swampDebugInfoFindLinesDebugLines(const struct SwampDebugInfoLines* lines, uint16_t opcodePosition);
const SwampDebugInfoLinesEntry* swampDebugInfoFindLinesCallstack(const struct SwampCallStack* callStack);
int swampDebugInfoFindLinesInContextToString(const struct SwampMachineContext* machineContext, const char** outString);
int swampDebugInfoFindLinesInContextToStringSingleLine(const struct SwampMachineContext* machineContext, const char** outString);
int swampDebugInfoFilesFindFile(const SwampDebugInfoFiles* files, uint16_t fileIndex, const char** outFilename);



#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_DEBUG_H

