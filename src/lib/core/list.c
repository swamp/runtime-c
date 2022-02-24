/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/list.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-runtime/execute.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>
#include <swamp-typeinfo/typeinfo.h>
#include <swamp-typeinfo/chunk.h>

void swampCoreListHead(SwampMaybe* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    if (list->count == 0) {
        swampMaybeNothing(result);
    } else {
        swampMaybeJust(result, list->itemAlign, list->value, list->itemSize);
    }
}

#define SwampCollectionIndex(coll, index) (((const uint8_t*)coll->value) + (index) * coll->itemSize)

// tail : List a -> Maybe (List a)
static void swampCoreListTail(SwampMaybe* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    if (list->count == 0) {
        swampMaybeNothing(result);
    } else {
        swampMaybeJust(result, list->itemAlign, SwampCollectionIndex(list, list->count-1), list->itemSize);
    }
}

// isEmpty : List a -> Bool
static void swampCoreListIsEmpty(SwampBool* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    *result = list->count == 0;
}

// length : List a -> Int
static void swampCoreListLength(SwampInt32 * result, SwampMachineContext* context, const SwampList** _list)
{
    *result = (*_list)->count;
}

// range : Int -> Int -> List Int
static void swampCoreListRange(SwampList** result, SwampMachineContext* context, const SwampInt32* start, const SwampInt32* end)
{
    size_t length = *end - *start + 1;

    SwampList* mutable = swampListAllocatePrepare(context->dynamicMemory, length, sizeof(SwampInt32), sizeof(SwampInt32));

    SwampInt32* target = *(SwampInt32**) mutable->value;
    for (size_t i=*start; i<=*end; ++i) {
        *target = i;
        target++;
    }

    *result = mutable;
}

// range0 : Int -> List Int
static void swampCoreListRange0(SwampList** result, SwampMachineContext* context, const SwampInt32* length)
{
    SwampList* mutable = swampListAllocatePrepare(context->dynamicMemory, *length, sizeof(SwampInt32), sizeof(SwampInt32));

    SwampInt32* target = (SwampInt32*) mutable->value;
    for (size_t i = 0; i < *length; ++i) {
        *target = i;
        target++;
    }

    *result = mutable;
}


// (a -> b) -> List a -> List b
static void swampCoreListMap(SwampList** result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;

    SwampParameters parameters;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.map()");

    const SwampFunc* realFunc;
    swampGetFunc(fn, &realFunc);
    SwampList* target = swampListAllocatePrepare(context->dynamicMemory, list->count, realFunc->returnOctetSize, realFunc->returnAlign);
    uint8_t* targetItemPointer = (uint8_t*)target->value;
    for (size_t i = 0; i < list->count; ++i) {
        swampContextReset(&ownContext);
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &realFunc);
        fnResult.expectedOctetSize = realFunc->returnOctetSize;
        parameters.parameterCount = realFunc->parameterCount;
        swampMemoryPositionAlign(&pos, list->itemAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceItemPointer, list->itemSize);
        //CLOG_INFO("calling for index %d, value:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, realFunc, parameters, 1);
        tc_memcpy_octets(targetItemPointer, ownContext.bp, target->itemSize);
        sourceItemPointer += list->itemSize;
        targetItemPointer += target->itemSize;
    }

    swampContextDestroyTemp(&ownContext);

    *result = target;
}


