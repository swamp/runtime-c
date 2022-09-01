/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/array.h>
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/blob.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-runtime/core/types.h>
#include <swamp-runtime/execute.h>
#include <swamp-runtime/panic.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/types.h>
#include <swamp-typeinfo/chunk.h>
#include <swamp-typeinfo/typeinfo.h>
#include <tiny-libc/tiny_libc.h>

typedef struct BlobRect {
    SwampCoreSize2i size;
    SwampCorePosition2i position;
} BlobRect;






static void swampCoreBlobHead(SwampMaybe* result, SwampMachineContext* context, const SwampList** _list)
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
static void swampCoreBlobTail(SwampMaybe* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    if (list->count == 0) {
        swampMaybeNothing(result);
    } else {
        swampMaybeJust(result, list->itemAlign, SwampCollectionIndex(list, list->count - 1), list->itemSize);
    }
}

// isEmpty : List a -> Bool
static void swampCoreBlobIsEmpty(SwampBool* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    *result = list->count == 0;
}

// length : List a -> Int
static void swampCoreBlobLength(SwampInt32* result, SwampMachineContext* context, const SwampList** _list)
{
    *result = (*_list)->count;
}

// __externalfn fromArray : Array Int -> Blob
static void swampCoreBlobFromArray(SwampBlob** result, SwampMachineContext* context, const SwampArray** _array)
{
    const SwampArray* array = *_array;

    if (array->itemSize != sizeof(SwampInt32)) {
        CLOG_ERROR("must be swamp integer array for blob")
    }

    SwampBlob* targetBlob = swampBlobAllocatePrepare(context->dynamicMemory, array->count);
    uint8_t* targetItemPointer = (uint8_t*)targetBlob->octets;
    for (size_t i = 0; i < array->count; ++i) {
        *targetItemPointer++ = *(const SwampInt32*) (((uint8_t*) array->value) + i * array->itemSize);
    }

    *result = targetBlob;
}

// __externalfn fromList : List Int -> Blob
static void swampCoreBlobFromList(SwampBlob** result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    if (list->itemSize != sizeof(SwampInt32)) {
        CLOG_ERROR("must be swamp integer array for blob")
    }

    SwampBlob* targetBlob = swampBlobAllocatePrepare(context->dynamicMemory, list->count);
    uint8_t* targetItemPointer = (uint8_t *) targetBlob->octets;
    for (size_t i = 0; i < list->count; ++i) {
        SwampInt32 v = *(const SwampInt32*) (((uint8_t*) list->value) + i * list->itemSize);
        uint8_t clampedV = (uint8_t)v;
        *targetItemPointer++ = clampedV;
    }

    *result = targetBlob;
}

// __externalfn mapToBlob : (Int -> Int) -> Blob -> Blob
static void swampCoreBlobMapToBlob(SwampBlob** result, SwampMachineContext* context, SwampFunction** _fn, const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = blob->octets;

    SwampResult fnResult;


    SwampParameters parameters;
    parameters.parameterCount = 1;
    parameters.octetSize = sizeof(SwampInt32);

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "mapToBlob()");
    // , fn->returnOctetSize,
    //                                                 fn->returnAlign
    SwampBlob* target = swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);
    uint8_t* targetItemPointer = (uint8_t *)target->octets;
    for (size_t i = 0; i < blob->octetCount; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);
        fnResult.expectedOctetSize = internalFunction->returnOctetSize;

        SwampInt32 v = *sourceItemPointer;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));

        tc_memcpy_octets(ownContext.bp + pos, &v, parameters.octetSize);
        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        SwampInt32 returnValue = *(SwampInt32*) ownContext.bp;

        *targetItemPointer = (uint8_t) returnValue;
        sourceItemPointer++;
        targetItemPointer++;
    }

    swampContextDestroyTemp(&ownContext);

    *result = target;
}

// map2 : (a -> b -> c) -> List a -> List b -> List c
static void swampCoreBlobMap2(void)
{
}

