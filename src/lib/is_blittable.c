/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-typeinfo/typeinfo.h>
#include <tiny-libc/tiny_libc.h>

int swampIsBlittableOrEcs(const SwtiType* maybeBlittableType)
{
    const SwtiType* unaliasedType = swtiUnalias(maybeBlittableType);
    switch (unaliasedType->type) {
        case SwtiTypeArray: {
            const SwtiArrayType* arrayType = ((const SwtiArrayType*) unaliasedType);
            return swampIsBlittableOrEcs(arrayType->itemType);
        } break;
        case SwtiTypeList: {
            const SwtiListType* listType = ((const SwtiListType*) unaliasedType);
            return swampIsBlittableOrEcs(listType->itemType);
        } break;
        case SwtiTypeResourceName:
            return 0;
        case SwtiTypeUnmanaged: {
            const SwtiUnmanagedType* unmanaged = (const SwtiUnmanagedType*) unaliasedType;
            if (!tc_str_equal(unmanaged->internal.name, "EcsWorld")) {
                return 0;
            }
            return 1;
        } break;
        case SwtiTypeFunction: {
            const SwtiFunctionType* fn = (const SwtiFunctionType*) unaliasedType;
            for (size_t i = 0; i < fn->parameterCount; ++i) {
                if (!swampIsBlittableOrEcs(fn->parameterTypes[i])) {
                    return 0;
                }
            }
            return 1;
        } break;
        case SwtiTypeTuple: {
            const SwtiTupleType* tuple = (const SwtiTupleType*) unaliasedType;
            for (size_t i = 0; i < tuple->fieldCount; ++i) {
                if (!swampIsBlittableOrEcs(tuple->fields[i].fieldType)) {
                    return 0;
                }
            }
            return 1;
        } break;
        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) unaliasedType;
            for (size_t i = 0; i < custom->variantCount; ++i) {
                for (size_t j = 0; j < custom->variantTypes[i].paramCount; ++j) {
                    if (!swampIsBlittableOrEcs(custom->variantTypes[i].fields[j].fieldType)) {
                        return 0;
                    }
                }
            }
            return 1;
        } break;
        case SwtiTypeRecord: {
            const SwtiRecordType* record = (const SwtiRecordType*) unaliasedType;
            for (size_t i = 0; i < record->fieldCount; ++i) {
                if (!swampIsBlittableOrEcs(record->fields[i].fieldType)) {
                    return 0;
                }
            }
            return 1;
        } break;
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) unaliasedType;
            return swampIsBlittableOrEcs(alias->targetType);
        } break;
        case SwtiTypeInt:
        case SwtiTypeFixed:
        case SwtiTypeChar:
        case SwtiTypeBoolean:
        case SwtiTypeString:
        case SwtiTypeBlob:
            return 1;
    }

    return 0;
}