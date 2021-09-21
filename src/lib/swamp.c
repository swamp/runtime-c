/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/log.h>
#include <swamp-runtime/opcodes.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/types.h>
#include <swamp-runtime/swamp_allocate.h>

#include <clog/clog.h>
#include <string.h> // memset


static const char* g_swamp_opcode_names[] = {
    "nop",       "cse",     "brfa",   "brt",  "jmp",  "call", "ret",         "ecall",
    "tail", "curry", "addi",       "subi",   "muli",  "divi",  "negi", "mulfx",  "divfx",          "cpeli",   "cpnei",   "cpli",
    "cplei", "cpgi", "cpgei",   "noti", "cpes", "cpnes", "andi", "ori", "xori", "noti",
    "crlst",   "crarr",   "conjl", "addlst", "appendstr", "ldi", "ldb", "ldr", "ldz", "cpy", "lde", "callvar"};

static const char* swamp_opcode_name(uint8_t opcode)
{
    return g_swamp_opcode_names[opcode];
}
#define SWAMP_CONFIG_DEBUG 1


// -------------------------------------------------------------
// Stack
// -------------------------------------------------------------

typedef struct SwampCallStackEntry {
    const uint8_t* pc;
    const uint8_t* basePointer;
    const SwampFunc* func;
} SwampCallStackEntry;

#define MAX_CALL_STACK_COUNT (24)

typedef struct SwampCallStack {
    SwampCallStackEntry entries[MAX_CALL_STACK_COUNT];
    size_t count;
} SwampCallStack;

#define READ_U32(variable) \
    variable = *((uint32_t*) pc); \
    pc += 4;


static uint32_t readU32(const uint8_t **pc) {
    const uint32_t* p = (const uint32_t*) *pc;

    *pc += 4;

    //SWAMP_LOG_DEBUG("read uint32 %d", *p);
    return *p;
}

static uint16_t readU16(const uint8_t **pc) {
    const uint16_t* p = (const uint16_t*) *pc;

    *pc += 2;

    //SWAMP_LOG_DEBUG("read uint16 %d", *p);
    return *p;
}

static uint8_t readU8(const uint8_t **pc) {
    const uint8_t* p = (const uint8_t*) *pc;

    *pc += 1;

    //SWAMP_LOG_DEBUG("read uint8 %d", *p);
    return *p;
}

static size_t readShortRange(const uint8_t **pc) {
    return readU16(pc);
}

static size_t readAlign(const uint8_t **pc) {
    return readU8(pc);
}

static size_t readCount(const uint8_t **pc) {
    return readU16(pc);
}

static size_t readShortCount(const uint8_t **pc) {
    return readU8(pc);
}

static SwampInt32 readInt32(const uint8_t **pc) {
    return (SwampInt32) readU32(pc);
}

static SwampBool readBool(const uint8_t **pc) {
    return (SwampBool) readU8(pc);
}

static void* readStackPointerPos(const uint8_t **pc, const uint8_t* bp)
{
    return (void*)(bp + readU32(pc));
}

static void* readStackPointerZeroPagePos(const uint8_t **pc, const uint8_t* stackMemory)
{
    return (void*)(stackMemory + readU32(pc));
}

static void* readTargetStackPointerPos(const uint8_t **pc, const uint8_t* bp)
{
    return readStackPointerPos(pc, bp);
}

static SwampInt32* readTargetStackIntPointerPos(const uint8_t **pc, const uint8_t* bp)
{
    return (SwampInt32*) readTargetStackPointerPos(pc, bp);
}

static const void* readSourceStackPointerPos(const uint8_t **pc, const uint8_t* bp)
{
    return readStackPointerPos(pc, bp);
}

static void* readDynamicPointerPos(const uint8_t **pc, const uint8_t* bp)
{
    return (void*)(bp + readU32(pc));
}

static const void* readSourceDynamicMemoryPointerPos(const uint8_t **pc, const uint8_t* bp)
{
    return readDynamicPointerPos(pc, bp);
}

static SwampInt32 readSourceIntStackPointerPos(const uint8_t **pc, const uint8_t* bp)
{
    return *((const SwampInt32*) readSourceStackPointerPos(pc, bp));
}

