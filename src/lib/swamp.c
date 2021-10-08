/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/context.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/opcodes.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/types.h>

#include <clog/clog.h>
#include <monotonic-time/monotonic_time.h>
#include <string.h> // memset
#include <swamp-dump/dump_ascii.h>
#include <swamp-typeinfo/chunk.h>

static const char* g_swamp_opcode_names[] = {
    "nop",  "cse",   "brfa", "brt",   "jmp",   "call",    "ret",    "ecall",   "tail",  "curry",  "addi",      "subi",
    "muli", "divi",  "negi", "mulfx", "divfx", "cpeli",   "cpnei",  "cpli",    "cplei", "cpgi",   "cpgei",     "noti",
    "cpes", "cpnes", "andi", "ori",   "xori",  "noti",    "crlst",  "crarr",   "conjl", "addlst", "appendstr", "ldi",
    "ldb",  "ldr",   "ldz",  "cpy",   "lde",   "callvar", "cmpeeq", "cmpeneq", "jmppi", "jmpps", "callvaralign"};

static const char* swamp_opcode_name(uint8_t opcode)
{
    return g_swamp_opcode_names[opcode];
}
#define SWAMP_CONFIG_DEBUG 0

typedef uint16_t SwampJump;
typedef uint8_t SwampJumpOffset;

// -------------------------------------------------------------
// Stack
// -------------------------------------------------------------

typedef struct SwampCallStackEntry {
    const uint8_t* pc;
    const uint8_t* basePointer;
    const SwampFunc* func;
    MonotonicTimeNanoseconds debugBeforeTimeNs;
} SwampCallStackEntry;

#define MAX_CALL_STACK_COUNT (24)

typedef struct SwampCallStack {
    SwampCallStackEntry entries[MAX_CALL_STACK_COUNT];
    size_t count;
} SwampCallStack;

#define DEBUGLOG_PARAMS 0

#define SWAMP_INLINE __attribute__ ((__always_inline__))

SWAMP_INLINE uint32_t readU32(const uint8_t** pc)
{
    const uint32_t* p = (const uint32_t*) *pc;

    *pc += 4;
#if DEBUGLOG_PARAMS
    SWAMP_LOG_DEBUG("read uint32 %d", *p);
#endif
    return *p;
}

SWAMP_INLINE uint16_t readU16(const uint8_t** pc)
{
    const uint16_t* p = (const uint16_t*) *pc;

    *pc += 2;
#if DEBUGLOG_PARAMS
    SWAMP_LOG_DEBUG("read uint16 %d", *p);
#endif
    return *p;
}

SWAMP_INLINE uint8_t readU8(const uint8_t** pc)
{
    const uint8_t* p = (const uint8_t*) *pc;

    *pc += 1;
#if DEBUGLOG_PARAMS
    SWAMP_LOG_DEBUG("read uint8 %d", *p);
#endif
    return *p;
}

SWAMP_INLINE size_t readShortRange(const uint8_t** pc)
{
    return readU16(pc);
}

SWAMP_INLINE size_t readAlign(const uint8_t** pc)
{
    return readU8(pc);
}

SWAMP_INLINE size_t readCount(const uint8_t** pc)
{
    return readU16(pc);
}

SWAMP_INLINE SwampJump readJump(const uint8_t** pc)
{
    return readU16(pc);
}

SWAMP_INLINE SwampJumpOffset readJumpOffset(const uint8_t** pc)
{
    return readU16(pc);
}

SWAMP_INLINE size_t readShortCount(const uint8_t** pc)
{
    return readU8(pc);
}

SWAMP_INLINE SwampInt32 readInt32(const uint8_t** pc)
{
    return (SwampInt32) readU32(pc);
}

SWAMP_INLINE SwampBool readBool(const uint8_t** pc)
{
    return (SwampBool) readU8(pc);
}

SWAMP_INLINE void* readStackPointerPos(const uint8_t** pc, const uint8_t* bp)
{
    return (void*) (bp + readU32(pc));
}

SWAMP_INLINE void* readTargetStackPointerPos(const uint8_t** pc, const uint8_t* bp)
{
    return readStackPointerPos(pc, bp);
}

SWAMP_INLINE SwampInt32* readTargetStackIntPointerPos(const uint8_t** pc, const uint8_t* bp)
{
    return (SwampInt32*) readTargetStackPointerPos(pc, bp);
}

