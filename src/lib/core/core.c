/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/array.h>
#include <swamp-runtime/core/blob.h>
#include <swamp-runtime/core/char.h>
#include <swamp-runtime/core/string.h>
#include <swamp-runtime/core/debug.h>
#include <swamp-runtime/core/int.h>
#include <swamp-runtime/core/list.h>
#include <swamp-runtime/core/math.h>
#include <swamp-runtime/core/maybe.h>
#include <swamp-runtime/core/bind.h>

const void* swampCoreFindFunction(const char* fullyQualifiedName)
{
    const void* foundFn = swampCoreMathFindFunction(fullyQualifiedName);
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

    foundFn = swampCoreBlobFindFunction(fullyQualifiedName);
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

    foundFn = swampCoreCharFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }

    foundFn = swampCoreStringFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }

    foundFn = swampCoreDebugFindFunction(fullyQualifiedName);
    if (foundFn) {
        return foundFn;
    }

    return 0;
}
