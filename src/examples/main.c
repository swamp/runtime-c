/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <clog/console.h>
#include <swamp-dump/dump_ascii.h>
#include <swamp-ecs-wrap/bind.h>
#include <swamp-ecs-wrap/world.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/core.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/opcodes.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/swamp_unpack.h>
#include <swamp-typeinfo/typeinfo.h>
#include <swamp-yaml-load/bind.h>
#include <unistd.h>

clog_config g_clog;


static void fakeCollide(EcsWrapWorldUnmanaged** targetUnmanagedWorld, const SwampMachineContext* context, const void** tilemap, const EcsWrapWorldUnmanaged** sourceWorld) {
    *targetUnmanagedWorld = *sourceWorld;
}

void* swampExampleFindFunction(const char* name)
{
    void* fn;

    fn = swampCoreFindFunction(name);
    if (fn) {
        return fn;
    }

    fn = swampYamlFindFunction(name);
    if (fn) {
        return fn;
    }

    fn = ecsWrapFindFunction(name);
    if (fn) {
        return fn;
    }

    if (tc_str_equal(name, "Turmoil.Collide2.withWorld")) {
        return fakeCollide;
    }

    return 0;
}

/*
type PlayerAction =
None
| Dash
| Up
| Down
| Left
| Right
| Fire


type alias PlayerInput =
{ inputId : Int
 , inputValue : PlayerAction
}
 */

enum HackInputPlayerAction {
    HackInputUp = 2
};
typedef struct TurmoilPlayerInput {
    SwampInt32 inputId;
    uint8_t playerAction;
} TurmoilPlayerInput;

