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
    "nop",       "crs",     "upd",   "get",  "lr",  "conj", "case",         "brfa",  "jump", "call",
    "ret",       "calle",   "tail",  "add",  "sub", "mul",  "div",          "eql",   "ne",   "less",
    "lessequal", "greater", "gte",   "band", "bor", "bxor", "bnot",         "not",   "bt",   "n/a",
    "n/a",       "n/a",     "curry", "crl",  "lap", "cre",  "stringappend", "fxmul", "fxdiv", "intdiv", "stsp"};

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

    SWAMP_LOG_DEBUG("read uint32 %d", *p);
    return *p;
}

static uint16_t readU16(const uint8_t **pc) {
    const uint16_t* p = (const uint16_t*) *pc;

    *pc += 2;

    SWAMP_LOG_DEBUG("read uint16 %d", *p);
    return *p;
}

static size_t readShortRange(const uint8_t **pc) {
    return readU16(pc);
}

static size_t readStructOffset(const uint8_t **pc) {
    return readU16(pc);
}

static size_t readCount(const uint8_t **pc) {
    return readU16(pc);
}

static size_t readSize(const uint8_t **pc) {
    return readU16(pc);
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

#define swampMemoryCopy(target, source, size) memcpy((void*)target, source, size)

#define swampMemoryMove(target, source, size) memmove((void*)target, source, size)

#define swampMemoryCompareEqual(target, source, size) (memcmp((void*)target, source, size) == 0)

static void pushOnStackPointer(const uint8_t** sp, const void* x, size_t octetSize)
{
    swampMemoryCopy(*sp, x, octetSize);
    *sp += octetSize;
}

int swampRun(SwampMachineContext* context, const SwampFunc* f, SwampParameters runParameters,
             SwampResult* result, SwampBool verbose_flag)
{
    const uint8_t* pc = f->opcodes;
    const uint8_t* bp = context->bp;
    const uint8_t* sp = context->sp;

    SwampCallStack temp_stack;
    temp_stack.count = 0;
    SwampCallStack* stack = &temp_stack;
    SwampCallStackEntry* call_stack_entry = &stack->entries[0];
    call_stack_entry->func = f;
    call_stack_entry->pc = pc;

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

    pushOnStackPointer(&sp, runParameters.source, runParameters.octetSize);

    while (1) {
#if SWAMP_CONFIG_DEBUG
        if (verbose_flag) {
            uint16_t addr = pc - call_stack_entry->pc;
            SWAMP_LOG_INFO("--- %04X  %s 0x%02x", addr, swamp_opcode_name(*pc), *pc);
            //swamp_registers_print(context->registers, 14, "opcode");
        }
#endif
        switch (*pc++) {

            case swamp_opcode_return: {
                uint8_t register_to_return = 0;
                if (stack->count == 0) {
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
                //context = &call_stack_entry->context;
            } break;

            case swamp_opcode_mem_cpy_zero_page: {
                void* target = readTargetStackPointerPos(&pc, bp);
                void* constantSource = readStackPointerZeroPagePos(&pc, context->stackMemory.memory);
                size_t range = readShortRange(&pc);
                swampMemoryCopy(target, constantSource, range);
            } break;
            case swamp_opcode_list_conj: {
                SwampListReference* target = (SwampListReference*) readTargetStackPointerPos(&pc, bp);
                const SwampListReference sourceList = (const SwampListReference) readSourceStackPointerPos(&pc, bp);
                const void* sourceItem = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);

                SwampList* newList = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, sizeof(SwampList));
                void* dynamicItemMemory = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, range);
                swampMemoryCopy(dynamicItemMemory, sourceItem, range);
                newList->value = dynamicItemMemory;
                newList->count = sourceList->count+1;
                newList->next = sourceList;

                *target = newList;
            } break;

            case swamp_opcode_list_append : {
                
            } break;

            case swamp_opcode_call: {
                const SwampFunc* func = (const SwampFunc*) readStackPointerZeroPagePos(&pc, context->stackMemory.memory);
                const void* basePointer = readSourceStackPointerPos(&pc, bp);

                if (func->curryFunction) {
                    swampMemoryMove(basePointer + func->curryOctetSize, basePointer, func->parametersOctetSize);
                    swampMemoryCopy(basePointer, func->curryOctets, func->curryOctetSize);
                    func = func->curryFunction;
                }

                // Store current state
                call_stack_entry->pc = pc;

#if SWAMP_CONFIG_DEBUG
                if (stack->count >= 31) {
                    SWAMP_LOG_ERROR("stack push overrun %zu", stack->count);
                }
#endif

                // Set new stack entry
                call_stack_entry = &stack->entries[++stack->count];
                call_stack_entry->func = func;
                call_stack_entry->pc = func->opcodes;
                call_stack_entry->basePointer = basePointer;

                // Set variables
                pc = call_stack_entry->pc;
            } break;

            case swamp_opcode_curry: {
                SwampFunc** targetFunc = (SwampFunc**) readTargetStackPointerPos(&pc, bp);
                const SwampFunc* sourceFunc = (const SwampFunc*) readStackPointerPos(&pc, bp);
                const void* argumentsStartPointer = readSourceStackPointerPos(&pc, bp);
                size_t argumentsRange = readShortRange(&pc);
                *targetFunc = swampCurryFuncAllocate(&context->dynamicMemory, sourceFunc, argumentsStartPointer, argumentsRange);
            } break;

            case swamp_opcode_call_external: {
                void* target = readTargetStackPointerPos(&pc, bp);
                size_t debugRequiredTargetRange = readShortRange(&pc);
                const SwampFunctionExternal* func = (const SwampFunctionExternal*) readStackPointerZeroPagePos(&pc, context->stackMemory.memory);
                const void* argumentStart = readSourceStackPointerPos(&pc, bp);
                func->externalFunction(context, argumentStart, target, debugRequiredTargetRange);
            } break;

            case swamp_opcode_enum_case: {
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
                pc = jump_to_use;
            } break;

            case swamp_opcode_case_pattern_matching: {
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
            case swamp_opcode_create_list: {
                SwampListReferenceData listReferenceTarget = (SwampListReferenceData) readTargetStackPointerPos(&pc, bp);
                size_t itemCount = readCount(&pc);
                size_t itemSize = readShortRange(&pc);
                void* targetItems = swampDynamicMemoryAlloc(&context->dynamicMemory, itemSize, itemCount);
                uint8_t* pItems = targetItems;
                for (size_t i = 0; i < itemCount; ++i) {
                    const void* item = readSourceStackPointerPos(&pc, bp);
                    swampMemoryCopy(pItems, item, itemSize);
                    pItems += itemSize;
                }
                const SwampList* list = swampListAllocateNoCopy(&context->dynamicMemory, targetItems, itemCount, itemSize);
                *listReferenceTarget = list;
            } break;
            case swamp_opcode_create_array: {
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
            case swamp_opcode_create_struct: {
                void* structTarget = readTargetStackPointerPos(&pc, bp);
                size_t itemCount = readCount(&pc);
                uint8_t* p = (uint8_t*) structTarget;
                for (size_t i = 0; i < itemCount; ++i) {
                    const void* item = readSourceStackPointerPos(&pc, bp);
                    size_t itemSize = readShortRange(&pc);
                    swampMemoryCopy(p, item, itemSize);
                    p += itemSize;
                }
            } break;

            case swamp_opcode_update_struct : {
                uint8_t* structTarget = (uint8_t*) readTargetStackPointerPos(&pc, bp);
                const void* structSource = readSourceStackPointerPos(&pc, bp);
                size_t structSize = readShortRange(&pc);
                swampMemoryCopy(structTarget, structSource, structSize);
                size_t fieldCount = readCount(&pc);
                for (size_t i = 0; i < fieldCount; ++i) {
                    const void* fieldSource = readSourceStackPointerPos(&pc, bp);
                    size_t fieldSize = readShortRange(&pc);
                    size_t targetStructOffset = readStructOffset(&pc);
                    swampMemoryCopy((structTarget + targetStructOffset), fieldSource, fieldSize);
                }
            } break;

            case swamp_opcode_string_append: {
                const SwampStringReferenceData target = (SwampStringReferenceData) readTargetStackPointerPos(&pc, bp);
                const SwampStringReference sourceStringA = *((const SwampStringReferenceData) readSourceStackPointerPos(&pc, bp));
                const SwampStringReference sourceStringB = *((const SwampStringReferenceData) readSourceStackPointerPos(&pc, bp));
                size_t totalCharacterCount = sourceStringA->characterCount + sourceStringB->characterCount;
                char* newCharacters = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, totalCharacterCount + 1);
                memcpy(newCharacters, sourceStringA->characters, sourceStringA->characterCount);
                memcpy(newCharacters + sourceStringA->characterCount, sourceStringB->characters, sourceStringB->characterCount);
                SwampString* newString = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, sizeof(SwampString));
                newString->characterCount = totalCharacterCount;
                newString->characters = newCharacters;
                *target = newString;
            } break;

            case swamp_opcode_mem_cpy: {
                void* target = readTargetStackPointerPos(&pc, bp);
                const void* constantSource = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);
                swampMemoryCopy(target, constantSource, range);
            } break;

            case swamp_opcode_jump: {
                uint8_t jump = *pc++;
                pc += jump;
            } break;

            case swamp_opcode_branch_false: {
                SwampBool truthy = *((SwampBool*)readSourceStackPointerPos(&pc, bp));
                uint8_t jump = *pc++;
                if (!truthy) {
                    pc += jump;
                }
            }

            case swamp_opcode_branch_true: {
                SwampBool truthy = *((SwampBool*)readSourceStackPointerPos(&pc, bp));
                uint8_t jump = *pc++;
                if (truthy) {
                    pc += jump;
                }
            }

            case swamp_opcode_cmp_equal: {
                SwampBool* target = (SwampBool*)readTargetStackPointerPos(&pc, bp);
                const void* a = readSourceStackPointerPos(&pc, bp);
                const void* b = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);
                *target = swampMemoryCompareEqual(a, b, range);
            } break;

            case swamp_opcode_cmp_not_equal: {
                SwampBool* target = (SwampBool*)readTargetStackPointerPos(&pc, bp);
                const void* a = readSourceStackPointerPos(&pc, bp);
                const void* b = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);
                *target = !swampMemoryCompareEqual(a, b, range);
            } break;

            case swamp_opcode_tail_call: {
                // Arguments are not altered during the function execution
                // so just set the PC.
                pc = call_stack_entry->func->opcodes;

            } break;

            case swamp_opcode_int_add: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a + b);
            } break;
            case swamp_opcode_int_sub: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a - b);
            } break;
            case swamp_opcode_int_div: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a / b);
            } break;
            case swamp_opcode_int_mul: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * b);
            } break;
            case swamp_opcode_fixed_div: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * SWAMP_FIXED_FACTOR / b);
            } break;
            case swamp_opcode_fixed_mul: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a * b / SWAMP_FIXED_FACTOR);
            } break;
            case swamp_opcode_int_greater: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a > b);
            } break;
            case swamp_opcode_int_gte: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a >= b);
            } break;
            case swamp_opcode_int_less: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a < b);
            } break;
            case swamp_opcode_int_lessequal: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a <= b);
            } break;
            case swamp_opcode_int_eql: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a == b);
            } break;
            case swamp_opcode_int_neql: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_BOOL(a != b);
            } break;
            case swamp_opcode_int_and: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a & b);
            } break;

            case swamp_opcode_int_or: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a | b);
            } break;
            case swamp_opcode_int_xor: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a ^ b);
            } break;
            case swamp_opcode_int_shl: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a << b);
            } break;
            case swamp_opcode_int_shr: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a >> b);
            } break;
            case swamp_opcode_int_mod: {
                GET_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(a % b);
            } break;

                // UNARY
            case swamp_opcode_int_not: {
                GET_UNARY_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(~a);
            } break;
            case swamp_opcode_int_negate: {
                GET_UNARY_OPERATOR_INT();
                SET_OPERATOR_RESULT_INT(-a);
            } break;
            case swamp_opcode_bool_not: {
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
