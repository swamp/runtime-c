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
void swampCoreListMap(SwampList** result, SwampMachineContext* context, SwampFunc** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunc* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;
    fnResult.expectedOctetSize = fn->returnOctetSize;

    SwampParameters parameters;
    parameters.parameterCount = 1;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextCreateTemp(&ownContext, context);

    SwampList* target = swampListAllocatePrepare(context->dynamicMemory, list->count, fn->returnOctetSize, fn->returnAlign);
    uint8_t* targetItemPointer = target->value;
    for (size_t i = 0; i < list->count; ++i) {
        swampContextReset(&ownContext);
        tc_memcpy_octets(ownContext.bp + fn->returnOctetSize, sourceItemPointer, list->itemSize);
        CLOG_INFO("calling for index %d, value:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, fn, parameters, 1);
        tc_memcpy_octets(targetItemPointer, ownContext.bp, target->itemSize);
        sourceItemPointer += list->itemSize;
        targetItemPointer += target->itemSize;
    }

    swampContextDestroy(&ownContext);

    *result = target;
}

// map2 : (a -> b -> c) -> List a -> List b -> List c
void swampCoreListMap2()
{

}

// indexedMap: (Int -> a -> b) -> List a -> List b
void swampCoreListIndexedMap()
{

}

// any : (a -> Bool) -> List a -> Bool
void swampCoreListAny()
{

}

//find : (a -> Bool) -> List a -> Maybe a
void swampCoreListFind()
{

}

//  member : a -> List a -> Bool
void swampCoreListMember()
{

}

// filterMap : (a -> Maybe b) -> List a -> List b
void swampCoreListFilterMap()
{

}

//       filterMap2 : (a -> b -> Maybe c) -> List a -> List b -> List c
void swampCoreListFilterMap2()
{

}

// filter : (a -> Bool) -> List a -> List a
void swampCoreListFilter()
{

}


// filter2 : (a -> b -> Bool) -> List a -> List b -> List b
void swampCoreListFilter2()
{

}

//remove : (a -> Bool) -> List a -> List a
void swampCoreListRemove()
{

}

// remove2 : (a -> b -> Bool) -> List a -> List b -> List b
void swampCoreListRemove2()
{

}

// concatMap : (a -> List b) -> List a -> List b
void swampCoreListConcatMap()
{

}

// concat : List (List a) -> List a
void swampCoreListConcat()
{

}


// range : Int -> Int -> List Int
void swampCoreListRange()
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

    CLOG_INFO("accumulator is now for index %d, value:%d", -1, *(const SwampInt32*)ownContext.bp);
    const uint8_t* sourceItemPointer = list->value;
    for (size_t i = 0; i < list->count; ++i) {
        SwampFunc* internalFunction;
        SwampMemoryPosition pos = swampExecutePrepare(fn, ownContext.bp, &internalFunction);

        SwampResult fnResult;
        fnResult.expectedOctetSize = bSize;

        SwampParameters parameters;
        parameters.parameterCount = 2;
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


        CLOG_INFO("calling foldl for index %d, list item:%d", i, *(const SwampInt32*)sourceItemPointer);
        swampRun(&fnResult, &ownContext, fn, parameters, 1);
        CLOG_INFO("accumulator is now for index %d, value:%d", i, *(const SwampInt32*)ownContext.bp);
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
void swampCoreListUnzip()
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
        {"List.indexedMap", swampCoreListIndexedMap},
        {"List.filterMap", swampCoreListFilterMap},
        {"List.foldl", swampCoreListFoldl},
        {"List.foldlstop", swampCoreListFoldlStop},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName)) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