int main(int argc, char* argv[])
{
    g_clog.log = clog_console;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
        return 1;
    }

    SWAMP_LOG_INFO("Swampfunction:%d SwampFunc:%d offset opcodes:%d parametersOctetSize: %d size_t:%d  enum:%d",
                   sizeof(SwampFunction), sizeof(SwampFunc), offsetof(SwampFunc, opcodes),
                   offsetof(SwampFunc, parameterCount), sizeof(size_t), sizeof(SwampFunction));
    SWAMP_LOG_INFO(
        "Swampexternalfunction: SwampFunctionExternal %d fullyQualifiedName:%d return:%d paramsCount:%d params7:%d",
        sizeof(SwampFunctionExternal), offsetof(SwampFunctionExternal, fullyQualifiedName),
        offsetof(SwampFunctionExternal, returnValue), offsetof(SwampFunctionExternal, parameterCount),
        offsetof(SwampFunctionExternal, parameters[7]));

    SwampUnpack unpack;
    swampUnpackInit(&unpack, 1);

    int unpackErr = swampUnpackFilename(&unpack, "gameplay.swamp-pack", swampExampleFindFunction, 1);
    if (unpackErr < 0) {
        SWAMP_ERROR("couldn't unpack %d", unpackErr)
        return unpackErr;
    }

    const SwampFunc* initFunc = swampLedgerFindFunction(&unpack.ledger, "init");
    if (initFunc == 0) {
        CLOG_ERROR("could not find 'init'-function");
    }
    SwampResult initResult;
    initResult.expectedOctetSize = initFunc->returnOctetSize;

    SwampMachineContext initContext;
    initContext.dynamicMemory = tc_malloc_type_count(SwampDynamicMemory, 1);
    initContext.dynamicMemory->maxAllocatedSize = 256 * 1024;
    initContext.dynamicMemory->memory = malloc(initContext.dynamicMemory->maxAllocatedSize);
    initContext.dynamicMemory->p = initContext.dynamicMemory->memory;

    initContext.stackMemory.maximumStackMemory = 32 * 1024;
    initContext.stackMemory.memory = malloc(initContext.stackMemory.maximumStackMemory);
    initContext.bp = initContext.stackMemory.memory;
    initContext.tempResult = malloc(2 * 1024);
    initContext.typeInfo = &unpack.typeInfoChunk;

    SwampStaticMemory staticMemory;
    swampStaticMemoryInit(&staticMemory, (uint8_t*) unpack.constantStaticMemoryOctets,
                          unpack.constantStaticMemoryMaxSize);
    initContext.constantStaticMemory = &staticMemory;

    SwampParameters initParameters;
    initParameters.octetSize = 0;
    initParameters.parameterCount = 0;
    int initWorked = swampRun(&initResult, &initContext, initFunc, initParameters, 1);
    if (initWorked < 0) {
        return initWorked;
    }
    const SwtiType* initFuncType = swtiChunkTypeFromIndex(initContext.typeInfo, initFunc->typeIndex);
    const SwtiFunctionType* initFnType = (const SwtiFunctionType*) initFuncType;
    const SwtiType* initReturnType = initFnType->parameterTypes[initFnType->parameterCount-1];
    char tempStr[32*1024];

    CLOG_INFO("result: %s", swampDumpToAsciiString(initContext.bp, initReturnType, 0, tempStr, 32*1024));

    const SwampFunc* mainFunc = swampLedgerFindFunction(&unpack.ledger, "main"); // unpack.entry;

    SwampMachineContext mainContext;
    /*
    context.dynamicMemory = tc_malloc_type_count(SwampDynamicMemory, 1);
    context.dynamicMemory->maxAllocatedSize = 128 * 1024;
    context.dynamicMemory->memory = malloc(context.dynamicMemory->maxAllocatedSize);
    context.dynamicMemory->p = context.dynamicMemory->memory;
     */
    mainContext.dynamicMemory = initContext.dynamicMemory;

    mainContext.stackMemory.maximumStackMemory = 32 * 1024;
    mainContext.stackMemory.memory = malloc(mainContext.stackMemory.maximumStackMemory);
    mainContext.bp = mainContext.stackMemory.memory;
    mainContext.tempResult = malloc(2 * 1024);
    mainContext.typeInfo = &unpack.typeInfoChunk;

    mainContext.constantStaticMemory = initContext.constantStaticMemory;
    typedef struct Position {
        int32_t x;
        int32_t y;
    } Position;

    SwampResult result;
    result.expectedOctetSize = 48; // sizeof(Position);

    SwampBool temp;
    SwampParameters parameters;

    parameters.parameterCount = 2;

    tc_memset_octets(mainContext.bp, 0xff, mainFunc->returnOctetSize);
    SwampMemoryPosition mainPos = mainFunc->returnOctetSize;

    swampMemoryPositionAlign(&mainPos, initFunc->returnAlign);
    tc_memcpy_octets(mainContext.bp + mainPos, initContext.bp, initFunc->returnOctetSize);
    mainPos += initFunc->returnOctetSize;


    swampMemoryPositionAlign(&mainPos, 8);
    const SwtiType* playerInputType = swtiChunkGetFromName(initContext.typeInfo, "HackInput.PlayerInput");
    SwtiMemorySize playerInputSize = swtiGetMemorySize(playerInputType);
    SwtiMemoryAlign playerInputAlign = swtiGetMemoryAlign(playerInputType);

    SwampList* inputList = swampListAllocatePrepare(mainContext.dynamicMemory, 1, playerInputSize, playerInputAlign);
    // SwampList* inputList = swampListAllocate(mainContext.dynamicMemory, 0, 0, playerInputSize, playerInputAlign);
    TurmoilPlayerInput exampleInput;

    if (sizeof(exampleInput) != playerInputSize) {
        CLOG_ERROR("internal error, sizeof exampleinput");
    }

    exampleInput.inputId = 1;
    exampleInput.playerAction = HackInputUp;

    tc_memcpy_octets(inputList->value + 0, &exampleInput, sizeof(exampleInput));
    *((SwampList**)(mainContext.bp + mainPos)) = inputList;
    mainPos += sizeof(SwampList*);

    parameters.octetSize = mainPos;

    CLOG_INFO("starting MAIN()");
    int worked = swampRun(&result, &mainContext, mainFunc, parameters, 1);
    if (worked < 0) {
        return worked;
    }
    const SwtiType* mainFuncType = swtiChunkTypeFromIndex(initContext.typeInfo, initFunc->typeIndex);
    const SwtiFunctionType* mainFnType = (const SwtiFunctionType*) mainFuncType;
    const SwtiType* mainReturnType = mainFnType->parameterTypes[mainFnType->parameterCount-1];
    char mainTempStr[32*1024];

    CLOG_INFO("main.update result: %s", swampDumpToAsciiString(mainContext.bp, mainReturnType, 0, mainTempStr, 32*1024));

    //swampContextDestroy(&mainContext);
    swampUnpackFree(&unpack);
    free(mainContext.dynamicMemory->memory);
    free(mainContext.dynamicMemory);

    return 0;
}