// concatMap : (a -> List b) -> List a -> List b
static void swampCoreListConcatMap(const SwampList** result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;

    SwampParameters parameters;

    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.concatMap");

    const SwampFunc* realFunc;
    swampGetFunc(fn, &realFunc);

    parameters.octetSize = realFunc->returnOctetSize;
    const SwtiType* realFuncType_ = swtiChunkTypeFromIndex(context->typeInfo, realFunc->typeIndex);
    if (realFuncType_->type != SwtiTypeFunction) {
        CLOG_ERROR("wrong function type")
    }
    const SwtiFunctionType* realFuncType = (const SwtiFunctionType *) realFuncType_;

    const SwtiType* realFuncReturnType = swtiUnalias(realFuncType->parameterTypes[realFuncType->parameterCount-1]);

    if (realFuncReturnType->type != SwtiTypeList) {
        CLOG_ERROR("wrong function item type")
    }

    const SwtiListType* realFuncReturnTypeList = (const SwtiListType*)realFuncReturnType;
    const SwtiType* itemType = realFuncReturnTypeList->itemType;
    size_t itemMemorySize = swtiGetMemorySize(itemType);
    size_t itemMemoryAlign = swtiGetMemoryAlign(itemType);

    uint8_t* target = context->tempResult;
    size_t maxCount = context->tempResultSize / itemMemorySize;
    size_t count = 0;

//
    uint8_t* targetItemPointer = (uint8_t*)target;
    for (size_t i = 0; i < list->count; ++i) {
        swampContextReset(&ownContext);
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &realFunc);
        fnResult.expectedOctetSize = realFunc->returnOctetSize;
        parameters.parameterCount = realFunc->parameterCount;
        swampMemoryPositionAlign(&pos, list->itemAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceItemPointer, list->itemSize);
        //CLOG_INFO("calling for index %d, value:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, realFunc, parameters, 1);


        const SwampList** returnedList_ = (const SwampList**) ownContext.bp;
        const SwampList* returnedList = *returnedList_;

        if (returnedList->count != 0) {
            if (count + returnedList->count < maxCount) {
                if (returnedList ->itemSize != itemMemorySize) {
                    CLOG_ERROR("wrong item size returned")
                }
                tc_memcpy_octets(targetItemPointer, returnedList->value, returnedList->count * itemMemorySize);
                count += returnedList->count;
                targetItemPointer += returnedList->count * itemMemorySize;
            }
        }

        sourceItemPointer += list->itemSize;
    }

    const SwampList* newList = swampListAllocate(context->dynamicMemory, target, count, itemMemorySize, itemMemoryAlign);

    swampContextDestroyTemp(&ownContext);

    *result = newList;
}


// map2 : (a -> b -> c) -> List a -> List b -> List c
static void swampCoreListMap2(SwampList** result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _lista, const SwampList** _listb)
{
    const SwampList* listA = *_lista;
    const SwampList* listB = *_listb;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceAItemPointer = listA->value;
    const uint8_t* sourceBItemPointer = listB->value;

    size_t countToUse = listA->count;
    if (listB->count < countToUse) {
        countToUse = listB->count;
    }

    SwampResult fnResult;

    SwampParameters parameters;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.map2");

    const SwampFunc* realFunc;
    swampGetFunc(fn, &realFunc);
    parameters.octetSize = realFunc->returnOctetSize;
    SwampList* target = swampListAllocatePrepare(context->dynamicMemory, countToUse, realFunc->returnOctetSize, realFunc->returnAlign);

    uint8_t* targetItemPointer = (uint8_t*)target->value;
    for (size_t i = 0; i < countToUse; ++i) {
        swampContextReset(&ownContext);
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &realFunc);
        fnResult.expectedOctetSize = realFunc->returnOctetSize;
        parameters.parameterCount = realFunc->parameterCount;

        swampMemoryPositionAlign(&pos, listA->itemAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceAItemPointer, listA->itemSize);
        pos += listA->itemSize;

        swampMemoryPositionAlign(&pos, listB->itemAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceBItemPointer, listB->itemSize);
        pos += listB->itemSize;

        //CLOG_INFO("calling for index %d, value:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, realFunc, parameters, 1);

        tc_memcpy_octets(targetItemPointer, ownContext.bp, target->itemSize);
        sourceAItemPointer += listA->itemSize;
        sourceBItemPointer += listB->itemSize;
        targetItemPointer += target->itemSize;
    }

    swampContextDestroyTemp(&ownContext);

    *result = target;
}

