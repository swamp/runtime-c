/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>

void swampCoreBlobHead(SwampMaybe* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    if (list->count == 0) {
        swampMaybeNothing(result);
    } else {
        swampMaybeJust(result, list->itemAlign, list->value, list->itemSize);
    }
}

#define SwampCollectionIndex(coll, index) (((const uint8_t*) coll->value) + (index) *coll->itemSize)

// tail : List a -> Maybe (List a)
void swampCoreBlobTail(SwampMaybe* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    if (list->count == 0) {
        swampMaybeNothing(result);
    } else {
        swampMaybeJust(result, list->itemAlign, SwampCollectionIndex(list, list->count - 1), list->itemSize);
    }
}

// isEmpty : List a -> Bool
void swampCoreBlobIsEmpty(SwampBool* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    *result = list->count == 0;
}

// length : List a -> Int
void swampCoreBlobLength(SwampInt32* result, SwampMachineContext* context, const SwampList** _list)
{
    *result = (*_list)->count;
}

// map : (a -> b) -> List a -> List b
void swampCoreBlobMap(SwampBlob** result, SwampMachineContext* context, SwampFunc** _fn, const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;
    const SwampFunc* fn = *_fn;
    const uint8_t* sourceItemPointer = blob->octets;

    SwampResult fnResult;
    fnResult.expectedOctetSize = fn->returnOctetSize;

    SwampParameters parameters;
    parameters.parameterCount = 1;
    parameters.octetSize = sizeof(SwampInt32);

    SwampMachineContext ownContext;
    swampContextInit(&ownContext, context->dynamicMemory, context->constantStaticMemory, context->typeInfo);
// , fn->returnOctetSize,
    //                                                 fn->returnAlign
    SwampBlob* target = swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);
    uint8_t* targetItemPointer = target->octets;
    for (size_t i = 0; i < blob->octetCount; ++i) {
        SwampMemoryPosition pos = fn->returnOctetSize;
        SwampInt32 v = *sourceItemPointer;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        swampContextReset(&ownContext);
        tc_memcpy_octets(ownContext.bp + pos, &v, parameters.octetSize);
        CLOG_INFO("calling for index %d", i);
        swampRun(&fnResult, &ownContext, fn, parameters, 1);
        *targetItemPointer = (uint8_t) (*(SwampInt32*) ownContext.bp);
        sourceItemPointer++;
        targetItemPointer++;
    }

    swampContextDestroy(&ownContext);

    *result = target;
}

// map2 : (a -> b -> c) -> List a -> List b -> List c
void swampCoreBlobMap2()
{
}

SwampMemoryPosition swampExecutePrepare(const SwampFunction* func, const void* bp, const SwampFunc** outFn)
{
    if (func->type == SwampFunctionTypeCurry) {
        const SwampCurryFunc* curry = (const SwampCurryFunc*) func;
        const SwampFunc* fn = curry->curryFunction;
        SwampMemoryPosition pos = fn->returnOctetSize;
        swampMemoryPositionAlign(&pos, curry->firstParameterAlign);
        tc_memcpy_octets(bp + pos, curry->curryOctets, curry->curryOctetSize);
        *outFn = curry->curryFunction;
        return pos + curry->curryOctetSize;
    }

    if (func->type == SwampFunctionTypeInternal) {
        const SwampFunc* fn = (const SwampFunc*) func;
        *outFn = fn;
        return fn->returnOctetSize;
    }

    CLOG_ERROR("unknown function");
}

// __externalfn indexedMap : (Int -> Int -> Int) -> Blob -> Blob
void swampCoreBlobIndexedMap(SwampBlob** result, SwampMachineContext* context, SwampFunction** _fn, const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = blob->octets;

    SwampResult fnResult;
    fnResult.expectedOctetSize = 0;

    SwampParameters parameters;
    parameters.parameterCount = 1;
    parameters.octetSize = sizeof(SwampInt32);

    SwampMachineContext ownContext;
    swampContextInit(&ownContext, context->dynamicMemory, context->constantStaticMemory, context->typeInfo);
    // , fn->returnOctetSize,
    //                                                 fn->returnAlign
    SwampBlob* target = swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);
    uint8_t* targetItemPointer = target->octets;
    for (size_t i = 0; i < blob->octetCount; ++i) {
        swampContextReset(&ownContext);
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, &ownContext.bp, &internalFunction);
        fnResult.expectedOctetSize = internalFunction->returnOctetSize;
        SwampInt32 index = i;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &index, sizeof(SwampInt32));
        SwampInt32 v = *sourceItemPointer;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));

        tc_memcpy_octets(ownContext.bp + pos, &v, sizeof(SwampInt32));
        CLOG_INFO("calling for index %d", i);
        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        *targetItemPointer = (uint8_t) (*(SwampInt32*) ownContext.bp);
        sourceItemPointer++;
        targetItemPointer++;
    }

    swampContextDestroy(&ownContext);

    *result = target;
}

// any : (a -> Bool) -> List a -> Bool
void swampCoreBlobAny()
{
}

// find : (a -> Bool) -> List a -> Maybe a
void swampCoreBlobFind()
{
}

//  member : a -> List a -> Bool
void swampCoreBlobMember()
{
}

// filterMap : (a -> Maybe b) -> List a -> List b
void swampCoreBlobFilterMap()
{
}

void swampCoreBlobFilterIndexedMap()
{
}

//       filterMap2 : (a -> b -> Maybe c) -> List a -> List b -> List c
void swampCoreBlobFilterMap2()
{
}

// filter : (a -> Bool) -> List a -> List a
void swampCoreBlobFilter()
{
}

// filter2 : (a -> b -> Bool) -> List a -> List b -> List b
void swampCoreBlobFilter2()
{
}

// remove : (a -> Bool) -> List a -> List a
void swampCoreBlobRemove()
{
}

// remove2 : (a -> b -> Bool) -> List a -> List b -> List b
void swampCoreBlobRemove2()
{
}

// concatMap : (a -> List b) -> List a -> List b
void swampCoreBlobConcatMap()
{
}

// concat : List (List a) -> List a
void swampCoreBlobConcat()
{
}

// range : Int -> Int -> List Int
void swampCoreBlobRange()
{
}

// foldl : (a -> b -> b) -> b -> List a -> b
void swampCoreBlobFoldl()
{
}

// unzip : List (a, b) -> (List a, List b)
void swampCoreBlobUnzip()
{
}

// foldlstop : (a -> b -> Maybe b) -> b -> List a -> b
void swampCoreBlobFoldlStop()
{
}

void swampCoreBlobGet2d()
{
}

void swampCoreBlobToString2d()
{
}

void* swampCoreBlobFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Blob.toString2d", swampCoreBlobToString2d}, {"Blob.map", swampCoreBlobMap},
        {"Blob.indexedMap", swampCoreBlobIndexedMap}, {"Blob.filterIndexedMap", swampCoreBlobFilterIndexedMap},
        {"Blob.get2d", swampCoreBlobGet2d},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
