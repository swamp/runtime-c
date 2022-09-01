/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/bind.h>

#include "swamp-runtime/execute.h"
#include "swamp-typeinfo/typeinfo.h"
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/maybe.h>
#include <tiny-libc/tiny_libc.h>
#include <swamp-typeinfo/chunk.h>


// withDefault : a -> Maybe a -> a
static void swampCoreMaybeWithDefault(void* result, SwampMachineContext* context, const SwampUnknownType* defaultValue, const SwampMaybe** maybe)
{
    if (swampMaybeIsNothing(*maybe)) {
        tc_memcpy_octets(result, defaultValue->ptr, defaultValue->size);
    } else {
        tc_memcpy_octets(result, swampMaybeJustGetValue(*maybe, defaultValue->align), defaultValue->size);
    }
}

const SwtiFunctionType* swampCoreGetFunctionType(const SwtiChunk* typeInfo, const SwampFunction* fn)
{
    size_t typeIndex;
    if (fn->type == SwampFunctionTypeCurry) {
        const SwampCurryFunc* curry = (const SwampCurryFunc*) fn;
        typeIndex = curry->typeIdIndex;
    } else if (fn->type == SwampFunctionTypeInternal) {
        const SwampFunc* internalFunc = (const SwampFunc*) fn;
        typeIndex = internalFunc->typeIndex;
    } else {
        CLOG_ERROR("Not supported")
    }

    const SwtiType* functionType = swtiChunkTypeFromIndex(typeInfo, typeIndex);
    if (functionType->type != SwtiTypeFunction) {
        CLOG_ERROR("wrong type")
    }

    const SwtiFunctionType* funcType = (const SwtiFunctionType*) functionType;

    return funcType;
}

const SwtiType* swampCoreGetFunctionReturnType(const SwtiChunk* typeInfo, const SwampFunction* fn)
{
    const SwtiFunctionType* funcType = swampCoreGetFunctionType(typeInfo, fn);

    const SwtiType* returnType = funcType->parameterTypes[funcType->parameterCount - 1];

    return returnType;
}

const SwtiType* swampCoreMaybeType(const SwtiType* maybeReturnType)
{
    if (maybeReturnType->type != SwtiTypeCustom) {
        CLOG_ERROR("must be a maybe type to return")
    }

    const SwtiCustomType* customType = (const SwtiCustomType*) maybeReturnType;

#if CONFIGURATION_DEBUG
    if (!tc_str_equal(customType->internal.name, "Maybe")) {
        CLOG_ERROR("must be a maybe type")
    }
#endif

    const SwtiCustomTypeVariant* justType = customType->variantTypes[1];
#if CONFIGURATION_DEBUG
    if (!tc_str_equal(justType->name, "Just")) {
        CLOG_ERROR("must be a maybe type")
    }
#endif

    return justType->fields[0].fieldType;

}


const SwtiType* swampCoreMaybeReturnType(const SwtiChunk* typeInfo, const SwampFunction* fn)
{
    const SwtiType* maybeReturnType = swampCoreGetFunctionReturnType(typeInfo, fn);
    return swampCoreMaybeType(maybeReturnType);
}


// maybe : b -> (a -> b) -> Maybe a -> b
static void swampCoreMaybeMaybe(void* result, SwampMachineContext* context, const SwampUnknownType* defaultValue, const SwampFunction*** _fn, const SwampMaybe*** maybe)
{
    const SwampFunction* fn = **_fn;
    if (swampMaybeIsNothing(*maybe)) {
        tc_memcpy_octets(result, defaultValue->ptr, defaultValue->size);
        return;
    }

    SwampMachineContext newContext;
    swampContextCreateTemp(&newContext, context, "maybeFn");

    const SwampFunc* realSwampFn;
    SwampMemoryPosition pos = swampExecutePrepare(fn, newContext.bp, &realSwampFn);
    const SwtiFunctionType* funcType = swampCoreGetFunctionType(context->typeInfo, fn);
    const SwtiType* aType = funcType->parameterTypes[0];
    const SwtiMemoryAlign aAlign = swtiGetMemoryAlign(aType);
    const void* aValue = swampMaybeJustGetValue(*maybe, aAlign);
    tc_memcpy_octets(newContext.bp + pos, aValue, swtiGetMemorySize(aType));
    swampMemoryPositionAlign(&pos, aAlign);

    SwampParameters parameters;
    parameters.octetSize = swtiGetMemorySize(aType);
    parameters.parameterCount = realSwampFn->parameterCount;

    SwampResult fnResult;
    fnResult.expectedOctetSize = realSwampFn->returnOctetSize;

    swampRun(&fnResult, &newContext, realSwampFn, parameters, true);

    tc_memcpy_octets(result, newContext.bp, defaultValue->size);

    swampContextDestroyTemp(&newContext);
}

const void* swampCoreMaybeFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Maybe.withDefault", SWAMP_C_FN(swampCoreMaybeWithDefault)},
        {"Maybe.maybe", SWAMP_C_FN(swampCoreMaybeMaybe)},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