#define GET_OPERATOR_INT()                                                                                             \
    SwampInt32* targetRegister = readTargetStackIntPointerPos(&pc, bp);                                                                                   \
    SwampInt32 a = readSourceIntStackPointerPos(&pc, bp);                                                                                            \
    SwampInt32 b = readSourceIntStackPointerPos(&pc, bp);

#define GET_UNARY_OPERATOR_INT()                                                                                       \
    SwampInt32* targetRegister = readTargetStackIntPointerPos(&pc, bp); \
    SwampInt32 a = readSourceIntStackPointerPos(&pc, bp);


#define SET_OPERATOR_RESULT_INT(intValue) *targetRegister = intValue;
#define SET_OPERATOR_RESULT_BOOL(boolValue) *((SwampBool*) targetRegister) = boolValue;

#define swampMemoryCopy(target, source, size) tc_memcpy_octets((void*)target, source, size)

#define swampMemoryMove(target, source, size) tc_memmove_octets((void*)target, source, size)

static void pushOnStackPointer(const uint8_t** sp, const void* x, size_t octetSize)
{
    swampMemoryCopy(*sp, x, octetSize);
    *sp += octetSize;
}


int swampRun(SwampResult* result, SwampMachineContext* context, const SwampFunc* f, SwampParameters runParameters,
             SwampBool verbose_flag)
{
    const uint8_t* pc = f->opcodes;
    const uint8_t* bp = context->bp;
//    const uint8_t* sp = context->sp;

    SwampCallStack temp_stack;
    temp_stack.count = 0;
    SwampCallStack* stack = &temp_stack;
    SwampCallStackEntry* call_stack_entry = &stack->entries[0];
    call_stack_entry->func = f;
    call_stack_entry->pc = pc;

    CLOG_VERBOSE("SAVE start pc:%p bp:%p", pc, bp)

    if (runParameters.parameterCount != f->parameterCount) {
        // ERROR
        SWAMP_LOG_INFO("mismatch! param count %zu vs function says %zu (%p, %s)",
                       runParameters.parameterCount, f->parameterCount,
                       (const void*) f, f->debugName);
       // swamp_value_print((const swamp_value*) f, "swamp_run()");
        return 0;
    }

    if (result->expectedOctetSize != f->returnOctetSize) {
        SWAMP_LOG_SOFT_ERROR("expected result %zu, but function returns %zu", result->expectedOctetSize, f->returnOctetSize);
        return -2;
    }

    pushOnStackPointer(&bp, runParameters.source, runParameters.octetSize);

    while (1) {
#if SWAMP_CONFIG_DEBUG || 1
        if (verbose_flag) {
            uint16_t addr = pc - call_stack_entry->pc;
            if (pc == 0) {
                SWAMP_LOG_SOFT_ERROR("pc is null");
            }
            SWAMP_LOG_INFO("--- %04X  [0x%02x]", addr, *pc);
            SWAMP_LOG_INFO("--- %04X  %s [0x%02x]", addr, swamp_opcode_name(*pc), *pc);
        }
#endif

        switch (*pc++) {

            case SwampOpcodeReturn: {
                if (stack->count == 0) {
                    CLOG_VERBOSE("we are done")
                    if (result->expectedOctetSize != f->returnOctetSize) {
                        SWAMP_LOG_SOFT_ERROR("expected result %zu, but function returns %zu", result->expectedOctetSize, f->returnOctetSize);
                        return -3;
                    }

                    result->target = (void*) bp;
                    return 0;
                }

                call_stack_entry = &stack->entries[--stack->count];
                pc = call_stack_entry->pc;
                bp = call_stack_entry->basePointer;
                CLOG_VERBOSE("POP pc: %p bp: %d", pc, bp - context->stackMemory.memory);
                //context = &call_stack_entry->context;
            } break;

            case SwampOpcodeLoadZeroMemory: {
                void** target = readTargetStackPointerPos(&pc, bp);
                void* constantSource = readSourceDynamicMemoryPointerPos(&pc, context->dynamicMemory.memory);
                const SwampString* constantString = (const SwampString*) constantSource;
                *target = constantSource;
            } break;

            case SwampOpcodeLoadInteger: {
                SwampInt32* target = ((SwampInt32*) readTargetStackPointerPos(&pc, bp));
                *target = readInt32(&pc);
            } break;

            case SwampOpcodeLoadBoolean: {
                SwampBool* target = ((SwampBool *) readTargetStackPointerPos(&pc, bp));
                *target = readBool(&pc);;
            } break;

            case SwampOpcodeLoadRune: {
                SwampCharacter* target = ((SwampCharacter *) readTargetStackPointerPos(&pc, bp));
                *target = readU8(&pc);
            } break;

            case SwampOpcodeMemCopy: {
                uint8_t* target = ((uint8_t *) readTargetStackPointerPos(&pc, bp));
                const uint8_t* source = ((const uint8_t *) readSourceStackPointerPos(&pc, bp));
                uint16_t range = readShortRange(&pc);
                tc_memcpy_octets(target, source, range);
            } break;

            case SwampOpcodeSetEnum: {
                uint8_t* target = ((uint8_t *) readTargetStackPointerPos(&pc, bp));
                *target = readU8(&pc);
            } break;

            case SwampOpcodeListConj: {
                SwampListReference* target = (SwampListReference*) readTargetStackPointerPos(&pc, bp);
                const SwampListReference sourceList = (const SwampListReference) readSourceStackPointerPos(&pc, bp);
                const void* sourceItem = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);

                SwampList* newList = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, sizeof(SwampList));
                void* dynamicItemMemory = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, range);
                swampMemoryCopy(dynamicItemMemory, sourceItem, range);
                newList->value = dynamicItemMemory;
                newList->count = sourceList->count+1;

                *target = newList;
            } break;

            case SwampOpcodeListAppend: {
                SwampListReferenceData target = (SwampListReferenceData) readTargetStackPointerPos(&pc, bp);
                const SwampListReference sourceListA = (const SwampListReference) readSourceStackPointerPos(&pc, bp);
                const SwampListReference sourceListB = (const SwampListReference) readSourceStackPointerPos(&pc, bp);
                *target = swampAllocateListAppendNoCopy(&context->dynamicMemory, sourceListA, sourceListB);
            } break;

            case SwampOpcodeCallExternalWithSizes: {
                const uint8_t* basePointer = readSourceStackPointerPos(&pc, bp);
                const SwampFunctionExternal* externalFunction = *((const SwampFunctionExternal **) readStackPointerPos(&pc, bp));
                uint8_t count = readShortCount(&pc);
                const void* params[8];
                for (uint8_t i=0; i<count;i++) {
                    uint16_t offset = readU16(&pc);
                    uint16_t size = readU16(&pc);
                    params[i] = basePointer + offset;
                }
                switch (externalFunction->parameterCount) {
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
            case SwampOpcodeCall:
            case SwampOpcodeCallExternal: {
                const uint8_t* basePointer = readSourceStackPointerPos(&pc, bp);
                const SwampFunc* func = *((const SwampFunc**) readStackPointerPos(&pc, bp));

                if (func->func.type == SwampFunctionTypeCurry) {
                    CLOG_VERBOSE("SwampFunctionTypeCurry pc:%p bp:%p", pc, bp)
                    const SwampCurryFunc* curry = (const SwampCurryFunc*) func;
                    swampMemoryMove((basePointer + curry->curryOctetSize), basePointer, func->parametersOctetSize);
                    swampMemoryCopy(basePointer, curry->curryOctets, curry->curryOctetSize);
                    func = curry->curryFunction;
                }

                if (func->func.type == SwampFunctionTypeExternal) {
                    CLOG_VERBOSE("Callexternal pc:%p bp:%p", pc, bp)
                    const SwampFunctionExternal* externalFunction = (const SwampFunctionExternal*) func;
                    switch (func->parameterCount) {
                        case 1:
                            externalFunction->function1(basePointer, context, basePointer + externalFunction->parameters[0].pos);
                            break;
                        case 2:
                            externalFunction->function2(basePointer, context, basePointer + externalFunction->parameters[0].pos, basePointer + externalFunction->parameters[1].pos);
                            break;
                        case 3:
                            externalFunction->function3(basePointer, context, basePointer + externalFunction->parameters[0].pos, basePointer + externalFunction->parameters[1].pos, basePointer + externalFunction->parameters[2].pos);
                            break;
                        case 4:
                            externalFunction->function4(basePointer, context, basePointer + externalFunction->parameters[0].pos, basePointer + externalFunction->parameters[1].pos, basePointer + externalFunction->parameters[2].pos, basePointer + externalFunction->parameters[3].pos);
                            break;
                        default:
                            SWAMP_LOG_ERROR("strange parameter count in external");
                    }
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
                    CLOG_VERBOSE("PUSH pc:%p bp:%d", pc, bp - context->stackMemory.memory)

                    // Set variables
                    bp = basePointer;
                    pc = func->opcodes;
                    CLOG_VERBOSE("Call pc:%p bp:%d", pc, bp - context->stackMemory.memory)
                }
            } break;

            case SwampOpcodeCurry: {
                SwampFunc** targetFunc = (SwampFunc**) readTargetStackPointerPos(&pc, bp);
                const SwampFunc* sourceFunc = (const SwampFunc*) readSourceStackPointerPos(&pc, bp);
                const void* argumentsStartPointer = readSourceStackPointerPos(&pc, bp);
                size_t argumentsRange = readShortRange(&pc);
                *targetFunc = swampCurryFuncAllocate(&context->dynamicMemory, sourceFunc, argumentsStartPointer, argumentsRange);
            } break;



            case SwampOpcodeEnumCase: {
                const void* source = readSourceStackPointerPos(&pc, bp);
                uint8_t case_count = *pc++;
                const uint8_t* jump_to_use = 0;
                int found = 0;
                uint8_t sourceUnionType = *((const uint8_t*)source);
                const uint8_t* previous_jump_target_pc = 0;
                for (uint8_t case_index = 0; case_index < case_count; ++case_index) {
                    uint8_t enum_type = *pc++;
                    uint8_t rel_jump_case = *pc++;
                    if (!previous_jump_target_pc) {
                        previous_jump_target_pc = pc;
                    }
                    previous_jump_target_pc += rel_jump_case;
                    //uint16_t absolute_jump_addr = previous_jump_target_pc - call_stack_entry->pc;

                    if (!found) {
                        if (enum_type == sourceUnionType || enum_type == 0xff) {
                            jump_to_use = previous_jump_target_pc;
                            found = 1;
                        }
                    }
                }
                if (jump_to_use == 0) {
                    CLOG_ERROR("could not find matching enum")
                }
                pc = jump_to_use;
            } break;

                /*
            case SwampOpcodeCasePatternMatching: {
                const void* source = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);
                uint8_t case_count = *pc++;
                int found = 0;
                const uint8_t* jump_to_use = 0;
                const uint8_t* previous_jump_target_pc = 0;
                for (uint8_t case_index = 0; case_index < case_count; ++case_index) {
                    const void* caseSource = readSourceStackPointerPos(&pc, bp);
                    uint8_t rel_jump_case = *pc++;

                    if (!previous_jump_target_pc) {
                        previous_jump_target_pc = pc;
                    }
                    previous_jump_target_pc += rel_jump_case;
                    // uint16_t absolute_jump_addr = previous_jump_target_pc - call_stack_entry->pc;

                    if (swampMemoryCompareEqual(source, caseSource, range)) {
                        jump_to_use = previous_jump_target_pc;
                        uint8_t rest = case_count - case_index - 1;
                        pc += rest * 2;
                        found = 1;
                        break;
                    }
                }
#if SWAMP_CONFIG_DEBUG
                if (!found) {
                    // ERROR
                    SWAMP_LOG_INFO("enum error! No case matching 0x%p. This should "
                                   "not be "
                                   "possible.", source);
                    return 0;
                }
#endif
                pc = jump_to_use;
            } break;
*/
            case SwampOpcodeListCreate: {
                SwampListReferenceData listReferenceTarget = (SwampListReferenceData) readTargetStackPointerPos(&pc, bp);
                size_t itemSize = readShortRange(&pc);
                size_t itemAlign = readAlign(&pc);
                size_t itemCount = readShortCount(&pc);
                void* targetItems = swampDynamicMemoryAlloc(&context->dynamicMemory, itemSize, itemCount);
                uint8_t* pItems = targetItems;
                for (size_t i = 0; i < itemCount; ++i) {
                    const void* item = readSourceStackPointerPos(&pc, bp);
                    swampMemoryCopy(pItems, item, itemSize);
                    pItems += itemSize;
                }
                const SwampList* list = swampListAllocateNoCopy(&context->dynamicMemory, targetItems, itemCount, itemSize, itemAlign);
                *listReferenceTarget = list;
            } break;

            case SwampOpcodeArrayCreate: {
                SwampArrayReferenceData arrayTarget = (SwampArrayReferenceData) readTargetStackPointerPos(&pc, bp);
                size_t itemCount = readCount(&pc);
                size_t itemSize = readShortRange(&pc);
                void* targetItems = swampDynamicMemoryAlloc(&context->dynamicMemory, itemSize, itemCount);
                uint8_t* pItems = targetItems;
                for (size_t i = 0; i < itemCount; ++i) {
                    const void* item = readSourceStackPointerPos(&pc, bp);
                    swampMemoryCopy(pItems, item, itemSize);
                    pItems += itemSize;
                }
                SwampArray* newArray = (SwampArray*) swampDynamicMemoryAlloc(&context->dynamicMemory, 1, sizeof(SwampArray));
                newArray->value = targetItems;
                newArray->count = itemCount;
                newArray->itemSize = itemSize;
                *arrayTarget = newArray;
            } break;


            case SwampOpcodeStringAppend: {
                const SwampStringReferenceData target = (SwampStringReferenceData) readTargetStackPointerPos(&pc, bp);
                const SwampStringReference sourceStringA = *((const SwampStringReferenceData) readSourceStackPointerPos(&pc, bp));
                const SwampStringReference sourceStringB = *((const SwampStringReferenceData) readSourceStackPointerPos(&pc, bp));
                size_t totalCharacterCount = sourceStringA->characterCount + sourceStringB->characterCount;
                char* newCharacters = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, totalCharacterCount + 1);
                tc_memcpy_octets(newCharacters, sourceStringA->characters, sourceStringA->characterCount);
                tc_memcpy_octets(newCharacters + sourceStringA->characterCount, sourceStringB->characters, sourceStringB->characterCount+1);
                SwampString* newString = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, sizeof(SwampString));
                newString->characterCount = totalCharacterCount;
                newString->characters = newCharacters;
                *target = newString;
            } break;


            case SwampOpcodeJump: {
                uint8_t jump = *pc++;
                pc += jump;
            } break;

            case SwampOpcodeBranchFalse: {
                SwampBool truth = *((SwampBool*)readSourceStackPointerPos(&pc, bp));
                uint8_t jump = *pc++;
                if (!truth) {
                    pc += jump;
                }
            } break;

            case SwampOpcodeBranchTrue: {
                SwampBool truth = *((SwampBool*)readSourceStackPointerPos(&pc, bp));
                uint8_t jump = *pc++;
                if (truth) {
                    pc += jump;
                }
            } break;

                /*
            case SwampOpcodeCompareEqual: {
                SwampBool* target = (SwampBool*)readTargetStackPointerPos(&pc, bp);
                const void* a = readSourceStackPointerPos(&pc, bp);
                const void* b = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);
                *target = swampMemoryCompareEqual(a, b, range);
            } break;

            case SwampOpcodeCompareNotEqual: {
                SwampBool* target = (SwampBool*)readTargetStackPointerPos(&pc, bp);
                const void* a = readSourceStackPointerPos(&pc, bp);
                const void* b = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);
                *target = !swampMemoryCompareEqual(a, b, range);
            } break;
*/
            case SwampOpcodeStringEqual: {
                SwampBool* target = (SwampBool*)readTargetStackPointerPos(&pc, bp);
                const SwampString* a = *((SwampString**)readSourceStackPointerPos(&pc, bp));
                const SwampString* b = *((SwampString**)readSourceStackPointerPos(&pc, bp));
                *target = swampStringEqual(a, b);
            } break;

            case SwampOpcodeStringNotEqual: {
                SwampBool* target = (SwampBool*)readTargetStackPointerPos(&pc, bp);
                const SwampString* a = *((SwampString**)readSourceStackPointerPos(&pc, bp));
                const SwampString* b = *((SwampString**)readSourceStackPointerPos(&pc, bp));
                *target = !swampStringEqual(a, b);
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
                SwampBool* target = (SwampBool*)readTargetStackPointerPos(&pc, bp);
                SwampBool a = *((SwampBool*)readSourceStackPointerPos(&pc, bp));
                *target = !a;
            } break;
            default:
                SWAMP_LOG_INFO("Unknown opcode: %02x", *(pc - 1));
                return 0;
        }
    }
}
