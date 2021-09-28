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
#include <swamp-runtime/core/core.h>
#include <swamp-runtime/context.h>
#include <swamp-ecs-wrap/bind.h>

clog_config g_clog;

void* swampExampleFindFunction(const char* name)
{
    void *fn;

    fn = swampCoreFindFunction(name);
    if (fn) {
        return fn;
    }

    fn = ecsWrapFindFunction(name);

    return fn;
}


int main(int argc, char* argv[])
{
    g_clog.log = clog_console;


    SWAMP_LOG_INFO("Swampfunction:%d SwampFunc:%d offset opcodes:%d parametersOctetSize: %d size_t:%d  enum:%d", sizeof(SwampFunction), sizeof(SwampFunc), offsetof(SwampFunc, opcodes),  offsetof(SwampFunc, parameterCount), sizeof (size_t), sizeof(SwampFunction));
    SWAMP_LOG_INFO("Swampexternalfunction: SwampFunctionExternal %d fullyQualifiedName:%d return:%d paramsCount:%d params7:%d", sizeof(SwampFunctionExternal), offsetof(SwampFunctionExternal, fullyQualifiedName), offsetof(SwampFunctionExternal, returnValue), offsetof(SwampFunctionExternal, parameterCount), offsetof(SwampFunctionExternal, parameters[7]));


    SwampUnpack unpack;
    swampUnpackInit(&unpack, 1);


    int unpackErr = swampUnpackFilename(&unpack, "gameplay.swamp-pack", swampExampleFindFunction, 1);
    if (unpackErr < 0) {
        //SWAMP_ERROR("couldn't unpack %d", unpackErr)
        //return unpackErr;
    }

    const SwampFunc* initFunc = swampLedgerFindFunction(&unpack.ledger, "init");
    if (initFunc == 0) {
        CLOG_ERROR("could not find 'init'-function");
    }
    const SwampFunc* func = swampLedgerFindFunction(&unpack.ledger, "main"); //unpack.entry;

    SwampMachineContext context;
    context.dynamicMemory = tc_malloc_type_count(SwampDynamicMemory, 1);
    context.dynamicMemory->maxAllocatedSize = 64 * 1024;
    context.dynamicMemory->memory = malloc(context.dynamicMemory->maxAllocatedSize);
    context.dynamicMemory->p = context.dynamicMemory->memory;

    context.stackMemory.maximumStackMemory = 32* 1024;
    context.stackMemory.memory = malloc(context.stackMemory.maximumStackMemory);
    context.bp = context.stackMemory.memory;
    context.tempResult = malloc(2 * 1024);
    context.typeInfo = &unpack.typeInfoChunk;

    SwampStaticMemory staticMemory;
    swampStaticMemoryInit(&staticMemory, (uint8_t*) unpack.constantStaticMemoryOctets, unpack.constantStaticMemoryMaxSize);
    context.constantStaticMemory = &staticMemory;
    typedef struct Position {
        int32_t x;
        int32_t y;
    } Position;

    SwampResult result;
    result.expectedOctetSize = 4; // sizeof(Position);
    result.target = 0;

    SwampBool temp;
    SwampParameters parameters;
    parameters.octetSize = sizeof(SwampBool);
    parameters.parameterCount = 1;
    tc_memcpy_octets(context.bp + result.expectedOctetSize, &temp, sizeof(SwampBool));

    int worked = swampRun(&result, &context, func, parameters, 1);
    if (worked < 0) {
        return worked;
    }
    Position *v = ((Position *) context.stackMemory.memory);
    CLOG_INFO("result is: %d %d", v->x, v->y);

    swampContextDestroy(&context);
    swampUnpackFree(&unpack);
    free(context.dynamicMemory->memory);
    free(context.dynamicMemory);

    return 0;
}