// __externalfn indexedMapToBlob : (Int -> Int -> Int) -> Blob -> Blob
static void swampCoreBlobIndexedMapToBlob(SwampBlob** result, SwampMachineContext* context, SwampFunction** _fn,
                             const SwampBlob** _blob)
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
    swampContextCreateTemp(&ownContext, context, "Blob.indexedMapToBlob");
    // , fn->returnOctetSize,
    //                                                 fn->returnAlign
    SwampBlob* target = swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);
    uint8_t* targetItemPointer = (uint8_t *)target->octets;
    for (size_t i = 0; i < blob->octetCount; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);
        fnResult.expectedOctetSize = internalFunction->returnOctetSize;
        SwampInt32 index = i;

        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &index, sizeof(SwampInt32));
        pos += sizeof(SwampInt32);

        SwampInt32 v = *sourceItemPointer;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &v, sizeof(SwampInt32));
        pos += sizeof(SwampInt32);

        parameters.octetSize = pos;
        parameters.parameterCount = internalFunction->parameterCount;
        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        *targetItemPointer = (uint8_t) (*(SwampInt32*) ownContext.bp);
        sourceItemPointer++;
        targetItemPointer++;
    }

    swampContextDestroyTemp(&ownContext);

    *result = target;
}


static void swampCoreBlobIndexedMapToBlobMutable(const SwampBlob** result, SwampMachineContext* context, SwampFunction** _fn,
                                   const SwampBlob** _blob)
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
    swampContextCreateTemp(&ownContext, context, "Blob.indexedMapToBlob!()");

    uint8_t* targetItemPointer = (uint8_t *)blob->octets;
    for (size_t i = 0; i < blob->octetCount; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);
        fnResult.expectedOctetSize = internalFunction->returnOctetSize;
        SwampInt32 index = i;

        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &index, sizeof(SwampInt32));
        pos += sizeof(SwampInt32);

        SwampInt32 v = *sourceItemPointer;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &v, sizeof(SwampInt32));
        pos += sizeof(SwampInt32);

        parameters.octetSize = pos;
        parameters.parameterCount = internalFunction->parameterCount;
        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        *targetItemPointer = (uint8_t) (*(SwampInt32*) ownContext.bp);
        sourceItemPointer++;
        targetItemPointer++;
    }

    swampContextDestroyTemp(&ownContext);

    *result = blob;
}


// __externalfn map2d : ({ x : Int, y : Int } -> Int -> a) -> { width : Int, height : Int } -> Blob -> List a
static void swampCoreBlobMap2d(SwampList** result, SwampMachineContext* context, SwampFunction** _fn,
                        const SwampCoreSize2i* blobSize, const SwampBlob** _blob)
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
    swampContextCreateTemp(&ownContext, context, "Blob.map2d");

    if (blobSize->width <= 0) {
        CLOG_ERROR("width must be greater than zero")
    }

    const SwtiType* returnType = swampCoreGetFunctionReturnType(context->typeInfo, fn);

    SwtiMemorySize returnSize = swtiGetMemorySize(returnType);
    SwtiMemoryAlign returnAlign = swtiGetMemoryAlign(returnType);
    SwampList* preparedList = swampListAllocatePrepare(context->dynamicMemory, blob->octetCount, returnSize, returnAlign);
    uint8_t* targetPointer = (uint8_t *) preparedList->value;

    size_t width = blobSize->width;

    for (size_t i = 0; i < blob->octetCount; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);
        fnResult.expectedOctetSize = internalFunction->returnOctetSize;

        SwampCorePosition2i blobPosition;
        blobPosition.x = i % width;
        blobPosition.y = i / width;

        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &blobPosition, sizeof(SwampCorePosition2i));
        pos += sizeof(SwampCorePosition2i);

        SwampInt32 v = *sourceItemPointer;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &v, sizeof(SwampInt32));
        pos += sizeof(SwampInt32);

        parameters.octetSize = pos;
        parameters.parameterCount = internalFunction->parameterCount;
        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);

        sourceItemPointer++;
        tc_memcpy_octets(targetPointer, ownContext.bp, returnSize);
        targetPointer += returnSize;
    }

    swampContextDestroyTemp(&ownContext);

    *result = preparedList;
}

