/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/opcodes.h>
#include <swamp-runtime/swamp.h>
#include <clog/clog.h>
#include <clog/console.h>
#include <swamp-runtime/swamp_allocate.h>

clog_config g_clog;

int main(int argc, char* argv[])
{
    g_clog.log = clog_console;

    uint8_t opcodes[] = {
        SwampOpcodeCopyFromZeroMemory,
        8,0,0,0, // target
        12,0,0,0, // source
        sizeof(SwampStringReference),0,
        SwampOpcodeCopyFromZeroMemory,
        16,0,0,0, // target
        20,0,0,0, // source
        8,0,
        SwampOpcodeStringAppend,
        0,0,0,0,
        sizeof(SwampStringReference),0,0,0,
        16,0,0,0,
        SwampOpcodeReturn,
    };

    SwampFunc func;
    func.debugName = "something";
    func.opcodeCount = 14;
    func.opcodes = opcodes;
    func.parameterCount = 2;
    func.parametersOctetSize = 8;
    func.totalStackUsed = 128;
    func.typeIndex = 0;
    func.returnOctetSize =  sizeof(SwampStringReference);


    SwampMachineContext context;

#define DYNAMIC_MEMORY_SIZE (32*1024)
    uint8_t* dynamicMemory = calloc(1, DYNAMIC_MEMORY_SIZE);
    swampDynamicMemoryInit(&context.dynamicMemory, dynamicMemory, DYNAMIC_MEMORY_SIZE);
    const SwampList* emptyList = swampListEmptyAllocate(&context.dynamicMemory);

    const SwampString* helloString = swampStringAllocate(&context.dynamicMemory, "Hello, ");
    const SwampString* worldString = swampStringAllocate(&context.dynamicMemory, "World!");

#define STACK_MEMORY_SIZE (128)
    uint8_t* stackMemory = calloc(1, STACK_MEMORY_SIZE);
    uint8_t* sp = stackMemory;

    *((SwampInt32*)sp)  = -49;
    sp += 4;

    *((SwampListReferenceData)sp) = emptyList;
    sp += sizeof(SwampListReference);

    *((SwampStringReferenceData)sp) = helloString;
    sp += sizeof(SwampStringReference);

    *((SwampStringReferenceData)sp) = worldString;
    sp += sizeof(SwampStringReference);

    swampStackMemoryInit(&context.stackMemory, stackMemory, STACK_MEMORY_SIZE);
    context.bp = sp;
    context.sp = sp;

    CLOG_INFO("sizeof (SwampList) %zu swampListReference* %zu SwampListReference %zu", sizeof(SwampList), sizeof(SwampListReference *), sizeof(SwampListReference));

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
    result.expectedOctetSize = sizeof(SwampStringReference);

    int worked = swampRun(&context, &func, parameters, &result, 0);
#if 0
    const SwampListReference resultList = (SwampListReference) result.target;
    const SwampInt32* resultIntegerInList = (SwampInt32*) resultList->value;
    CLOG_INFO("result in list: %d", *resultIntegerInList);
#else
    const SwampStringReference stringReference = *((SwampStringReferenceData) result.target);
    CLOG_INFO("result string is: '%s' %zu", stringReference->characters, stringReference->characterCount);
#endif

    free(context.stackMemory.memory);
    free(context.dynamicMemory.memory);

    return worked;
}
