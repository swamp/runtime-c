/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/log.h>
#include <swamp-runtime/opcodes.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/types.h>

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
    const uint8_t* bp;
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

#define SET_OPERATOR_RESULT_INT(intValue) *targetRegister = intValue;
#define SET_OPERATOR_RESULT_BOOL(boolValue) *((SwampBool*) targetRegister) = boolValue;

#define swampMemoryCopy(target, source, size) memcpy((void*)target, source, size)

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
                bp = call_stack_entry->bp;
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
                void* sourceItem = readSourceStackPointerPos(&pc, bp);
                size_t range = readShortRange(&pc);

                SwampList* newList = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, sizeof(SwampList));
                void* dynamicItemMemory = swampDynamicMemoryAlloc(&context->dynamicMemory, 1, range);
                swampMemoryCopy(dynamicItemMemory, sourceItem, range);
                newList->value = dynamicItemMemory;
                newList->count = sourceList->count+1;
                newList->next = sourceList;

                *target = newList;
            } break;

            case swamp_opcode_string_append: {
                SwampStringReference* target = (SwampStringReference*) readTargetStackPointerPos(&pc, bp);
                const SwampStringReference sourceStringA = *((const SwampStringReference*) readSourceStackPointerPos(&pc, bp));
                const SwampStringReference sourceStringB = *((const SwampStringReference*) readSourceStackPointerPos(&pc, bp));
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
                void* constantSource = readSourceStackPointerPos(&pc, bp);
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
            default:
                SWAMP_LOG_INFO("Unknown opcode: %02x", *(pc - 1));
                return 0;
        }
    }
}