// any : (Int -> Bool) -> Blob -> Bool
static void swampCoreBlobAny(SwampBool* result, SwampMachineContext* context, SwampFunction** _fn, const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;
    const SwampFunction* fn = *_fn;

    SwampResult fnResult;

    SwampParameters parameters;
    parameters.parameterCount = 1;
    parameters.octetSize = sizeof(SwampInt32);

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "Blob.any");

    const uint8_t* sourceItemPointer = blob->octets;
    SwampBool foundAny = 0;

    for (size_t i = 0; i < blob->octetCount; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);
        fnResult.expectedOctetSize = internalFunction->returnOctetSize;

        SwampInt32 v = *sourceItemPointer;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &v, parameters.octetSize);

        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        SwampBool returnValue = *(SwampBool*) ownContext.bp;
        if (returnValue) {
            foundAny = 1;
            break;
        }
        sourceItemPointer++;
    }

    swampContextDestroyTemp(&ownContext);

    *result = foundAny;
}

// find : (a -> Bool) -> Blob -> Maybe a
static void swampCoreBlobFind(void)
{
}

//  member : Int -> Blob -> Bool
static void swampCoreBlobMember(SwampBool* result, SwampMachineContext* context, SwampInt32* runeToLookFor,
                         const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;

    void* foundPointer = tc_memchr(blob->octets, *runeToLookFor, blob->octetCount);
    *result = (foundPointer != 0);
}

// filterMap : (a -> Maybe b) -> Blob -> List b
static void swampCoreBlobFilterMap(void)
{
}

// __externalfn slice2d : { x : Int, y : Int } -> { width : Int, height : Int } -> { width : Int, height : Int } -> Blob
// -> Blob
static void swampCoreBlobSlice2d(SwampBlob** result, SwampMachineContext* context, const SwampCorePosition2i* slicePos,
                          const SwampCoreSize2i* blobSize, SwampCoreSize2i* sliceSize, const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;

    BlobRect sliceRect;
    sliceRect.position = *slicePos;
    sliceRect.size = *sliceSize;

    SwampInt32 rightSide = sliceRect.position.x + sliceRect.size.width;
    if (rightSide > blobSize->width) {
        CLOG_ERROR("wrong slice2d width")
    }

    if ((blob->octetCount % blobSize->width) != 0) {
        CLOG_ERROR("wrong slice2d, not an even blob 2d")
    }

    SwampInt32 calculatedHeight = blob->octetCount / blobSize->width;
    if (calculatedHeight != blobSize->height) {
        CLOG_ERROR("wrong height")
    }

    SwampInt32 lowerSide = sliceRect.position.y + sliceRect.size.height;
    if (lowerSide > calculatedHeight) {
        CLOG_ERROR("wrong slice2d height %d of %d", lowerSide, calculatedHeight)
    }

    if (sliceRect.position.x < 0 || sliceRect.position.y < 0) {
        CLOG_ERROR("wrong position")
    }

    SwampBlob* targetBlob = swampBlobAllocatePrepare(context->dynamicMemory,
                                                     sliceRect.size.width * sliceRect.size.height);

    uint8_t* targetOctets = (uint8_t*) targetBlob->octets;

    for (size_t y = sliceRect.position.y; y < lowerSide; ++y) {
        const uint8_t* source = blob->octets + y * blobSize->width + sliceRect.position.x;
        tc_memcpy_octets(targetOctets, source, sliceRect.size.width);
        targetOctets += sliceRect.size.width;
    }

    *result = targetBlob;
}

