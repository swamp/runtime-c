/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/opcodes.h>
#include <swamp-runtime/swamp.h>
#include <clog/clog.h>
#include <clog/console.h>

clog_config g_clog;

int main(int argc, char* argv[])
{
    g_clog.log = clog_console;

    uint8_t opcodes[] = {
        swamp_opcode_mem_cpy_zero_page,
        8,0,0,0, // target
        0,0,0,0, // source
        4,0,
        swamp_opcode_int_add,
        12,0,0,0,
        4,0,0,0,
        8,0,0,0,
        swamp_opcode_mem_cpy,
        0,0,0,0,
        12,0,0,0,
        4,0,
        swamp_opcode_return
    };

    SwampFunc func;
    func.debugName = "something";
    func.opcodeCount = 14;
    func.opcodes = opcodes;
    func.parameterCount = 2;
    func.parametersOctetSize = 8;
    func.totalStackUsed = 128;
    func.typeIndex = 0;
    func.returnOctetSize = 4;

#define STACK_MEMORY_SIZE (128)
    uint8_t* stackMemory = calloc(1, STACK_MEMORY_SIZE);
    uint8_t* sp = stackMemory;

    *((SwampInt32*)sp)  = -49;
    sp += 4;

    SwampMachineContext context;
    swampStackMemoryInit(&context.stackMemory, stackMemory, STACK_MEMORY_SIZE);
    context.bp = sp;
    context.sp = sp;

#define DYNAMIC_MEMORY_SIZE (32*1024)
    uint8_t* dynamicMemory = calloc(1, DYNAMIC_MEMORY_SIZE);
    swampDynamicMemoryInit(&context.dynamicMemory, dynamicMemory, DYNAMIC_MEMORY_SIZE);


    uint8_t parameterOctets[4+4];
    uint8_t* parameterPointer = parameterOctets;

    *((SwampInt32*) parameterPointer) = -4;
    parameterPointer += sizeof(SwampInt32);

    *((SwampInt32*) parameterPointer) = 18;
    //parameterPointer += sizeof(SwampInt32);

    SwampParameters parameters;
    parameters.parameterCount = 2;
    parameters.octetSize = 8;
    parameters.source = parameterOctets;

    SwampResult result;
    result.expectedOctetSize = 4;

    int worked = swampRun(&context, &func, parameters, &result, 0);
    CLOG_INFO("result: %d", *((SwampInt32*) result.target));

    free(context.stackMemory.memory);
    free(context.dynamicMemory.memory);

    return worked;
}
