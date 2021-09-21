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

clog_config g_clog;

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
    int unpackErr = swampUnpackFilename(&unpack, "first.swamp-pack", swampCoreFindFunction, 1);
    if (unpackErr < 0) {
        SWAMP_ERROR("couldn't unpack %d", unpackErr)
        return unpackErr;
    }

    SWAMP_LOG_INFO("Swampfunction:%d SwampFunc:%d offset opcodes:%d parametersOctetSize: %d size_t:%d  enum:%d", sizeof(SwampFunction), sizeof(SwampFunc), offsetof(SwampFunc, opcodes),  offsetof(SwampFunc, parameterCount), sizeof (size_t), sizeof(SwampFunction));
    SWAMP_LOG_INFO("Swampexternalfunction: SwampFunctionExternal %d parameterCount:%d return:%d params0:%d params7:%d", sizeof(SwampFunctionExternal), offsetof(SwampFunctionExternal, fullyQualifiedName), offsetof(SwampFunctionExternal, returnValue), offsetof(SwampFunctionExternal, parameters[0]), offsetof(SwampFunctionExternal, parameters[7]));


    const SwampFunc* func = unpack.entry;

    SwampMachineContext context;
    swampDynamicMemoryInit(&context.dynamicMemory, (uint8_t*) unpack.dynamicMemoryOctets, unpack.dynamicMemoryMaxSize);
    context.stackMemory.memory = malloc(32 * 1024);
    context.stackMemory.maximumStackMemory = 32* 1024;
    context.bp = context.stackMemory.memory;



    typedef struct Position {
        int32_t x;
        int32_t y;
    } Position;

    SwampResult result;
    result.expectedOctetSize = sizeof(Position);
    result.target = 0;

    SwampParameters parameters;
    parameters.octetSize = 0;
    parameters.parameterCount = 0;

    int worked = swampRun(&result, &context, func, parameters, 1);
#if 0
    const SwampListReference resultList = (SwampListReference) result.target;
    const SwampInt32* resultIntegerInList = (SwampInt32*) resultList->value;
    CLOG_INFO("result in list: %d", *resultIntegerInList);
#else



    Position *v = ((Position *) context.stackMemory.memory);
    CLOG_INFO("result is: %d %d", v->y, v->x);
    //SwampString* v = *((SwampString **) context.stackMemory.memory);
    //CLOG_INFO("result is: %s", v->characters);
#endif

    swampUnpackFree(&unpack);

    return 0;
}