// indexedMap: (Int -> a -> b) -> List a -> List b
static void swampCoreListIndexedMap(SwampList** result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const uint8_t* sourceItemPointer = list->value;
    const SwampFunction* fn = *_fn;


    const SwampFunc* swampFn = 0;
    swampGetFunc(fn, &swampFn);

    SwampResult fnResult;
    fnResult.expectedOctetSize = swampFn->returnOctetSize;

    SwampParameters parameters;
    parameters.parameterCount = swampFn->parameterCount;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.indexedMap");

    SwampList* targetListB = swampListAllocatePrepare(context->dynamicMemory, list->count, swampFn->returnOctetSize, swampFn->returnAlign);
    uint8_t* targetItemPointer = (uint8_t*)targetListB->value;
    for (size_t i = 0; i < list->count; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);
        SwampInt32 index = i;

        swampMemoryPositionAlign(&pos, sizeof(SwampInt32));
        tc_memcpy_octets(ownContext.bp + pos, &index, sizeof(SwampInt32));
        pos += sizeof(SwampInt32);

        swampMemoryPositionAlign(&pos, list->itemAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceItemPointer, list->itemSize);
        //CLOG_INFO("calling for index %d, value:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        tc_memcpy_octets(targetItemPointer, ownContext.bp, targetListB->itemSize);
        sourceItemPointer += list->itemSize;
        targetItemPointer += targetListB->itemSize;
    }

    swampContextDestroyTemp(&ownContext);

    *result = targetListB;
}

// any : (a -> Bool) -> List a -> Bool
static void swampCoreListAny(SwampBool* result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;

    SwampParameters parameters;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.any");

    const SwampFunc* realFunc;
    swampGetFunc(fn, &realFunc);

    if (realFunc->returnOctetSize != sizeof(SwampBool)) {
        CLOG_ERROR("List.any internal error sizeof");
    }

    SwampBool found = 0;

    for (size_t i = 0; i < list->count; ++i) {
        swampContextReset(&ownContext);
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &realFunc);
        fnResult.expectedOctetSize = realFunc->returnOctetSize;
        parameters.parameterCount = realFunc->parameterCount;
        swampMemoryPositionAlign(&pos, list->itemAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceItemPointer, list->itemSize);
        //CLOG_INFO("calling for index %d, value:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, realFunc, parameters, 1);
        SwampBool truthful = *(const SwampBool*) (ownContext.bp);
        if (truthful) {
            found = 1;
            break;
        }

        sourceItemPointer += list->itemSize;
    }

    swampContextDestroyTemp(&ownContext);

    *result = found;
}

// find : (a -> Bool) -> List a -> Maybe a
static void swampCoreListFind(SwampMaybe * result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;

    SwampParameters parameters;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.find");

    const SwampFunc* realFunc;
    swampGetFunc(fn, &realFunc);

    if (realFunc->returnOctetSize != sizeof(SwampBool)) {
        CLOG_ERROR("List.any internal error sizeof");
    }

    SwampBool found = 0;

    for (size_t i = 0; i < list->count; ++i) {
        swampContextReset(&ownContext);
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &realFunc);
        fnResult.expectedOctetSize = realFunc->returnOctetSize;
        parameters.parameterCount = realFunc->parameterCount;
        swampMemoryPositionAlign(&pos, list->itemAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceItemPointer, list->itemSize);
        //CLOG_INFO("calling for index %d, value:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, realFunc, parameters, 1);
        SwampBool truthful = *(const SwampBool*) (ownContext.bp);
        if (truthful) {
            found = 1;
            swampMaybeJust(result, list->itemAlign, sourceItemPointer, list->itemSize);
            break;
        }

        sourceItemPointer += list->itemSize;
    }

    if (!found) {
        swampMaybeNothing(result);
    }

    swampContextDestroyTemp(&ownContext);
}

//  member : a -> List a -> Bool
static void swampCoreListMember(SwampBool* result, SwampMachineContext* context, const void* data, const SwampList** _list)
{
    const SwampList* list = *_list;
    const uint8_t* sourceItemPointer = list->value;

    SwampBool found = 0;

    for (size_t i = 0; i < list->count; ++i) {
        if (tc_memcmp(sourceItemPointer, data, list->itemSize) == 0) {
            found = 1;
            break;
        }
        sourceItemPointer += list->itemSize;
    }

    *result = found;
}