// __externalfn filterIndexedMap : (Int -> Int -> Maybe a) -> Blob -> List a
static void swampCoreBlobFilterIndexedMap(const SwampList** result, SwampMachineContext* context, SwampFunction** _fn,
                                   const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;
    const SwampFunction* fn = *_fn;

    const SwtiType* returnType = swampCoreMaybeReturnType(context->typeInfo, fn);
    const SwtiType* maybeReturnType = swampCoreGetFunctionReturnType(context->typeInfo, fn);

    SwtiMemorySize returnSize = swtiGetMemorySize(returnType);
    SwtiMemoryAlign returnAlign = swtiGetMemoryAlign(returnType);
    uint8_t* temp = tc_malloc(returnSize * blob->octetCount);
    size_t resultCount = 0;
    uint8_t* targetPointer = temp;

    SwampMachineContext ownContext;

    swampContextCreateTemp(&ownContext, context, "Blob.filterIndexedMap");
    SwampResult fnResult;
    SwtiMemorySize returnSizeWithMaybe = swtiGetMemorySize(maybeReturnType);
    fnResult.expectedOctetSize = returnSizeWithMaybe;

    SwampParameters parameters;

    for (size_t i = 0; i < blob->octetCount; ++i) {
        uint8_t currentOctet = blob->octets[i];

        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);

        uint8_t* argumentPointer = ownContext.bp;

        swampMemoryPositionAlign(&pos, 4);
        SwampInt32 indexValue = i;
        tc_memcpy_octets(argumentPointer + pos, &indexValue, sizeof(SwampInt32));
        pos += sizeof(SwampInt32);

        SwampInt32 intValue = currentOctet;
        swampMemoryPositionAlign(&pos, 4);
        tc_memcpy_octets(argumentPointer + pos, &intValue, sizeof(SwampInt32));
        pos += sizeof(SwampInt32);

        parameters.parameterCount = internalFunction->parameterCount;
        parameters.octetSize = sizeof(SwampInt32) + sizeof(SwampInt32);

        int runErr = swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        if (runErr < 0) {
            CLOG_ERROR("couldn't run %d", runErr)
            return;
        }

        if (swampMaybeIsJust(ownContext.bp)) {
            const void* sourceData = swampMaybeJustGetValue(ownContext.bp, returnAlign);
            tc_memcpy_octets(targetPointer, sourceData, returnSize);
            targetPointer += returnSize;
            resultCount++;
        }
    }

    const SwampList* list = swampListAllocate(context->dynamicMemory, temp, resultCount, returnSize, returnAlign);

    swampContextDestroyTemp(&ownContext);

    tc_free(temp);

    *result = list;
}


// __externalvarfn filterIndexedMap2d : ({ x : Int, y : Int } -> Int -> Maybe a) -> { width : Int, height : Int } -> Blob -> List a
static void swampCoreBlobFilterIndexedMap2d(const SwampList** result, SwampMachineContext* context, SwampFunction** _fn,
                                   const SwampCoreSize2i* blobSize, const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;
    const SwampFunction* fn = *_fn;

    const SwtiType* returnType = swampCoreMaybeReturnType(context->typeInfo, fn);
    const SwtiType* maybeReturnType = swampCoreGetFunctionReturnType(context->typeInfo, fn);

    SwtiMemorySize returnSize = swtiGetMemorySize(returnType);
    SwtiMemoryAlign returnAlign = swtiGetMemoryAlign(returnType);
    uint8_t* temp = tc_malloc(returnSize * blob->octetCount);
    size_t resultCount = 0;
    uint8_t* targetPointer = temp;

    SwampMachineContext ownContext;

    swampContextCreateTemp(&ownContext, context, "Blob.filterIndexedMap2d");
    SwampResult fnResult;

    SwtiMemorySize returnSizeWithMaybe = swtiGetMemorySize(maybeReturnType);
    fnResult.expectedOctetSize = returnSizeWithMaybe;

    SwampParameters parameters;

    for (size_t i = 0; i < blob->octetCount; ++i) {
        uint8_t currentOctet = blob->octets[i];

        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);

        uint8_t* argumentPointer = ownContext.bp;

        SwampCorePosition2i position;
        position.x = i % blobSize->width;
        position.y = i / blobSize->width;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(argumentPointer + pos, &position, sizeof(SwampCorePosition2i));
        pos += sizeof(SwampCorePosition2i);

        SwampInt32 intValue = currentOctet;
        swampMemoryPositionAlign(&pos, 4);
        tc_memcpy_octets(argumentPointer + pos, &intValue, sizeof(SwampInt32));

        parameters.parameterCount = internalFunction->parameterCount;
        parameters.octetSize = sizeof(SwampInt32) + sizeof(SwampInt32);

        int runErr = swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        if (runErr < 0) {
            CLOG_ERROR("couldn't run %d", runErr)
            return;
        }

        if (swampMaybeIsJust(ownContext.bp)) {
            const void* sourceData = swampMaybeJustGetValue(ownContext.bp, returnAlign);
            tc_memcpy_octets(targetPointer, sourceData, returnSize);
            targetPointer += returnSize;
            resultCount++;
        }
    }

    const SwampList* list = swampListAllocate(context->dynamicMemory, temp, resultCount, returnSize, returnAlign);

    swampContextDestroyTemp(&ownContext);

    tc_free(temp);

    *result = list;
}

int swampBlobIsEmpty(const SwampBlob* blob) // TODO: Move this
{
    return blob->octets == 0;
}

