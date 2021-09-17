/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/opcodes.h>
#include <swamp-runtime/swamp.h>
#include <clog/clog.h>
#include <clog/console.h>
#include <swamp-runtime/swamp_unpack.h>
#include <swamp-runtime/log.h>
#include <unistd.h>
clog_config g_clog;

typedef struct SwampConstantLedgerEntry {
    uint32_t constantType;
    uint32_t offset;
} SwampConstantLedgerEntry;

#define LedgerTypeFunc (3)
#define LedgerTypeString (1)

void logMemory(const uint8_t* octets, size_t count) {
    const uint8_t* p = octets;
    for (size_t i=0; i<count; ++i) {
        uint8_t data = *p;
        CLOG_INFO("%d %02X", i, data);
        p++;
    }
}


const SwampFunc* checkLedger(const uint8_t* const dynamicMemoryOctets, const SwampConstantLedgerEntry* entries) {
    const SwampFunc* entryFunc = 0;
    const SwampConstantLedgerEntry* entry = entries;
    while (entry->constantType != 0) {
        CLOG_INFO("ledger: %d %d", entry->constantType, entry->offset);
        const uint8_t* p = (dynamicMemoryOctets + entry->offset);
        switch (entry->constantType) {
            case LedgerTypeFunc: {
                SwampFunc* func = (const SwampFunc*)p;
                CLOG_INFO("func: opcode count %d", func->opcodeCount)
                CLOG_INFO("func: opcode offset %d", func->opcodes)
                func->opcodes = (dynamicMemoryOctets + (uintptr_t)func->opcodes);
                CLOG_INFO("func: first opcode: %02X", *func->opcodes);
                entryFunc = func;
            } break;
            case LedgerTypeString: {
                SwampString* str = (const SwampString *)p;
                CLOG_INFO("str: character count %d", str->characterCount)
                str->characters = (dynamicMemoryOctets + (uintptr_t)str->characters);
                CLOG_INFO("str: characters: %s", str->characters);
            } break;
        }
        entry++;
    }

    return entryFunc;
}



int main(int argc, char* argv[])
{
    g_clog.log = clog_console;


    SwampUnpack unpack;
    swampUnpackInit(&unpack, 1);

    char buf[1024];
    if (getcwd(buf, 1024) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    } else {
       SWAMP_LOG_INFO("pwd: %s", buf);
        }
    int unpackErr = swampUnpackFilename(&unpack, "out.swamp-pack", 1);
    if (unpackErr < 0) {
        SWAMP_ERROR("couldn't unpack %d", unpackErr)
        return unpackErr;
    }

    const SwampFunc* func = checkLedger( unpack.dynamicMemoryOctets, (SwampConstantLedgerEntry*) unpack.ledgerOctets);

    SwampMachineContext context;
    context.dynamicMemory.memory = unpack.dynamicMemoryOctets;
    context.dynamicMemory.maxAllocatedSize = unpack.dynamicMemoryMaxSize;
    context.stackMemory.memory = malloc(32 * 1024);
    context.stackMemory.maximumStackMemory = 32* 1024;
    context.bp = context.stackMemory.memory;
    context.sp = context.stackMemory.memory;

    SwampResult result;
    result.expectedOctetSize = 0; // sizeof(SwampStringReference);
    result.target = 0;

    SwampParameters parameters;
    parameters.octetSize = 0;
    parameters.parameterCount = 0;

    int worked = swampRun(&context, func, parameters, &result, 1);
#if 0
    const SwampListReference resultList = (SwampListReference) result.target;
    const SwampInt32* resultIntegerInList = (SwampInt32*) resultList->value;
    CLOG_INFO("result in list: %d", *resultIntegerInList);
#else
    SwampBool *v = ((SwampBool *) context.stackMemory.memory);
    CLOG_INFO("result is: %d", *v);
    //SwampString* v = *((SwampString **) context.stackMemory.memory);
    //CLOG_INFO("result is: %s", v->characters);
#endif

    swampUnpackFree(&unpack);

    return 0;
}