// filterMap : (a -> Maybe b) -> List a -> List b
static void swampCoreListFilterMap(void)
{
    CLOG_ERROR("Not implemented")
}

//       filterMap2 : (a -> b -> Maybe c) -> List a -> List b -> List c
static void swampCoreListFilterMap2(void)
{

}

// filter : (a -> Bool) -> List a -> List a
static void swampCoreListFilter(void)
{

}


// filter2 : (a -> b -> Bool) -> List a -> List b -> List b
static void swampCoreListFilter2(void)
{

}

//remove : (a -> Bool) -> List a -> List a
static void swampCoreListRemove(void)
{

}

// remove2 : (a -> b -> Bool) -> List a -> List b -> List b
static void swampCoreListRemove2(void)
{

}



// concat : List (List a) -> List a
static void swampCoreListConcat(void)
{

}


static void align(size_t* pos, size_t align)
{
    size_t rest = *pos % align;
    if (rest != 0) {
        *pos += align - rest;
    }
}


// foldl : (a -> b -> b) -> b -> List a -> b
static void swampCoreListFoldl(void* result, SwampMachineContext* context, SwampFunction*** _fn, const SwampUnknownType* initialValue, const SwampList*** _list)
{
    const SwampList* list = **_list;
    const SwampFunction* fn = **_fn;

    size_t aAlign = list->itemAlign;
    size_t aSize = list->itemSize;
    if (list->count != 0 && aSize == 0) {
        CLOG_ERROR("size is zero");
    }

    size_t bOffset = aSize;

    align(&bOffset, initialValue->align);
    size_t bSize = initialValue->size;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.foldl");

    size_t bAlign = initialValue->align;

    if (list->count == 0) {
        tc_memcpy_octets(ownContext.bp, initialValue->ptr, bSize);
    }
    const uint8_t* sourceItemPointer = list->value;
    for (size_t i = 0; i < list->count; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);

        SwampResult fnResult;
        fnResult.expectedOctetSize = bSize;

        SwampParameters parameters;
        parameters.parameterCount = internalFunction->parameterCount;
        parameters.octetSize = aSize + bSize;

        swampMemoryPositionAlign(&pos, aAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceItemPointer, aSize);
        pos += aSize;

        swampMemoryPositionAlign(&pos, bAlign);
        if (i == 0) {
            tc_memcpy_octets(ownContext.bp + pos, initialValue->ptr, bSize);
        } else {
            tc_memcpy_octets(ownContext.bp + pos, ownContext.bp, bSize);
        }
        pos += bSize;


        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        sourceItemPointer += aSize;
    }

    tc_memcpy_octets(result, ownContext.bp, bSize);

    swampContextDestroyTemp(&ownContext);
}


// reduce : (a -> a -> a) -> List a -> a
static void swampCoreListReduce(void* result, SwampMachineContext* context, SwampFunction*** _fn, const SwampList*** _list)
{
    const SwampList* list = **_list;
    const SwampFunction* fn = **_fn;

    size_t aAlign = list->itemAlign;
    size_t aSize = list->itemSize;
    if (aSize == 0) {
        CLOG_ERROR("size is zero");
    }

    if (list->count == 0) {
        CLOG_ERROR("sorry reduce is not supported with zero length lists");
    }

    const uint8_t* sourceItemPointer = list->value;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.reduce");
    tc_memcpy_octets(ownContext.bp, sourceItemPointer, list->itemSize);
    sourceItemPointer += aSize;

    for (size_t i = 1; i < list->count; ++i) {
        const SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);

        SwampResult fnResult;
        fnResult.expectedOctetSize = aSize;

        SwampParameters parameters;
        parameters.parameterCount = internalFunction->parameterCount;
        parameters.octetSize = aSize + aSize;

        swampMemoryPositionAlign(&pos, aAlign);
        tc_memcpy_octets(ownContext.bp + pos, sourceItemPointer, aSize);
        pos += aSize;

        swampMemoryPositionAlign(&pos, aAlign);
        tc_memcpy_octets(ownContext.bp + pos, ownContext.bp, aSize);
        pos += aSize;


        swampRun(&fnResult, &ownContext, internalFunction, parameters, 1);
        sourceItemPointer += aSize;
    }

    tc_memcpy_octets(result, ownContext.bp, aSize);

    swampContextDestroyTemp(&ownContext);
}