SWAMP_INLINE const void* readSourceStackPointerPos(const uint8_t** pc, const uint8_t* bp)
{
    return readStackPointerPos(pc, bp);
}

SWAMP_INLINE const void* readSourceStaticMemoryPointerPos(const uint8_t** pc, const SwampStaticMemory* staticMemory)
{
    return swampStaticMemoryGet(staticMemory, readU32(pc));
}

SWAMP_INLINE SwampInt32 readSourceIntStackPointerPos(const uint8_t** pc, const uint8_t* bp)
{
    return *((const SwampInt32*) readSourceStackPointerPos(pc, bp));
}

#define GET_OPERATOR_INT()                                                                                             \
    SwampInt32* targetRegister = readTargetStackIntPointerPos(&pc, bp);                                                \
    SwampInt32 a = readSourceIntStackPointerPos(&pc, bp);                                                              \
    SwampInt32 b = readSourceIntStackPointerPos(&pc, bp);

#define GET_UNARY_OPERATOR_INT()                                                                                       \
    SwampInt32* targetRegister = readTargetStackIntPointerPos(&pc, bp);                                                \
    SwampInt32 a = readSourceIntStackPointerPos(&pc, bp);

#define SET_OPERATOR_RESULT_INT(intValue) *targetRegister = intValue;
#define SET_OPERATOR_RESULT_BOOL(boolValue) *((SwampBool*) targetRegister) = boolValue;

#define swampMemoryCopy(target, source, size) tc_memcpy_octets((void*) target, source, size)

#define swampMemoryMove(target, source, size) tc_memmove_octets((void*) target, source, size)

#define SWAMP_RUN_MEASURE_PERFORMANCE (0)

