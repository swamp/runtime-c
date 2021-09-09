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


    uint8_t* stackMemory = calloc(1, 128);
    uint8_t* sp = stackMemory;

    *((SwampInt32*) sp) = -4;
    sp += sizeof(SwampInt32);

    *((SwampInt32*) sp) = 18;
    sp += sizeof(SwampInt32);

    uint8_t opcodes[] = {
        swamp_opcode_int_add,
        8,0,0,0,
        4,0,0,0,
        0,0,0,0,
        swamp_opcode_return
    };

    SwampFunc func;

    func.debugName = "something";
    func.opcodeCount = 14;
    func.opcodes = opcodes;
    func.parameterCount = 0;
    func.parametersOctetSize = 0;
    func.totalStackUsed = 128;
    func.typeIndex = 0;

    SwampMachineContext context;
    context.stackMemory = stackMemory;
    context.maximumStackMemory = 128;
    context.bp = stackMemory;
    context.sp = sp;

    SwampParameters parameters;
    parameters.parameterCount = 0;
    parameters.octetSize = 0;
    parameters.source = 0;

    SwampResult result;

    int worked = swampRun(&context, &func, parameters, &result, 0);
    CLOG_INFO("result: %d", *((SwampInt32*) sp));

    return worked;
}