//       filterMap2 : (a -> b -> Maybe c) -> List a -> List b -> List c
static void swampCoreBlobFilterMap2(void)
{
}

// filter : (a -> Bool) -> List a -> List a
static void swampCoreBlobFilter(void)
{
}

// filter2 : (a -> b -> Bool) -> List a -> List b -> List b
static void swampCoreBlobFilter2(void)
{
}

// remove : (a -> Bool) -> List a -> List a
static void swampCoreBlobRemove(void)
{
}

// remove2 : (a -> b -> Bool) -> List a -> List b -> List b
static void swampCoreBlobRemove2(void)
{
}

// concatMap : (a -> List b) -> List a -> List b
static void swampCoreBlobConcatMap(void)
{
}

// concat : List (List a) -> List a
static void swampCoreBlobConcat(void)
{
}

// range : Int -> Int -> List Int
static void swampCoreBlobRange(void)
{
}

// foldl : (a -> b -> b) -> b -> List a -> b
static void swampCoreBlobFoldl(void)
{
}

// unzip : List (a, b) -> (List a, List b)
static void swampCoreBlobUnzip(void)
{
}

// foldlstop : (a -> b -> Maybe b) -> b -> List a -> b
static void swampCoreBlobFoldlStop(void)
{
}

// __externalfn get2d : { x : Int, y : Int } -> { width : Int, height : Int } -> Blob -> Maybe Int
static void swampCoreBlobGet2d(SwampMaybe* result, SwampMachineContext* context, SwampCorePosition2i* position,
                        SwampCoreSize2i* size, const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;

    // These checks are not really needed. You can just check the index, but this
    // gives more valuable information about what went wrong.
    if (position->x < 0 || position->x >= size->width) {
        // SWAMP_LOG_SOFT_ERROR("position X is out of bounds %d %d", position->x, size->width);
        swampMaybeNothing(result);
        return;
    }

    if (position->y < 0 || position->y >= size->height) {
        // SWAMP_LOG_SOFT_ERROR("position Y is out of bounds %d %d", position->y, size->height);
        swampMaybeNothing(result);
        return;
    }

    int index = position->y * size->width + position->x;

    if (index < 0 || index >= blob->octetCount) {
        // SWAMP_LOG_SOFT_ERROR("position is out of octet count bounds %d %d", index, blob->octet_count);
        swampMaybeNothing(result);
        return;
    }

    SwampInt32 v = blob->octets[index];

    swampMaybeJust(result, 4, &v, sizeof(SwampInt32));
}

// __externalfn fill2d! : { x : Int, y : Int } -> { width : Int, height : Int } -> Int -> { width : Int, height : Int }
// -> Blob -> Blob
static void swampCoreBlobFill2d(const SwampBlob** result, SwampMachineContext* context, SwampCorePosition2i* position,
                         SwampCoreSize2i* blobDimensions, const SwampInt32* fillValue, const SwampCoreSize2i* fillSize,
                         const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;

    // These checks are not really needed. You can just check the index, but this
    // gives more valuable information about what went wrong.
    if (position->x < 0 || position->x >= blobDimensions->width) {
        swampPanic(context,"position X is out of bounds %d %d", position->x, blobDimensions->width);
        return;
    }

    if (position->y < 0 || position->y >= blobDimensions->height) {
        swampPanic(context, "position Y is out of bounds %d %d", position->y, blobDimensions->height);
        return;
    }

    int index = position->y * blobDimensions->width + position->x;
    if (index < 0 || index >= blob->octetCount) {
        swampPanic(context, "position is out of octet count bounds %d %zu", index, blob->octetCount);
        return;
    }

    if (position->x + fillSize->width > blobDimensions->width) {
        swampPanic(context,"fill blobDimensions is wrong");
    }

    if (position->y + fillSize->height > blobDimensions->height) {
        swampPanic(context,"fill blobDimensions is wrong");
    }

    const SwampBlob* newBlob = blob; // MUTABLE! // swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);
    //tc_memcpy_octets(newBlob->octets, blob->octets, blob->octetCount);

    uint8_t* targetOctets = (uint8_t *)newBlob->octets + position->y * blobDimensions->width + position->x;
    uint8_t fillValueOctet = *fillValue;
    for (size_t y = position->y; y < position->y + fillSize->height; ++y) {
        tc_memset_octets(targetOctets, fillValueOctet, fillSize->width);
        targetOctets += blobDimensions->width;
    }

    *result = newBlob;
}