int swampRun(SwampResult* result, SwampMachineContext* context, const SwampFunc* f, SwampParameters runParameters,
             SwampBool verbose_flag)
{
    const uint8_t* pc = f->opcodes;
    const uint8_t* bp = context->bp;

    SwampCallStack temp_stack;
    temp_stack.count = 0;
    SwampCallStack* stack = &temp_stack;
    SwampCallStackEntry* call_stack_entry = &stack->entries[0];
    call_stack_entry->func = f;
    call_stack_entry->pc = pc;

    //CLOG_VERBOSE("SAVE start '%s' pc:%p bp:%p", f->debugName, pc, bp)

    if (runParameters.parameterCount != f->parameterCount) {
        // ERROR
        SWAMP_LOG_INFO("mismatch! param count %zu vs function says %zu (%p, %s)", runParameters.parameterCount,
                       f->parameterCount, (const void*) f, f->debugName);
        // swamp_value_print((const swamp_value*) f, "swamp_run()");
        return 0;
    }

    if (result->expectedOctetSize != f->returnOctetSize) {
        SWAMP_LOG_SOFT_ERROR("expected result %zu, but function returns %zu", result->expectedOctetSize,
                             f->returnOctetSize);
        return -2;
    }

#if SWAMP_CALL_DEBUG
    CLOG_INFO("call '%s' %d", f->debugName, f->parameterCount);
    char temp[8*1024];
    const SwtiFunctionType* fnType = swtiChunkTypeFromIndex(context->typeInfo, f->typeIndex);
    SwampMemoryPosition pos = 0;
    pos += f->returnOctetSize;

    for (size_t i=0; i<fnType->parameterCount-1; ++i) {
        const SwtiType* paramType = fnType->parameterTypes[i];
        size_t align = swtiGetMemoryAlign(paramType);
        swampMemoryPositionAlign(&pos, align);

        CLOG_INFO("  param %d %s", i, swampDumpToAsciiString(bp + pos, paramType, 0, temp, 8*1024));
        pos += swtiGetMemorySize(paramType);
    }

#endif

#if SWAMP_RUN_MEASURE_PERFORMANCE
    call_stack_entry->debugBeforeTimeNs = monotonicTimeNanosecondsNow();
#endif
    while (1) {
#if SWAMP_CONFIG_DEBUG || DEBUGLOG_PARAMS
        if (verbose_flag || 1) {
            uint16_t addr = pc - call_stack_entry->func->opcodes;
            if (pc == 0) {
                SWAMP_LOG_SOFT_ERROR("pc is null");
            }
            //            SWAMP_LOG_INFO("--- %04X  [0x%02x]", addr, *pc);
            SWAMP_LOG_INFO("--- %04X %s [0x%02x]", addr, swamp_opcode_name(*pc), *pc);
        }
#endif

        switch (*pc++) {

            case SwampOpcodeReturn: {
#if SWAMP_RUN_MEASURE_PERFORMANCE
                MonotonicTimeNanoseconds after = monotonicTimeNanosecondsNow();
                MonotonicTimeNanoseconds timeSpent = after - call_stack_entry->debugBeforeTimeNs;
                CLOG_INFO("time spent in func %s:%lu", call_stack_entry->func->debugName, timeSpent);
#endif
                if (stack->count == 0) {
                    if (verbose_flag) {
                       // CLOG_VERBOSE("swampRun(%s) is complete", call_stack_entry->func->debugName);
                    }
                    if (result->expectedOctetSize != f->returnOctetSize) {
                        SWAMP_LOG_SOFT_ERROR("expected result %zu, but function returns %zu", result->expectedOctetSize,
                                             f->returnOctetSize);
                        return -3;
                    }

                    return 0;
                }

                call_stack_entry = &stack->entries[--stack->count];
                pc = call_stack_entry->pc;
                bp = call_stack_entry->basePointer;
                if (verbose_flag) {
                   // CLOG_VERBOSE("POP back to '%s' pc: %p bp: %d", call_stack_entry->func->debugName, pc,
                     //            bp - context->stackMemory.memory);
                }
                // context = &call_stack_entry->context;
            } break;

            case SwampOpcodeLoadZeroMemory: {
                void** target = readTargetStackPointerPos(&pc, bp);
                *target = readSourceStaticMemoryPointerPos(&pc, context->constantStaticMemory);
            } break;

            case SwampOpcodeLoadInteger: {
                SwampInt32* target = ((SwampInt32*) readTargetStackPointerPos(&pc, bp));
                *target = readInt32(&pc);
            } break;

            case SwampOpcodeLoadBoolean: {
                SwampBool* target = ((SwampBool*) readTargetStackPointerPos(&pc, bp));
                *target = readBool(&pc);
                ;
            } break;

            case SwampOpcodeLoadRune: {
                SwampCharacter* target = ((SwampCharacter*) readTargetStackPointerPos(&pc, bp));
                *target = readU8(&pc);
            } break;

            case SwampOpcodeMemCopy: {
                uint8_t* target = ((uint8_t*) readTargetStackPointerPos(&pc, bp));
                const uint8_t* source = ((const uint8_t*) readSourceStackPointerPos(&pc, bp));
                uint16_t range = readShortRange(&pc);
                tc_memcpy_octets(target, source, range);
            } break;

            case SwampOpcodeSetEnum: {
                uint8_t* target = ((uint8_t*) readTargetStackPointerPos(&pc, bp));
                *target = readU8(&pc);
            } break;

            case SwampOpcodeListConj: {
                SwampListReference* target = (SwampListReference*) readTargetStackPointerPos(&pc, bp);
                const SwampListReference sourceList = (const SwampListReference) readSourceStackPointerPos(&pc, bp);
                const void* sourceItem = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);

                SwampList* newList = swampDynamicMemoryAlloc(context->dynamicMemory, 1, sizeof(SwampList), 8);
                void* dynamicItemMemory = swampDynamicMemoryAlloc(context->dynamicMemory, 1, range, sourceList->itemAlign);
                swampMemoryCopy(dynamicItemMemory, sourceItem, range);
                newList->value = dynamicItemMemory;
                newList->count = sourceList->count + 1;
                newList->itemAlign = sourceList->itemAlign;
                newList->itemSize = sourceList->itemSize;

                *target = newList;
            } break;

            case SwampOpcodeListAppend: {
                SwampListReferenceData target = (SwampListReferenceData) readTargetStackPointerPos(&pc, bp);
                const SwampListReference sourceListA = *(const SwampListReferenceData) readSourceStackPointerPos(&pc, bp);
                const SwampListReference sourceListB = *(const SwampListReferenceData) readSourceStackPointerPos(&pc, bp);
                *target = swampAllocateListAppendNoCopy(context->dynamicMemory, sourceListA, sourceListB);
            } break;

            case SwampOpcodeCallExternalWithSizes: {
                const uint8_t* basePointer = readSourceStackPointerPos(&pc, bp);
                const SwampFunctionExternal* externalFunction = *(
                    (const SwampFunctionExternal**) readStackPointerPos(&pc, bp));
                uint8_t count = readShortCount(&pc);
                const void* params[8];
                for (uint8_t i = 0; i < count; i++) {
                    uint16_t offset = readU16(&pc);
                    uint16_t size = readU16(&pc);
                    params[i] = basePointer + offset;
                }
                switch (count - 1) { // externalfunction->paramCOunt
                    case 0:
                        externalFunction->function0(basePointer, context);
                        break;
                    case 1:
                        externalFunction->function1(basePointer, context, params[1]);
                        break;
                    case 2:
                        externalFunction->function2(basePointer, context, params[1], params[2]);
                        break;
                    case 3:
                        externalFunction->function3(basePointer, context, params[1], params[2], params[3]);
                        break;
                    case 4:
                        externalFunction->function4(basePointer, context, params[1], params[2], params[3], params[4]);
                        break;
                    default:
                        SWAMP_LOG_ERROR("strange parameter count in external with sizes");
                }
            } break;
            case SwampOpcodeCallExternalWithExtendedSizes: {
                const uint8_t* basePointer = readSourceStackPointerPos(&pc, bp);
                const SwampFunctionExternal* externalFunction = *(
                    (const SwampFunctionExternal**) readStackPointerPos(&pc, bp));
                uint8_t count = readShortCount(&pc);
                SwampUnknownType unknownTypes[8];
                for (uint8_t i = 0; i < count; i++) {
                    uint16_t offset = readU16(&pc);
                    uint16_t size = readU16(&pc);
                    uint8_t align = readU8(&pc);
                    unknownTypes[i].ptr = basePointer + offset;
                    unknownTypes[i].size = size;
                    unknownTypes[i].align = align;
                }
                switch (count - 1) { // externalfunction->paramCOunt
                    case 1:
                        externalFunction->function1(basePointer, context, &unknownTypes[1]);
                        break;
                    case 2:
                        externalFunction->function2(basePointer, context, &unknownTypes[1], &unknownTypes[2]);
                        break;
                    case 3:
                        externalFunction->function3(basePointer, context, &unknownTypes[1], &unknownTypes[2], &unknownTypes[3]);
                        break;
                    case 4:
                        externalFunction->function4(basePointer, context, &unknownTypes[1], &unknownTypes[2], &unknownTypes[3], &unknownTypes[4]);
                        break;
                    default:
                        SWAMP_LOG_ERROR("strange parameter count in external with sizes");
                }
            } break;
            case SwampOpcodeCall:
            case SwampOpcodeCallExternal: {
                const uint8_t* basePointer = readSourceStackPointerPos(&pc, bp);
                const SwampFunc* func = *((const SwampFunc**) readStackPointerPos(&pc, bp));

                if (func->func.type == SwampFunctionTypeCurry) {
                    //CLOG_VERBOSE("SwampFunctionTypeCurry pc:%p bp:%p", pc, bp)
                    const SwampCurryFunc* curry = (const SwampCurryFunc*) func;
                    swampMemoryMove((basePointer + curry->curryOctetSize), basePointer, func->parametersOctetSize);
                    swampMemoryCopy(basePointer, curry->curryOctets, curry->curryOctetSize);
                    func = curry->curryFunction;
                }



                if (func->func.type == SwampFunctionTypeExternal) {
#if SWAMP_RUN_MEASURE_PERFORMANCE
                    MonotonicTimeNanoseconds beforeTimeNs = monotonicTimeNanosecondsNow();
#endif
                    const SwampFunctionExternal* externalFunction = (const SwampFunctionExternal*) func;
                   // CLOG_VERBOSE("Callexternal '%s' pc:%p bp:%d", externalFunction->fullyQualifiedName, pc,
                     //            bp - context->stackMemory.memory)
                    switch (func->parameterCount) {
                        case 0:
                            externalFunction->function0(basePointer, context);
                            break;
                        case 1:
                            externalFunction->function1(basePointer, context,
                                                        basePointer + externalFunction->parameters[0].pos);
                            break;
                        case 2:
                            externalFunction->function2(basePointer, context,
                                                        basePointer + externalFunction->parameters[0].pos,
                                                        basePointer + externalFunction->parameters[1].pos);
                            break;
                        case 3:
                            externalFunction->function3(basePointer, context,
                                                        basePointer + externalFunction->parameters[0].pos,
                                                        basePointer + externalFunction->parameters[1].pos,
                                                        basePointer + externalFunction->parameters[2].pos);
                            break;
                        case 4:
                            externalFunction->function4(basePointer, context,
                                                        basePointer + externalFunction->parameters[0].pos,
                                                        basePointer + externalFunction->parameters[1].pos,
                                                        basePointer + externalFunction->parameters[2].pos,
                                                        basePointer + externalFunction->parameters[3].pos);
                            break;
                        default:
                            SWAMP_LOG_ERROR("strange parameter count in external");
                    }
#if SWAMP_RUN_MEASURE_PERFORMANCE
                    MonotonicTimeNanoseconds afterTimeNs = monotonicTimeNanosecondsNow();
                    CLOG_INFO("external '%s' %lu ns", externalFunction->fullyQualifiedName, afterTimeNs - beforeTimeNs);
#endif
                } else {
#if SWAMP_CONFIG_DEBUG
                    if (stack->count >= 31) {
                        SWAMP_LOG_ERROR("stack push overrun %zu", stack->count);
                    }
#endif

                    // Save current state
                    call_stack_entry->pc = pc;
                    call_stack_entry->basePointer = bp;

                    // Set new stack entry
                    call_stack_entry = &stack->entries[++stack->count];
                    call_stack_entry->func = func;
                    call_stack_entry->pc = func->opcodes;
                    call_stack_entry->basePointer = basePointer;
                    // CLOG_VERBOSE("PUSH pc:%p bp:%04X", pc, bp - context->stackMemory.memory)

                    // Set variables
                    bp = basePointer;
                    pc = func->opcodes;
                    //CLOG_VERBOSE("Call '%s' pc:%p bp:%04X", func->debugName, pc, bp - context->stackMemory.memory)
#if SWAMP_RUN_MEASURE_PERFORMANCE
                    call_stack_entry->debugBeforeTimeNs = monotonicTimeNanosecondsNow();
#endif
                }
            } break;

            case SwampOpcodeCurry: {
                SwampFunc** targetFunc = (SwampFunc**) readTargetStackPointerPos(&pc, bp);
                uint16_t typeIdIndex = readU16(&pc);
                uint8_t align = readU8(&pc);
                const SwampFunc* sourceFunc = *(const SwampFunc**) readSourceStackPointerPos(&pc, bp);
                const void* argumentsStartPointer = readSourceStackPointerPos(&pc, bp);
                size_t argumentsRange = readShortRange(&pc);
                *targetFunc = swampCurryFuncAllocate(context->dynamicMemory, typeIdIndex, align, sourceFunc, argumentsStartPointer,
                                                     argumentsRange);
            } break;

            case SwampOpcodeEnumCase: {
                const void* source = readSourceStackPointerPos(&pc, bp);
                uint8_t caseCount = *pc++;
                const uint8_t* jumpPcToUse = 0;
                uint8_t sourceUnionType = *((const uint8_t*) source);
                const uint8_t* previous_jump_target_pc = 0;
                for (uint8_t caseIndex = 0; caseIndex < caseCount; ++caseIndex) {
                    uint8_t enum_type = *pc++;
                    SwampJumpOffset rel_jump_case = readJumpOffset(&pc);

                    if (!previous_jump_target_pc) {
                        previous_jump_target_pc = pc;
                    }
                    previous_jump_target_pc += rel_jump_case;
#if 0
                    uint16_t absolute_jump_addr = previous_jump_target_pc - call_stack_entry->pc;
                    CLOG_INFO("enum:%d addr: %04X relative:%d", enum_type, absolute_jump_addr, rel_jump_case);
#endif

                    if (enum_type == sourceUnionType || enum_type == 0xff) {
                        jumpPcToUse = previous_jump_target_pc;
                        uint8_t remainingCases = caseCount - caseIndex - 1;
                        pc += (remainingCases * (1 + 2));
                    }
                }
                if (jumpPcToUse == 0) {
                    CLOG_ERROR("could not find matching enum %d", sourceUnionType)
                }
                pc = jumpPcToUse;
            } break;

            case SwampOpcodePatternMatchingInt: {
                SwampInt32 integerToMatch = readSourceIntStackPointerPos(&pc, bp);
                size_t consequenceCount = readShortCount(&pc);
                int found = 0;
                const uint8_t* jumpToUse = 0;
                const uint8_t* previousJumpTarget = 0;
                for (uint8_t consequenceIndex = 0; consequenceIndex < consequenceCount; ++consequenceIndex) {
                    SwampInt32 consequenceValue = readInt32(&pc);
                    SwampJumpOffset jumpOffset = readJumpOffset(&pc);

                    if (!previousJumpTarget) {
                        previousJumpTarget = pc;
                    }
                    previousJumpTarget += jumpOffset;

                    if (integerToMatch == consequenceValue) {
                        jumpToUse = previousJumpTarget;
                        uint8_t rest = consequenceCount - consequenceIndex - 1;
                        pc += (rest * (4 + 2)) + 2;
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    SwampJumpOffset jumpOffset = readJumpOffset(&pc);
                    previousJumpTarget += jumpOffset;
                    jumpToUse = previousJumpTarget;
                }
                pc = jumpToUse;
            } break;

            case SwampOpcodeListCreate: {
                SwampListReferenceData listReferenceTarget = (SwampListReferenceData) readTargetStackPointerPos(&pc,
                                                                                                                bp);
                size_t itemSize = readShortRange(&pc);
                size_t itemAlign = readAlign(&pc);
                size_t itemCount = readShortCount(&pc);
                void* targetItems = swampDynamicMemoryAlloc(context->dynamicMemory, itemCount, itemSize, itemAlign);
                uint8_t* pItems = targetItems;
                for (size_t i = 0; i < itemCount; ++i) {
                    const void* item = readSourceStackPointerPos(&pc, bp);
                    swampMemoryCopy(pItems, item, itemSize);
                    pItems += itemSize;
                }
                const SwampList* list = swampListAllocateNoCopy(context->dynamicMemory, targetItems, itemCount,
                                                                itemSize, itemAlign);
                *listReferenceTarget = list;
            } break;

            case SwampOpcodeArrayCreate: {
                SwampArrayReferenceData arrayTarget = (SwampArrayReferenceData) readTargetStackPointerPos(&pc, bp);
                size_t itemSize = readShortRange(&pc);
                size_t itemAlign = readAlign(&pc);
                size_t itemCount = readShortCount(&pc);
                void* targetItems = swampDynamicMemoryAlloc(context->dynamicMemory, itemCount, itemSize, itemAlign);
                uint8_t* pItems = targetItems;
                for (size_t i = 0; i < itemCount; ++i) {
                    const void* item = readSourceStackPointerPos(&pc, bp);
                    swampMemoryCopy(pItems, item, itemSize);
                    pItems += itemSize;
                }
                SwampArray* newArray = (SwampArray*) swampDynamicMemoryAlloc(context->dynamicMemory, 1,
                                                                             sizeof(SwampArray), 8);
                newArray->value = targetItems;
                newArray->count = itemCount;
                newArray->itemSize = itemSize;
                newArray->itemAlign = itemAlign;
                *arrayTarget = newArray;
            } break;

            case SwampOpcodeStringAppend: {
                const SwampStringReferenceData target = (SwampStringReferenceData) readTargetStackPointerPos(&pc, bp);
                const SwampStringReference sourceStringA = *(
                    (const SwampStringReferenceData) readSourceStackPointerPos(&pc, bp));
                const SwampStringReference sourceStringB = *(
                    (const SwampStringReferenceData) readSourceStackPointerPos(&pc, bp));
                size_t totalCharacterCount = sourceStringA->characterCount + sourceStringB->characterCount;
                char* newCharacters = swampDynamicMemoryAlloc(context->dynamicMemory, 1, totalCharacterCount + 1, 1);
                tc_memcpy_octets(newCharacters, sourceStringA->characters, sourceStringA->characterCount);
                tc_memcpy_octets(newCharacters + sourceStringA->characterCount, sourceStringB->characters,
                                 sourceStringB->characterCount + 1);
                SwampString* newString = swampDynamicMemoryAlloc(context->dynamicMemory, 1, sizeof(SwampString), 8);
                newString->characterCount = totalCharacterCount;
                newString->characters = newCharacters;
                *target = newString;
            } break;

            case SwampOpcodeJump: {
                SwampJump jump = readJump(&pc);
                pc += jump;
            } break;

            case SwampOpcodeBranchFalse: {
                SwampBool truth = *((SwampBool*) readSourceStackPointerPos(&pc, bp));
                SwampJump jump = readJump(&pc);
                if (!truth) {
                    pc += jump;
                }
            } break;

            case SwampOpcodeBranchTrue: {
                SwampBool truth = *((SwampBool*) readSourceStackPointerPos(&pc, bp));
                SwampJump jump = readJump(&pc);
                if (truth) {
                    pc += jump;
                }
            } break;

            case SwampOpcodeStringEqual: {
                SwampBool* target = (SwampBool*) readTargetStackPointerPos(&pc, bp);
                const SwampString* a = *((SwampString**) readSourceStackPointerPos(&pc, bp));
                const SwampString* b = *((SwampString**) readSourceStackPointerPos(&pc, bp));
                *target = swampStringEqual(a, b);
            } break;

            case SwampOpcodeStringNotEqual: {
                SwampBool* target = (SwampBool*) readTargetStackPointerPos(&pc, bp);
                const SwampString* a = *((SwampString**) readSourceStackPointerPos(&pc, bp));
                const SwampString* b = *((SwampString**) readSourceStackPointerPos(&pc, bp));
                *target = !swampStringEqual(a, b);
            } break;

            case SwampOpcodeCmpEnumEqual: {
                SwampBool* targetRegister = readTargetStackPointerPos(&pc, bp);
                const uint8_t* a = readSourceStackPointerPos(&pc, bp);
                const uint8_t* b = readSourceStackPointerPos(&pc, bp);
                *targetRegister = *a == *b;
            } break;
            case SwampOpcodeCmpEnumNotEqual: {
                SwampBool* targetRegister = readTargetStackPointerPos(&pc, bp);
                const uint8_t* a = readSourceStackPointerPos(&pc, bp);
                const uint8_t* b = readSourceStackPointerPos(&pc, bp);
                *targetRegister = *a != *b;
            } break;

            case SwampOpcodeTailCall: {
                // Arguments are not altered during the function execution
                // so just set the PC.
                pc = call_stack_entry->func->opcodes;

            } break;

            case SwampOpcodeIntAdd: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a + b);
            } break;
            case SwampOpcodeIntSub: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a - b);
            } break;
            case SwampOpcodeIntDiv: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a / b);
            } break;
            case SwampOpcodeIntMul: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * b);
            } break;
            case SwampOpcodeFixedDiv: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * SWAMP_FIXED_FACTOR / b);
            } break;
            case SwampOpcodeFixedMul: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * b / SWAMP_FIXED_FACTOR);
            } break;
            case SwampOpcodeIntGreater: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a > b);
            } break;
            case SwampOpcodeIntGreaterOrEqual: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a >= b);
            } break;
            case SwampOpcodeIntLess: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a < b);
            } break;
            case SwampOpcodeIntLessEqual: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a <= b);
            } break;
            case SwampOpcodeIntEqual: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a == b);
            } break;
            case SwampOpcodeIntNotEqual: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a != b);
            } break;
            case SwampOpcodeIntAnd: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a & b);
            } break;

            case SwampOpcodeIntOr: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a | b);
            } break;
            case SwampOpcodeIntXor: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a ^ b);
            } break;
            case SwampOpcodeIntShiftLeft: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a << b);
            } break;
            case SwampOpcodeIntShiftRight: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a >> b);
            } break;
            case SwampOpcodeIntMod: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a % b);
            } break;

                // UNARY
            case SwampOpcodeIntNot: {
                GET_UNARY_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(~a);
            } break;
            case SwampOpcodeIntNegate: {
                GET_UNARY_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(-a);
            } break;
            case SwampOpcodeBoolNot: {
                SwampBool* target = (SwampBool*) readTargetStackPointerPos(&pc, bp);
                SwampBool a = *((SwampBool*) readSourceStackPointerPos(&pc, bp));
                *target = !a;
            } break;
            default:
                SWAMP_LOG_INFO("Unknown opcode: %02x", *(pc - 1));
                return 0;
        }
    }
}
