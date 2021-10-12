/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/context.h>
#include <swamp-typeinfo/typeinfo.h>
#include <tiny-libc/tiny_libc.h>

int swampIsBlittableOrEcs(const SwtiType* stateType)
{
    switch (stateType->type) {
        case SwtiTypeArray: {
            const SwtiArrayType* arrayType = ((const SwtiArrayType*) stateType);
            return swampIsBlittableOrEcs(arrayType->itemType);
        } break;
        case SwtiTypeList: {
            const SwtiListType* listType = ((const SwtiListType*) stateType);
            return swampIsBlittableOrEcs(listType->itemType);
        } break;
        case SwtiTypeResourceName:
            return 0;
        case SwtiTypeUnmanaged: {
            const SwtiUnmanagedType* unmanaged = (const SwtiUnmanagedType*) stateType;
            if (!tc_str_equal(unmanaged->internal.name, "EcsWorld")) {
                return 0;
            }
        } break;
        case SwtiTypeFunction: {
            const SwtiFunctionType* fn = (const SwtiFunctionType*) stateType;
            for (size_t i = 0; i < fn->parameterCount; ++i) {
                if (!swampIsBlittableOrEcs(fn->parameterTypes[i])) {
                    return 0;
                }
            }
        } break;
        case SwtiTypeTuple: {
            const SwtiTupleType* tuple = (const SwtiTupleType*) stateType;
            for (size_t i = 0; i < tuple->fieldCount; ++i) {
                if (!swampIsBlittableOrEcs(tuple->fields[i].fieldType)) {
                    return 0;
                }
            }
        } break;
        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) stateType;
            for (size_t i = 0; i < custom->variantCount; ++i) {
                for (size_t j = 0; j < custom->variantTypes[i].paramCount; ++j) {
                    if (!swampIsBlittableOrEcs(custom->variantTypes[i].fields[j].fieldType)) {
                        return 0;
                    }
                }
            }
        } break;
        case SwtiTypeRecord: {
            const SwtiRecordType* record = (const SwtiRecordType*) stateType;
            for (size_t i = 0; i < record->fieldCount; ++i) {
                if (!swampIsBlittableOrEcs(record->fields[i].fieldType)) {
                    return 0;
                }
            }
        } break;
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) stateType;
            return swampIsBlittableOrEcs(alias->targetType);
        } break;
    }

    return 1;
}