// __externalfn drawWindow2d! : { x : Int, y : Int } -> { width : Int, height : Int } -> { width : Int, height : Int }
// -> Blob -> Blob
static void swampCoreBlobDrawWindow2d(const SwampBlob** result, SwampMachineContext* context, SwampCorePosition2i* position,
                         SwampCoreSize2i* size, const SwampCoreSize2i* fillSize,
                         const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;

    // These checks are not really needed. You can just check the index, but this
    // gives more valuable information about what went wrong.
    if (position->x < 0 || position->x >= size->width) {
        swampPanic(context, "position X is out of bounds %d of width %d", position->x, size->width);
        return;
    }

    if (position->y < 0 || position->y >= size->height) {
        swampPanic(context, "position Y is out of bounds %d of height %d", position->y, size->height);
        return;
    }

    int index = position->y * size->width + position->x;
    if (index < 0 || index >= blob->octetCount) {
        swampPanic(context, "position is out of octet count bounds %d %zu", index, blob->octetCount);
        return;
    }

    if (position->x + fillSize->width > size->width) {
        swampPanic(context, "fill size is wrong");
    }

    if (position->y + fillSize->height > size->height) {
        swampPanic(context, "fill size is wrong");
    }

    const SwampBlob* newBlob = blob; // MUTABLE! // swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);

    tc_memcpy_octets((void*)newBlob->octets, blob->octets, blob->octetCount);
    uint8_t* targetOctets = (uint8_t *)newBlob->octets + position->y * size->width + position->x;
    *targetOctets = '+';
    targetOctets = (uint8_t *)newBlob->octets + ( position->y + fillSize->height - 1 ) * size->width + position->x;
    *targetOctets = '+';
    targetOctets = (uint8_t *)newBlob->octets + ( position->y  ) * size->width + position->x + fillSize->width - 1;
    *targetOctets = '+';
    targetOctets = (uint8_t *)newBlob->octets + ( position->y + fillSize->height - 1 ) * size->width + position->x + fillSize->width - 1;
    *targetOctets = '+';

    for (size_t x = position->x+1; x < position->x + fillSize->width - 1; ++x) {
        uint8_t* above = (uint8_t *)newBlob->octets + position->y * size->width + x;
        *above = '-';
        uint8_t* below = (uint8_t *)newBlob->octets + (position->y + fillSize->height - 1) * size->width + x;
        *below = '-';
    }

    for (size_t y = position->y+1; y < position->y + fillSize->height -1; ++y) {
        uint8_t* above = (uint8_t *)newBlob->octets + y * size->width + position->x;
        *above = '|';
        uint8_t* below =(uint8_t *) newBlob->octets + y * size->width + position->x + fillSize->width - 1;
        *below = '|';
    }

    *result = newBlob;
}


// __externalfn copy2d! : { x : Int, y : Int } -> { width : Int, height : Int } -> { width : Int, height : Int } -> Blob -> Blob -> Blob
static void swampCoreBlobCopy2d(const SwampBlob** result, SwampMachineContext* context, SwampCorePosition2i* position,
                               SwampCoreSize2i* targetBlobSize, const SwampCoreSize2i* sourceBlobSize, const SwampBlob** _sourceBlob,
                               const SwampBlob** _blob)
{
    const SwampBlob* targetBlob = *_blob;
    const SwampBlob* sourceBlob = *_sourceBlob;

    BlobRect targetRect;
    targetRect.position = *position;
    targetRect.size = *sourceBlobSize;

    // These checks are not really needed. You can just check the index, but this
    // gives more valuable information about what went wrong.
    if (position->x < 0 || position->x >= targetBlobSize->width) {
        swampPanic(context, "position X is out of bounds %d %d", position->x, targetBlobSize->width);
        return;
    }

    if (position->y < 0 || position->y >= targetBlobSize->height) {
        swampPanic(context, "position Y is out of bounds %d %d", position->y, targetBlobSize->height);
        return;
    }

    int index = position->y * targetBlobSize->width + position->x;
    if (index < 0 || index >= targetBlob->octetCount) {
        swampPanic(context, "position is out of octet count bounds %d %zu", index, sourceBlob->octetCount);
        return;
    }

    if (position->x + sourceBlobSize->width > targetBlobSize->width) {
        swampPanic(context, "fill size is wrong. Source x: %d + width: %d vs target width %d", position->x, sourceBlobSize->width, targetBlobSize->width);
    }

    if (position->y + sourceBlobSize->height > targetBlobSize->height) {
        swampPanic(context, "fill size is wrong. Source y:%d height: %d vs target height %d", position->y, sourceBlobSize->height, targetBlobSize->height);
    }

    const SwampBlob* newBlob = targetBlob; // MUTABLE! // swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);

    for (size_t y = 0; y < sourceBlobSize->height; ++y) {
        const uint8_t* sourceOctets = sourceBlob->octets + y * sourceBlobSize->width;
        uint8_t* targetOctets = (uint8_t *) newBlob->octets + (y + targetRect.position.y) * targetBlobSize->width + targetRect.position.x;
        tc_memcpy_octets(targetOctets, sourceOctets, sourceBlobSize->width);
    }

    *result = newBlob;
}


