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
#include <swamp-runtime/log.h>
struct SwampMachineContext;

typedef uint8_t SwampBool;

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
typedef uint32_t SwampMemoryPosition;

struct SwampDynamicMemory;
struct SwampUnmanaged;

void swampMemoryPositionAlign(SwampMemoryPosition* position, size_t align);

typedef size_t (*SwampUnmanagedSerialize)(const void* self, uint8_t* target, size_t maxSize);
typedef size_t (*SwampUnmanagedToString)(const void* self, int flags, char* target, size_t maxSize);

typedef int (*SwampUnmanagedCompact)(struct SwampUnmanaged **original, struct SwampDynamicMemory* memory);
typedef int (*SwampUnmanagedClone)(struct SwampUnmanaged **original, struct SwampDynamicMemory* memory);

typedef struct SwampUnmanaged {
    const void* ptr;
    const char* debugName;
    SwampUnmanagedSerialize serialize;
    SwampUnmanagedToString toString;
    SwampUnmanagedCompact compact;
    SwampUnmanagedClone clone;
} SwampUnmanaged;

typedef struct SwampUnknownType {
    const void* ptr; // Must be first!
    size_t size;
    size_t align;
} SwampUnknownType;

typedef struct SwampParameters {
    size_t octetSize;
    size_t parameterCount;
} SwampParameters;

typedef struct SwampResult {
    size_t expectedOctetSize;
} SwampResult;

#define SWAMP_FIXED32_TO_FLOAT(v) (v / (float) SWAMP_FIXED_FACTOR)
#define SWAMP_INT_FIXED_TO_FLOAT(v) SWAMP_FIXED32_TO_FLOAT((v));




typedef uint8_t SwampMaybe;

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

int swampBlobIsEmpty(const SwampBlob* blob);

typedef struct SwampArray {
    const void* value;
    size_t count;
    size_t itemSize;
    size_t itemAlign;
} SwampArray;

typedef SwampArray* SwampArrayReference;
typedef SwampArray** SwampArrayReferenceData;


typedef SwampArray SwampList;

#define SWAMP_LIST_TYPE_DECLARE(type) \
typedef struct SwampList##type { \
    const type* value; \
    size_t count; \
    size_t itemSize; \
    size_t itemAlign; \
    } SwampList##type;

#define SWAMP_LIST_TYPE(type) SwampList##type

#if _MSC_VER
#define SWAMP_INLINE __forceinline
#else
#define SWAMP_INLINE __attribute__ ((__always_inline__))
#endif


SWAMP_INLINE const void* swampListGetItem(const SwampList* list, size_t index);

typedef const SwampList* SwampListReference;
typedef const SwampList** SwampListReferenceData;

typedef enum SwampFunctionType {
    SwampFunctionTypeInternal,
    SwampFunctionTypeExternal,
    SwampFunctionTypeCurry
} SwampFunctionType;

typedef struct SwampFunction {
    SwampFunctionType type;
} SwampFunction;

struct SwampDebugInfoLines;

typedef struct SwampFunc {
    SwampFunction func;
    size_t parameterCount;
    size_t parametersOctetSize;
    const uint8_t* opcodes;
    size_t opcodeCount;
    size_t returnOctetSize;
    size_t returnAlign;
    const char* debugName;
    uint16_t typeIndex;
    struct SwampDebugInfoLines* debugInfoLines;
    size_t debugInfoLinesOctetCount;
    struct SwampDebugInfoVariables* debugInfoVariables;
    size_t debugInfoVariablesOctetCount;
} SwampFunc;

typedef struct SwampCurryFunc {
    SwampFunction func;
    size_t curryOctetSize;
    const uint8_t* curryOctets;
    const struct SwampFunc* curryFunction;
    uint16_t typeIdIndex;
    uint8_t firstParameterAlign;
} SwampCurryFunc;

typedef void (*SwampExternalFunction0)(void* result, struct SwampMachineContext* context);
typedef void (*SwampExternalFunction1)(void* result, struct SwampMachineContext* context, const void* argument1);
typedef void (*SwampExternalFunction2)(void* result, struct SwampMachineContext* context, const void* argument1, const void* argument2);
typedef void (*SwampExternalFunction3)(void* result, struct SwampMachineContext* context, const void* argument1, const void* argument2, const void* argument3);
typedef void (*SwampExternalFunction4)(void* result, struct SwampMachineContext* context, const void* argument1, const void* argument2, const void* argument3, const void* argument4);
typedef void (*SwampExternalFunction5)(void* result, struct SwampMachineContext* context, const void* argument1,
                                       const void* argument2, const void* argument3, const void* argument4,
                                       const void* argument5);



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
    SwampExternalFunction0 function0;
    SwampExternalFunction5 function5;
} SwampFunctionExternal;

struct SwampMachineContext;

#endif
