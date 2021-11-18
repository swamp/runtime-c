/*---------------------------------------------------------------------------------------------
*  Copyright (c) Peter Bjorklund. All rights reserved.
*  Licensed under the MIT License. See LICENSE in the project root for license information.
*--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/context.h>
#include <tiny-libc/tiny_libc.h>
#include <swamp-runtime/swamp_allocate.h>

void swampCoreStringFromInt(SwampString** result, SwampMachineContext* context, const SwampInt32* intValue)
{
    static char temp[64];

    tc_snprintf(temp, 64, "%d", *intValue);

    const SwampString* s = swampStringAllocate(context->dynamicMemory, temp);

    *result = s;
}

void* swampCoreStringFindFunction(const char* fullyQualifiedName)
{
   SwampBindingInfo info[] = {
       {"String.fromInt", swampCoreStringFromInt},
   };

   for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
       if (tc_str_equal(info[i].name, fullyQualifiedName)) {
           return info[i].fn;
       }
   }
   // SWAMP_LOG_INFO("didn't find: %s", function_name);
   return 0;
}