// __externalfn toString2d : { width : Int, height : Int } -> Blob -> String
static void swampCoreBlobToString2d(const SwampString** result, SwampMachineContext* context, SwampCoreSize2i* size,
                             const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;

    int area = size->height * size->width;
    if (area < 0 || area > blob->octetCount) {
        *result = swampStringAllocate(context->dynamicMemory, "");
        return;
    }
    char* tempString = tc_malloc(size->height * (size->width + 1) + 2);

    int index = 0;
    for (size_t y = 0; y < size->height; ++y) {
        tempString[index++] = '\n';
        for (size_t x = 0; x < size->width; ++x) {
            uint8_t ch = blob->octets[size->width * y + x];
            if (ch < 32) {
                ch = '.';
            }
            tempString[index++] = ch;
        }
    }
    tempString[index++] = '\n';
    tempString[index] = 0;

    *result = swampStringAllocate(context->dynamicMemory, tempString);

    tc_free(tempString);
}

// __externalfn make : Int -> Blob
static void swampCoreBlobMake(SwampBlob** result, SwampMachineContext* context, const SwampInt32* octetCount)
{
    SwampBlob* targetBlob = swampBlobAllocatePrepare(context->dynamicMemory,
                                                     *octetCount);

    uint8_t* targetOctets = (uint8_t *)targetBlob->octets;

    tc_memset_octets(targetOctets, 0, *octetCount);

    *result = targetBlob;
}

const void* swampCoreBlobFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Blob.toString2d", SWAMP_C_FN(swampCoreBlobToString2d)},
        {"Blob.mapToBlob", SWAMP_C_FN(swampCoreBlobMapToBlob)},
        {"Blob.indexedMapToBlob", SWAMP_C_FN(swampCoreBlobIndexedMapToBlob)},
        {"Blob.filterIndexedMap", SWAMP_C_FN(swampCoreBlobFilterIndexedMap)},
        {"Blob.indexedMapToBlob!", SWAMP_C_FN(swampCoreBlobIndexedMapToBlobMutable)},
        {"Blob.filterIndexedMap2d", SWAMP_C_FN(swampCoreBlobFilterIndexedMap2d)},
        {"Blob.get2d", SWAMP_C_FN(swampCoreBlobGet2d)},
        {"Blob.fromArray", SWAMP_C_FN(swampCoreBlobFromArray)},
        {"Blob.fromList", SWAMP_C_FN(swampCoreBlobFromList)},
        {"Blob.member", SWAMP_C_FN(swampCoreBlobMember)},
        {"Blob.any", SWAMP_C_FN(swampCoreBlobAny)},
        {"Blob.fill2d!", SWAMP_C_FN(swampCoreBlobFill2d)},
        {"Blob.drawWindow2d!", SWAMP_C_FN(swampCoreBlobDrawWindow2d)},
        {"Blob.copy2d!", SWAMP_C_FN(swampCoreBlobCopy2d)},
        {"Blob.slice2d", SWAMP_C_FN(swampCoreBlobSlice2d)},
        {"Blob.make", SWAMP_C_FN(swampCoreBlobMake)},
        {"Blob.map2d", SWAMP_C_FN(swampCoreBlobMap2d)},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
