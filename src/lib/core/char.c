/*---------------------------------------------------------------------------------------------
*  Copyright (c) Peter Bjorklund. All rights reserved.
*  Licensed under the MIT License. See LICENSE in the project root for license information.
*--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/context.h>
#include <tiny-libc/tiny_libc.h>

void swampCoreCharFromCode(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* code)
{
   *result = *code;
}

void swampCoreCharOrd(SwampInt32* result, SwampMachineContext* context, const SwampInt32 * charValue)
{
   *result = *charValue;
}

void swampCoreCharToCode(SwampInt32* result, SwampMachineContext* context, const SwampInt32 * charValue)
{
    *result = *charValue;
}

void* swampCoreCharFindFunction(const char* fullyQualifiedName)
{
   SwampBindingInfo info[] = {
       {"Char.fromCode", swampCoreCharFromCode},
        {"Char.toCode", swampCoreCharToCode},
       {"Char.ord", swampCoreCharOrd},
   };

   for (size_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
       if (tc_str_equal(info[i].name, fullyQualifiedName)) {
           return info[i].fn;
       }
   }
   // SWAMP_LOG_INFO("didn't find: %s", function_name);
   return 0;
}
