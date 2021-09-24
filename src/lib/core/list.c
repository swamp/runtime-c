/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/core/bind.h>
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
void swampCoreListMap(SwampList** result, SwampMachineContext* context, SwampFunc** _fn, const SwampList** _list)
{
    const SwampList* list = *_list;
    const SwampFunc* fn = *_fn;
    const uint8_t* sourceItemPointer = list->value;

    SwampResult fnResult;
    fnResult.target = context->tempResult;
    fnResult.expectedOctetSize = fn->returnOctetSize;

    SwampParameters parameters;
    parameters.parameterCount = 1;
    parameters.octetSize = list->itemSize;

    SwampMachineContext ownContext;
    swampContextInit(&ownContext, context->dynamicMemory, context->typeInfo);

    SwampList* target = swampListAllocatePrepare(context->dynamicMemory, list->count, fn->returnOctetSize, fn->returnAlign);
    uint8_t* targetItemPointer = target->value;
    for (size_t i = 0; i < list->count; ++i) {
        parameters.source = sourceItemPointer;
        swampContextReset(&ownContext);
        CLOG_INFO("calling for index %d", i);
        swampRun(&fnResult, &ownContext, fn, parameters, 1);
        tc_memcpy_octets(targetItemPointer, fnResult.target, target->itemSize);
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




// foldl : (a -> b -> b) -> b -> List a -> b
void swampCoreListFoldl()
{

}

// unzip : List (a, b) -> (List a, List b)
void swampCoreListUnzip()
{

}

// foldlstop : (a -> b -> Maybe b) -> b -> List a -> b
void swampCoreListFoldlStop()
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
