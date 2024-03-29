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
#include <stdbool.h>
#include <assert.h>

typedef bool SwampBool;
static_assert(sizeof(bool) == 1, "bools needs to be exactly one octet");

#define SwampTrue (true)
#define SwampFalse (false)

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
struct SwampUnmanagedMemory;
struct SwampUnmanaged;

void swampMemoryPositionAlign(SwampMemoryPosition* position, size_t align);

typedef int (*SwampUnmanagedSerialize)(const void* self, uint8_t* target, size_t maxSize);
typedef int (*SwampUnmanagedDeSerialize)(void* self, const uint8_t* source, size_t maxSize);
typedef int (*SwampUnmanagedToString)(const void* self, int flags, char* target, size_t maxSize);

typedef int (*SwampUnmanagedCompact)(struct SwampUnmanaged **original, struct SwampDynamicMemory* memory, struct SwampUnmanagedMemory* unmanagedMemory);
typedef int (*SwampUnmanagedClone)(struct SwampUnmanaged **original, struct SwampDynamicMemory* memory, struct SwampUnmanagedMemory* unmanagedMemory);
typedef int (*SwampUnmanagedDestroy)(void* self);

#define SWAMP_C_FN(fn) ((const void*)fn)

#define SWAMP_UNMANAGED_PTR(type, unmanaged) (type) ((unmanaged)->ptr)

typedef struct SwampUnmanaged {
    void* ptr; // Must be first
    const char* debugName;
    SwampUnmanagedSerialize serialize;
    SwampUnmanagedDeSerialize deSerialize;
    SwampUnmanagedToString toString;
    SwampUnmanagedCompact compact;
    SwampUnmanagedClone clone;
    SwampUnmanagedDestroy destroy;
} SwampUnmanaged;

int swampUnmanagedVerifyWithName(const SwampUnmanaged* unmanaged, const char* debugName);


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
#define SWAMP_INLINE inline __attribute__ ((__always_inline__))
#endif

#define SWAMP_ENUM uint8_t

#include <clog/clog.h>

SWAMP_INLINE const void* swampListGetItem(const SwampList* list, size_t index)
{
    if (index >= list->count) {
        CLOG_ERROR("index in list doesnt exist")
        return 0;
    }

    return (const uint8_t*) list->value + list->itemSize * index;
}

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
    const struct SwampDebugInfoLines* debugInfoLines;
    size_t debugInfoLinesOctetCount;
    const struct SwampDebugInfoVariables* debugInfoVariables;
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

struct SwampMachineContext;

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
