/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-runtime/core/types.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-typeinfo/chunk.h>
#include <swamp-typeinfo/typeinfo.h>



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

// __externalfn fromArray : Array Int -> Blob
void swampCoreBlobFromArray(SwampBlob** result, SwampMachineContext* context, const SwampArray** _array)
{
    const SwampArray* array = *_array;

    if (array->itemSize != sizeof(SwampInt32)) {
        CLOG_ERROR("must be swamp integer array for blob");
    }

    SwampBlob* targetBlob = swampBlobAllocatePrepare(context->dynamicMemory, array->count);
    uint8_t* targetItemPointer = targetBlob->octets;
    for (size_t i=0; i<array->count; ++i) {
        *targetItemPointer++ = *(const SwampInt32*)(array->value + i * array->itemSize);
    }

    *result = targetBlob;
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
    swampContextCreateTemp(&ownContext, context);
// , fn->returnOctetSize,
    //                                                 fn->returnAlign
    SwampBlob* target = swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);
    uint8_t* targetItemPointer = target->octets;
    for (size_t i = 0; i < blob->octetCount; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);
        fnResult.expectedOctetSize = internalFunction->returnOctetSize;

        SwampInt32 v = *sourceItemPointer;
        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));

        tc_memcpy_octets(ownContext.bp + pos, &v, parameters.octetSize);
        swampRun(&fnResult, &ownContext, fn, parameters, 1);
        SwampInt32 returnValue = *(SwampInt32*) ownContext.bp;

        *targetItemPointer = (uint8_t) returnValue;
        sourceItemPointer++;
        targetItemPointer++;
    }

    swampContextDestroyTemp(&ownContext);

    *result = target;

}

// map2 : (a -> b -> c) -> List a -> List b -> List c
void swampCoreBlobMap2()
{
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
    swampContextCreateTemp(&ownContext, context);
    // , fn->returnOctetSize,
    //                                                 fn->returnAlign
    SwampBlob* target = swampBlobAllocatePrepare(context->dynamicMemory, blob->octetCount);
    uint8_t* targetItemPointer = target->octets;
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

// __externalfn filterIndexedMap : (Int -> Int -> Maybe a) -> Blob -> List a
void swampCoreBlobFilterIndexedMap(SwampList** result, SwampMachineContext* context, SwampFunction** _fn, const SwampBlob** _blob)
{
    const SwampBlob* blob = *_blob;
    const SwampFunction* fn = *_fn;

    size_t typeIndex;
    if (fn->type == SwampFunctionTypeCurry) {
        const SwampCurryFunc* curry = (const SwampCurryFunc*) fn;
        typeIndex = curry->typeIdIndex;
    } else if (fn->type == SwampFunctionTypeInternal) {
        const SwampFunc* internalFunc = (const SwampFunc*) fn;
        typeIndex = internalFunc->typeIndex;
    }

    const SwtiType* functionType = swtiChunkTypeFromIndex(context->typeInfo, typeIndex);
    if (functionType->type != SwtiTypeFunction) {
        CLOG_ERROR("wrong type");
    }
    const SwtiFunctionType* funcType = (const SwtiFunctionType*) functionType;
    const SwtiType* returnType = funcType->parameterTypes[funcType->parameterCount-1];
    SwtiMemorySize returnSize = swtiGetMemorySize(returnType);
    SwtiMemoryAlign returnAlign = swtiGetMemoryAlign(returnType);
    uint8_t* temp = tc_malloc(returnSize * blob->octetCount);
    size_t resultCount = 0;
    uint8_t* targetPointer = temp;

    SwampMachineContext ownContext;

    swampContextCreateTemp(&ownContext, context);
    SwampResult fnResult;
    fnResult.expectedOctetSize = returnSize;

    SwampParameters parameters;

    for (size_t i = 0; i < blob->octetCount; ++i) {
        const uint8_t* v = blob->octets[i];

        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);

        uint8_t* argumentPointer = ownContext.bp;

        swampMemoryPositionAlign(&pos, 4);
        SwampInt32 indexValue = i;
        tc_memcpy_octets(argumentPointer + pos, &indexValue, sizeof(SwampInt32));

        SwampInt32 intValue = v;
        swampMemoryPositionAlign(&pos, 4);
        tc_memcpy_octets(argumentPointer + pos, &intValue, sizeof(SwampInt32));

        parameters.parameterCount = internalFunction->parameterCount;
        parameters.octetSize = sizeof(SwampInt32) + sizeof(SwampInt32);

        int runErr = swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        if (runErr < 0) {
            CLOG_ERROR("couldn't run %d", runErr);
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

    tc_free(temp);

    *result = list;
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

// __externalfn get2d : { x : Int, y : Int } -> { width : Int, height : Int } -> Blob -> Maybe Int
void swampCoreBlobGet2d(SwampMaybe* result, SwampMachineContext* context, SwampCorePosition2i* position, SwampCoreSize2i* size, const SwampBlob** _blob)
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

// __externalfn toString2d : { width : Int, height : Int } -> Blob -> String
void swampCoreBlobToString2d(SwampString** result, SwampMachineContext* context, SwampCoreSize2i* size, const SwampBlob** _blob)
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
            tempString[index++] = ch;
        }
    }
    tempString[index++] = '\n';
    tempString[index] = 0;

    *result = swampStringAllocate(context->dynamicMemory, tempString);

    tc_free(tempString);
}

void* swampCoreBlobFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Blob.toString2d", swampCoreBlobToString2d}, {"Blob.map", swampCoreBlobMap},
        {"Blob.indexedMap", swampCoreBlobIndexedMap}, {"Blob.filterIndexedMap", swampCoreBlobFilterIndexedMap},
        {"Blob.get2d", swampCoreBlobGet2d},
        {"Blob.fromArray", swampCoreBlobFromArray},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
