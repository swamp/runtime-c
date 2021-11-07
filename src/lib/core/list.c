/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-runtime/execute.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>
#include <swamp-runtime/core/maybe.h>

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
void swampCoreListTail(SwampMaybe* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    if (list->count == 0) {
        swampMaybeNothing(result);
    } else {
        swampMaybeJust(result, list->itemAlign, SwampCollectionIndex(list, list->count-1), list->itemSize);
    }
}

// isEmpty : List a -> Bool
void swampCoreListIsEmpty(SwampBool* result, SwampMachineContext* context, const SwampList** _list)
{
    const SwampList* list = *_list;

    *result = list->count == 0;
}

// length : List a -> Int
void swampCoreListLength(SwampInt32 * result, SwampMachineContext* context, const SwampList** _list)
{
    *result = (*_list)->count;
}


// (a -> b) -> List a -> List b
void swampCoreListMap(SwampList** result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;

    SwampParameters parameters;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context);

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

// map2 : (a -> b -> c) -> List a -> List b -> List c
void swampCoreListMap2(SwampList** result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _lista, const SwampList** _listb)
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
    swampContextCreateTemp(&ownContext, context);

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
void swampCoreListIndexedMap(SwampList** result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const uint8_t* sourceItemPointer = list->value;
    const SwampFunction* fn = *_fn;


    const SwampFunc* swampFn = 0;
    swampGetFunc(fn, &swampFn);

    SwampResult fnResult;
    fnResult.expectedOctetSize = swampFn->returnOctetSize;

    SwampParameters parameters;
    parameters.parameterCount = 2;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context);

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
        swampRun(&fnResult, &ownContext, fn, parameters, 1);
        tc_memcpy_octets(targetItemPointer, ownContext.bp, targetListB->itemSize);
        sourceItemPointer += list->itemSize;
        targetItemPointer += targetListB->itemSize;
    }

    swampContextDestroyTemp(&ownContext);

    *result = targetListB;
}

// any : (a -> Bool) -> List a -> Bool
void swampCoreListAny(SwampBool* result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;

    SwampParameters parameters;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context);

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
void swampCoreListFind(SwampMaybe * result, SwampMachineContext* context, SwampFunction** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunction* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;

    SwampParameters parameters;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context);

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
void swampCoreListMember(SwampBool* result, SwampMachineContext* context, const void* data, const SwampList** _list)
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
void swampCoreListFilterMap(void)
{
    CLOG_ERROR("Not implemented")
}

//       filterMap2 : (a -> b -> Maybe c) -> List a -> List b -> List c
void swampCoreListFilterMap2(void)
{

}

// filter : (a -> Bool) -> List a -> List a
void swampCoreListFilter(void)
{

}


// filter2 : (a -> b -> Bool) -> List a -> List b -> List b
void swampCoreListFilter2(void)
{

}

//remove : (a -> Bool) -> List a -> List a
void swampCoreListRemove(void)
{

}

// remove2 : (a -> b -> Bool) -> List a -> List b -> List b
void swampCoreListRemove2(void)
{

}

// concatMap : (a -> List b) -> List a -> List b
void swampCoreListConcatMap(void)
{

}

// concat : List (List a) -> List a
void swampCoreListConcat(void)
{

}


// range : Int -> Int -> List Int
void swampCoreListRange(void)
{

}

void align(size_t* pos, size_t align)
{
    size_t rest = *pos % align;
    if (rest != 0) {
        *pos += align - rest;
    }
}


// foldl : (a -> b -> b) -> b -> List a -> b
void swampCoreListFoldl(void* result, SwampMachineContext* context, SwampFunction*** _fn, const SwampUnknownType* initialValue, const SwampList*** _list)
{
    const SwampList* list = **_list;
    const SwampFunction* fn = **_fn;

    size_t aAlign = list->itemAlign;
    size_t aSize = list->itemSize;
    if (aSize == 0) {
        CLOG_ERROR("size is zero");
    }

    size_t bOffset = aSize;

    align(&bOffset, initialValue->align);
    size_t bSize = initialValue->size;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context);

    size_t bAlign = initialValue->align;

    if (list->count == 0) {
        tc_memcpy_octets(ownContext.bp, initialValue->ptr, bSize);
    }
    const uint8_t* sourceItemPointer = list->value;
    for (size_t i = 0; i < list->count; ++i) {
        SwampFunc* internalFunction;
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

// foldlstop : (a -> b -> Maybe b) -> b -> List a -> b
void swampCoreListFoldlStop(void* result, SwampMachineContext* context, SwampFunc*** _fn, const SwampUnknownType* initialValue, const SwampList*** _list)
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
    swampContextCreateTemp(&ownContext, context);

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
        CLOG_INFO("foldlstop:  for index %d, list item:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, fn, parameters, 1);
        if (swampMaybeIsNothing(ownContext.bp)) {
            break;
        } else {
            const uint8_t* justValue = swampMaybeJustGetValue(ownContext.bp, initialValue->align);
            tc_memcpy_octets(lastKnownGoodValue, justValue, bSize);
            tc_memmove_octets(ownContext.bp, justValue, bSize);
        }
        CLOG_INFO("foldlstop: accumulator is now for index %d, value:%d", i, *(const SwampInt32*)ownContext.bp);
        sourceItemPointer += aSize;
    }

    tc_memcpy_octets(result, lastKnownGoodValue, bSize);

    swampContextDestroy(&ownContext);
}

// unzip : List (a, b) -> (List a, List b)
void swampCoreListUnzip(void)
{

}




void* swampCoreListFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"List.head", swampCoreListHead},
        {"List.tail", swampCoreListTail},
        {"List.isEmpty", swampCoreListIsEmpty},
        {"List.length", swampCoreListLength},
        {"List.map", swampCoreListMap},
        {"List.map2", swampCoreListMap2},
        {"List.indexedMap", swampCoreListIndexedMap},
        {"List.filterMap", swampCoreListFilterMap},
        {"List.foldl", swampCoreListFoldl},
        {"List.foldlstop", swampCoreListFoldlStop},
        {"List.any", swampCoreListAny},
        {"List.find", swampCoreListFind},
        {"List.member", swampCoreListMember},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