// foldlstop : (a -> b -> Maybe b) -> b -> List a -> b
static void swampCoreListFoldlStop(void* result, SwampMachineContext* context, SwampFunc*** _fn, const SwampUnknownType* initialValue, const SwampList*** _list)
{
    const SwampList* list = **_list;
    const SwampFunc* fn = **_fn;

    size_t aSize = list->itemSize;
    size_t bSize = fn->returnOctetSize;

    SwampResult fnResult;
    fnResult.expectedOctetSize = bSize;

    SwampParameters parameters;
    parameters.parameterCount = 2;
    parameters.octetSize = aSize + bSize;

    size_t bOffset = aSize;

    align(&bOffset, initialValue->align);


    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context, "List.foldlstop");

    uint8_t tempBuf[32];
    uint8_t lastKnownGoodValue[32];

    tc_memcpy_octets(ownContext.bp, initialValue->ptr, bSize);
    tc_memcpy_octets(lastKnownGoodValue, initialValue->ptr, bSize);
    CLOG_INFO("foldlstop: accumulator is now for index %d, value:%d", -1, *(const SwampInt32*)ownContext.bp);
    const uint8_t* sourceItemPointer = list->value;
    for (size_t i = 0; i < list->count; ++i) {
        tc_memcpy_octets(tempBuf, sourceItemPointer, aSize);
        tc_memcpy_octets(tempBuf + bOffset, ownContext.bp, bSize);
        swampContextReset(&ownContext);
        tc_memcpy_octets(ownContext.bp +bSize, tempBuf, aSize + bSize);
        CLOG_INFO("foldlstop:  for index %zu, list item:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, fn, parameters, 1);
        if (swampMaybeIsNothing(ownContext.bp)) {
            break;
        } else {
            const uint8_t* justValue = swampMaybeJustGetValue(ownContext.bp, initialValue->align);
            tc_memcpy_octets(lastKnownGoodValue, justValue, bSize);
            tc_memmove_octets(ownContext.bp, justValue, bSize);
        }
        CLOG_INFO("foldlstop: accumulator is now for index %zu, value:%d", i, *(const SwampInt32*)ownContext.bp);
        sourceItemPointer += aSize;
    }

    tc_memcpy_octets(result, lastKnownGoodValue, bSize);

    swampContextDestroy(&ownContext);
}

// unzip : List (a, b) -> (List a, List b)
static void swampCoreListUnzip(void)
{

}




const void* swampCoreListFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"List.head", SWAMP_C_FN(swampCoreListHead)},
        {"List.tail", SWAMP_C_FN(swampCoreListTail)},
        {"List.isEmpty", SWAMP_C_FN(swampCoreListIsEmpty)},
        {"List.length", SWAMP_C_FN(swampCoreListLength)},
        {"List.map", SWAMP_C_FN(swampCoreListMap)},
        {"List.map2", SWAMP_C_FN(swampCoreListMap2)},
        {"List.indexedMap", SWAMP_C_FN(swampCoreListIndexedMap)},
        {"List.filterMap", SWAMP_C_FN(swampCoreListFilterMap)},
        {"List.foldl", SWAMP_C_FN(swampCoreListFoldl)},
        {"List.foldlstop", SWAMP_C_FN(swampCoreListFoldlStop)},
        {"List.reduce", SWAMP_C_FN(swampCoreListReduce)},
        {"List.any", SWAMP_C_FN(swampCoreListAny)},
        {"List.find", SWAMP_C_FN(swampCoreListFind)},
        {"List.member", SWAMP_C_FN(swampCoreListMember)},
        {"List.range", SWAMP_C_FN(swampCoreListRange)},
        {"List.range0", SWAMP_C_FN(swampCoreListRange0)},
        {"List.concatMap", SWAMP_C_FN(swampCoreListConcatMap)},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
