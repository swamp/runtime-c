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

typedef const SwampString* SwampStringReference;
typedef const SwampString** SwampStringReferenceData;

typedef struct SwampArray {
    const void* value;
    size_t count;
    size_t itemSize;
} SwampArray;

typedef SwampArray* SwampArrayReference;
typedef SwampArray** SwampArrayReferenceData;

typedef struct SwampFunc {
    size_t curryOctetSize;
    const uint8_t* curryOctets;
    const struct SwampFunc* curryFunction;

    size_t parameterCount;
    size_t parametersOctetSize;
    const uint8_t* opcodes;
    size_t opcodeCount;

    size_t totalStackUsed;
    size_t returnOctetSize;

//    const swamp_value** constants; // or frozen variables in closure
  //  size_t constant_count;
    const char* debugName;
    uint16_t typeIndex;
} SwampFunc;


typedef enum SwampFunctionType {
    SwampFunctionTypeInternal,
    SwampFunctionTypeExternal,
} SwampFunctionType;

typedef struct SwampFunction {
    const SwampFunc* internal_function;
//    const swamp_external_func* external_function;
    SwampFunctionType type;
} swampFunction;

struct SwampMachineContext;

typedef void (*SwampExternalFunction)(struct SwampMachineContext* context, const void* arguments,
                                      void* result, size_t requiredResultSize);

typedef struct SwampFunctionExternal {
    const SwampExternalFunction externalFunction;
} SwampFunctionExternal;

#endif
