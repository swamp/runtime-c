#include <swamp-runtime/debug.h>
#include <clog/clog.h>
#include <stddef.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/types.h>
#include <flood/out_stream.h>

void swampDebugInfoLinesOutput(const SwampDebugInfoLines* lines)
{
    CLOG_OUTPUT("debug lines count %d %zu %zu", lines->count, sizeof(SwampDebugInfoLinesEntry), offsetof(SwampDebugInfoLines, lines));
    for (size_t i=0; i<lines->count;++i) {
        const SwampDebugInfoLinesEntry* entry = &lines->lines[i];
        CLOG_OUTPUT(" %d entry %04X file:%d ", i, entry->opcodePosition, entry->sourceFileId);
    }
}





const SwampDebugInfoLinesEntry* swampDebugInfoFindLinesDebugLines(const SwampDebugInfoLines* lines, uint16_t opcodePosition)
{
    const SwampDebugInfoLinesEntry* bestEntry = 0;

    for (size_t i=0; i<lines->count; ++i) {
        const SwampDebugInfoLinesEntry* entry = &lines->lines[i];
        if (entry->opcodePosition > opcodePosition) {
            break;
        }
        if (entry->startLocation.line != 0 && entry->startLocation.column != 0) {
            bestEntry = entry;
        }
    }

    return bestEntry;
}


const SwampDebugInfoLinesEntry* swampDebugInfoFindLinesCallstack(const SwampCallStack* callStack)
{
   const SwampCallStackEntry* entry = &callStack->entries[callStack->count];

   const SwampFunc* func = entry->func;

   uint16_t opcodePosition = entry->pc - func->opcodes;

   return swampDebugInfoFindLinesDebugLines(func->debugInfoLines, opcodePosition);
}

const SwampDebugInfoLinesEntry* swampDebugInfoFindLinesInContext(const SwampMachineContext* machineContext)
{
    return swampDebugInfoFindLinesCallstack(&machineContext->callStack);
}

int swampDebugInfoFilesFindFile(const SwampDebugInfoFiles* files, uint16_t fileIndex, const char** outFilename)
{
    if (fileIndex >= files->count) {
        *outFilename = 0;
        return -2;
    }

    *outFilename = files->filenames[fileIndex];

    return 1;
}

int swampDebugInfoFindLinesInContextToStringSingleLine(const SwampMachineContext* machineContext, const char** outString)
{
    static char temp[32 * 1024];

    FldOutStream stream;
    fldOutStreamInit(&stream, temp, 32*1024);
    *outString = temp;

    const SwampDebugInfoLinesEntry* entry = swampDebugInfoFindLinesInContext(machineContext);
    if (!entry) {
        fldOutStreamWritef(&stream, "no information found");
        fldOutStreamWriteInt8(&stream, 0);
        return -1;
    }

    const char* fileName;

    int fileErr = swampDebugInfoFilesFindFile(machineContext->debugInfoFiles, entry->sourceFileId, &fileName);
    if (fileErr < 0) {
        return fileErr;
    }

    fldOutStreamWritef(&stream, "%s:%d:%d", fileName, entry->startLocation.line+1, entry->startLocation.column+1);
    fldOutStreamWriteInt8(&stream, 0);
}

int swampDebugInfoWriteLineFromContext(FldOutStream* stream, const SwampMachineContext* machineContext)
{
    const SwampDebugInfoLinesEntry* entry = swampDebugInfoFindLinesInContext(machineContext);
    if (!entry) {
        fldOutStreamWritef(stream, "no information found");
        fldOutStreamWriteInt8(stream, 0);
        return -1;
    }

    const char* fileName;

    int fileErr = swampDebugInfoFilesFindFile(machineContext->debugInfoFiles, entry->sourceFileId, &fileName);
    if (fileErr < 0) {
        return fileErr;
    }

    fldOutStreamWritef(stream, "%s:%d:%d", fileName, entry->startLocation.line+1, entry->startLocation.column+1);
    return 1;
}

int swampDebugInfoFindLinesInContextToString(const SwampMachineContext* machineContext, const char** outString)
{
    static char temp[32 * 1024];

    FldOutStream stream;
    fldOutStreamInit(&stream, temp, 32*1024);
    *outString = temp;

    for (const SwampMachineContext* context = machineContext; context; context = context->parent) {
        swampDebugInfoWriteLineFromContext(&stream, context);
        if (context->debugString) {
            fldOutStreamWritef(&stream, "\nengine: %s", context->debugString);
        }
        if (context->parent) {
            fldOutStreamWriteInt8(&stream, '\n');
        }
    }

    fldOutStreamWriteInt8(&stream, 0);

    return stream.pos - 1;
}

