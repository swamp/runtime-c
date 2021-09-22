/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/debug.h>
#include <swamp-runtime/core/int.h>
#include <swamp-runtime/core/list.h>
#include <swamp-runtime/core/array.h>
#include <swamp-runtime/core/math.h>
#include <swamp-runtime/core/maybe.h>

void* swampCoreFindFunction(const char* fullyQualifiedName)
{
    void * foundFn = swampCoreMathFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }

    foundFn = swampCoreListFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }

    foundFn = swampCoreArrayFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }

    foundFn = swampCoreMaybeFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }


    foundFn = swampCoreIntFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }

    foundFn = swampCoreDebugFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }



    return 0;
}
