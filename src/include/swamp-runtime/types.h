/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_types_h
#define swamp_types_h

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct SwampMachineContext;

typedef int SwampBool;

#define SwampTrue (1)
#define SwampFalse (0)

#define SWAMP_FIXED_FACTOR (1000)

#define SWAMP_ERROR(...)                                                                                               \
    SWAMP_LOG_INFO(__VA_ARGS__);                                                                                       \
    abort();

#define SWAMP_ASSERT(expr, ...)                                                                                        \
    if (!(expr)) {                                                                                                     \
        SWAMP_LOG_INFO(__VA_ARGS__);                                                                                   \
        abort();                                                                                                       \
    }


/// BASIC INTERNALS
typedef int32_t SwampInt32;
typedef SwampInt32 SwampCharacter;
typedef int32_t SwampFixed32;
typedef uint32_t SwampResourceNameId;


typedef struct SwampParameters {
    void* source;
    size_t octetSize;
    size_t parameterCount;
} SwampParameters;

typedef struct SwampResult {
    void* target;
    size_t expectedOctetSize;
} SwampResult;

#define SWAMP_FIXED32_TO_FLOAT(v) (v / (float) SWAMP_FIXED_FACTOR)
#define SWAMP_INT_FIXED_TO_FLOAT(v) SWAMP_FIXED32_TO_FLOAT((v)->value);


typedef struct SwampList {
    const struct SwampList* next;
    size_t count;
    const void* value;
} SwampList;

typedef const SwampList* SwampListReference;
typedef const SwampList** SwampListReferenceData;

typedef struct SwampString {
    const char* characters;
    size_t characterCount;
} SwampString;

int swampStringEqual(const SwampString* a, const SwampString* b);

typedef const SwampString* SwampStringReference;
typedef const SwampString** SwampStringReferenceData;


typedef struct SwampBlob {
    const uint8_t* octets;
    size_t octetCount;
} SwampBlob;

typedef struct SwampArray {
    const void* value;
    size_t count;
    size_t itemSize;
} SwampArray;

typedef SwampArray* SwampArrayReference;
typedef SwampArray** SwampArrayReferenceData;

typedef enum SwampFunctionType {
    SwampFunctionTypeInternal,
    SwampFunctionTypeExternal,
    SwampFunctionTypeCurry
} SwampFunctionType;

typedef struct SwampFunction {
    SwampFunctionType type;
} SwampFunction;

typedef struct SwampFunc {
    SwampFunction func;
    size_t parameterCount;
    size_t parametersOctetSize;
    const uint8_t* opcodes;
    size_t opcodeCount;
    size_t returnOctetSize;
    const char* debugName;
    uint16_t typeIndex;
} SwampFunc;

typedef struct SwampCurryFunc {
    SwampFunction func;
    size_t curryOctetSize;
    const uint8_t* curryOctets;
    const struct SwampFunc* curryFunction;
} SwampCurryFunc;

typedef void (*SwampExternalFunction1)(void* result, struct SwampMachineContext* context, const void* argument1);
typedef void (*SwampExternalFunction2)(void* result, struct SwampMachineContext* context, const void* argument1, const void* argument2);
typedef void (*SwampExternalFunction3)(void* result, struct SwampMachineContext* context, const void* argument1, const void* argument2, const void* argument3);
typedef void (*SwampExternalFunction4)(void* result, struct SwampMachineContext* context, const void* argument1, const void* argument2, const void* argument3, const void* argument4);



typedef struct SwampFunctionExternalPosRange {
    uint32_t pos;
    uint32_t range;
} SwampFunctionExternalPosRange;

typedef struct SwampFunctionExternal {
    SwampFunction func;
    size_t parameterCount;
    SwampFunctionExternalPosRange returnValue;
    SwampFunctionExternalPosRange parameters[8];
    SwampExternalFunction1 function1;
    SwampExternalFunction2 function2;
    SwampExternalFunction3 function3;
    SwampExternalFunction4 function4;
    const char* fullyQualifiedName;
} SwampFunctionExternal;

struct SwampMachineContext;

#endif
