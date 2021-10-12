/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/execute.h>
#include <swamp-runtime/types.h>
#include <tiny-libc/tiny_libc.h>

SwampMemoryPosition swampExecutePrepare(const SwampFunction* func, const void* bp, const SwampFunc** outFn)
{
    if (func->type == SwampFunctionTypeCurry) {
        const SwampCurryFunc* curry = (const SwampCurryFunc*) func;
        const SwampFunc* fn = curry->curryFunction;
        SwampMemoryPosition pos = fn->returnOctetSize;
        swampMemoryPositionAlign(&pos, curry->firstParameterAlign);
        tc_memcpy_octets(bp + pos, curry->curryOctets, curry->curryOctetSize);
        pos += curry->curryOctetSize;
        *outFn = curry->curryFunction;
        return pos;
    }

    if (func->type == SwampFunctionTypeInternal) {
        const SwampFunc* fn = (const SwampFunc*) func;
        *outFn = fn;
        return fn->returnOctetSize;
    }

    CLOG_ERROR("unknown function");
}
