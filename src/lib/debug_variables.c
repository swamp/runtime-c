#include <swamp-runtime/debug_variables.h>
#include <clog/clog.h>
#include <stddef.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/types.h>
#include <flood/out_stream.h>
#include <swamp-typeinfo/chunk.h>
#include <swamp-typeinfo/typeinfo.h>
#include <swamp-dump/dump_ascii.h>

void swampDebugInfoVariablesOutput(const SwampDebugInfoVariables* variables, const char* description)
{
    CLOG_OUTPUT("%s debug variables count %d", description, variables->count);
    for (size_t i=0; i<variables->count;++i) {
        const SwampDebugInfoVariablesEntry* entry = &variables->variables[i];
        CLOG_OUTPUT(".. %zu '%s' %04X-%04X address:%04X typeId:%d scopeId:%d", i, entry->name, entry->startOpcodePosition, entry->endOpcodePosition, entry->stackPosition, entry->typeId, entry->scopeId);
    }
}


static size_t swampDebugInfoFindVariablesDebugVariables(const SwampDebugInfoVariables* variables, uint16_t opcodePosition, SwampDebugInfoVariablesEntry* target, size_t maxCount)
{
    size_t foundCount = 0;

    for (size_t i=0; i<variables->count; ++i) {
        const SwampDebugInfoVariablesEntry* entry = &variables->variables[i];
        if (opcodePosition > entry->startOpcodePosition && opcodePosition <= entry->endOpcodePosition) {
            if (foundCount >= maxCount) {
                CLOG_SOFT_ERROR("out of buffer")
                return -1;
            }
            target[foundCount++] = *entry;
        }
    }

    return foundCount;
}

static int swampDebugInfoWriteLineFromEntry(FldOutStream* stream, const SwtiChunk* typeInformation, const uint8_t* bp, const SwampDebugInfoVariablesEntry* entry)
{
    const SwtiType* type = swtiChunkTypeFromIndex(typeInformation, entry->typeId);
    if (!type) {
        CLOG_ERROR("can not find type");
        return -2;
    }

    const void* ptr = bp + entry->stackPosition;

    fldOutStreamWritef(stream, "  %s = ", entry->name);

    int dumpErr = swampDumpToAscii(ptr, type, 0, 0, stream);
    if (dumpErr < 0) {
        return dumpErr;
    }

    return dumpErr;
}

int swampDebugInfoVariablesInCallStackEntryToString(const SwampCallStackEntry* callStackEntry, const SwtiChunk* typeInformation, FldOutStream* stream) {

#define MAX_ENTRIES (64)
    static SwampDebugInfoVariablesEntry entries[MAX_ENTRIES];

    const SwampFunc* func = callStackEntry->func;

    uint16_t opcodePosition = callStackEntry->pc - func->opcodes;

    int count = swampDebugInfoFindVariablesDebugVariables(func->debugInfoVariables, opcodePosition, entries, MAX_ENTRIES);
    if (count < 0) {
        return count;
    }

    for (size_t i = 0; i < count; ++i) {
        const SwampDebugInfoVariablesEntry* entry = &entries[i];
        const uint8_t* bp = callStackEntry->basePointer;
        swampDebugInfoWriteLineFromEntry(stream, typeInformation, bp, entry);
        fldOutStreamWritef(stream, "\n");
    }

    return stream->pos - 1;
}

static int swampDebugInfoFindVariablesInCallStackToString(const SwampCallStack* callStack, const SwtiChunk* typeInformation, const char** outString) {
#define TEMP_SIZE (128 * 1024)
    static char temp[TEMP_SIZE];

    FldOutStream stream;
    fldOutStreamInit(&stream, temp, TEMP_SIZE);
    *outString = temp;

    fldOutStreamWritef(&stream, "==== variables ===\n");
    for (int i=callStack->count; i>=0; --i) {
        const SwampCallStackEntry* entry = &callStack->entries[i];
        fldOutStreamWritef(&stream, "stack %d:\n", i);
        int err = swampDebugInfoVariablesInCallStackEntryToString(entry, typeInformation, &stream);
        if (err < 0) {
            return err;
        }
    }

    fldOutStreamWritef(&stream, "---- variables done---\n");

    int endOfStringError = fldOutStreamWriteInt8(&stream, 0);
    if (endOfStringError < 0) {
        return endOfStringError;
    }

    return stream.pos -1;
}

int swampDebugInfoFindVariablesInContextToString(const SwampMachineContext* machineContext, const char** outString)
{
    return swampDebugInfoFindVariablesInCallStackToString(&machineContext->callStack, machineContext->typeInfo, outString);
}

