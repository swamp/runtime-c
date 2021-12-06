/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-runtime/blittable.h>
#include <swamp-runtime/compact.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/types.h>
#include <swamp-typeinfo/typeinfo.h>
#include <tiny-libc/tiny_libc.h>

static int compactOrClone(void* v, const SwtiType* type, int doClone, SwampDynamicMemory* targetMemory)
{
    switch (type->type) {
        case SwtiTypeBoolean:
        case SwtiTypeInt:
        case SwtiTypeFixed:
        case SwtiTypeChar:
            return 0;
        case SwtiTypeRecord: {
            const SwtiRecordType* record = (const SwtiRecordType*) type;
            for (size_t i = 0; i < record->fieldCount; i++) {
                const SwtiRecordTypeField* field = &record->fields[i];
                int errorCode = compactOrClone((uint8_t*)v + field->memoryOffsetInfo.memoryOffset, field->fieldType, doClone, targetMemory);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) type;
            const uint8_t* p = (const uint8_t*) v;
            if (*p >= custom->variantCount) {
                CLOG_ERROR("illegal variant index %d", *p);
            }
            const SwtiCustomTypeVariant* variant = &custom->variantTypes[*p];
            p++;
            for (size_t i = 0; i < variant->paramCount; ++i) {
                const SwtiCustomTypeVariantField* field = &variant->fields[i];
                int errorCode = compactOrClone(p + field->memoryOffsetInfo.memoryOffset, field->fieldType, doClone, targetMemory);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeArray: {
            const SwtiArrayType* arrayType = ((const SwtiArrayType*) type);
            SwampArray** _array = (const SwampArray**) v;
            const SwampArray* array = *_array;
            SwampArray* newArrayStruct = swampDynamicMemoryAllocDebug(targetMemory, 1, sizeof(SwampArray), 8,
                                                                      "SwampArray");

            const void* newItems = swampDynamicMemoryAllocDebug(targetMemory, array->count, array->itemSize,
                                                                array->itemAlign, "array items");
            tc_memcpy_octets(newItems, array->value, array->count * array->itemSize);
            *newArrayStruct = *array;
            newArrayStruct->value = newItems;
            const uint8_t* p = newArrayStruct->value;
            for (size_t i = 0; i < newArrayStruct->count; i++) {
                int errorCode = compactOrClone(p, arrayType->itemType, doClone, targetMemory);
                if (errorCode != 0) {
                    return errorCode;
                }

                p += newArrayStruct->itemSize;
            }

            *_array = newArrayStruct;
        } break;
        case SwtiTypeList: {
            const SwtiListType* listType = ((const SwtiListType*) type);
            SwampList** _list = (const SwampList**) v;
            const SwampList* list = *_list;
            SwampList* newArrayStruct = swampDynamicMemoryAllocDebug(targetMemory, 1, sizeof(SwampList), 8,
                                                                      "SwampList");

            const void* newItems = swampDynamicMemoryAllocDebug(targetMemory, list->count, list->itemSize,
                                                                list->itemAlign, "list items");
            tc_memcpy_octets(newItems, list->value, list->count * list->itemSize);
            *newArrayStruct = *list;
            newArrayStruct->value = newItems;
            const uint8_t* p = newArrayStruct->value;
            for (size_t i = 0; i < newArrayStruct->count; i++) {
                int errorCode = compactOrClone(p, listType->itemType, doClone, targetMemory);
                if (errorCode != 0) {
                    return errorCode;
                }

                p += newArrayStruct->itemSize;
            }

            *_list = newArrayStruct;
        } break;
        case SwtiTypeFunction:
            return -1;
        case SwtiTypeString: {
            SwampString** _str = (const SwampString**) v;
            const SwampString* str = *_str;
            SwampString* newStrStruct = swampDynamicMemoryAllocDebug(targetMemory, 1, sizeof(SwampString), 8,
                                                                     "SwampString");
            char* newCharacters = swampDynamicMemoryAllocDebug(targetMemory, str->characterCount + 1, 1, 1, "characters");
            tc_memcpy_octets(newCharacters, str->characters, str->characterCount + 1);
            newStrStruct->characters = newCharacters;
            newStrStruct->characterCount = str->characterCount;
            *_str = newStrStruct;
        } break;
        case SwtiTypeAny:
        case SwtiTypeAnyMatchingTypes:
        case SwtiTypeResourceName:
            CLOG_ERROR("not supported in this version");
            break;
        case SwtiTypeUnmanaged: {
            const SwampUnmanaged** _unmanaged = (const SwampUnmanaged**) v;
            const SwampUnmanaged* unmanaged = *_unmanaged;

            if (doClone) {
                return unmanaged->clone(_unmanaged, targetMemory);
            } else {
                return unmanaged->compact(_unmanaged, targetMemory);
            }
        } break;
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) type;
            int errorCode = compactOrClone(v, alias->targetType, doClone, targetMemory);
            if (errorCode != 0) {
                return errorCode;
            }

        } break;
        case SwtiTypeBlob: {
            SwampBlob** _blob = (SwampBlob**) v;
            const SwampBlob* sourceBlob = *_blob;
            SwampBlob* newBlobStruct = swampDynamicMemoryAllocDebug(targetMemory, 1, sizeof(SwampBlob), 8, "SwampBlob");
            uint8_t* octets = swampDynamicMemoryAllocDebug(targetMemory, 1, sourceBlob->octetCount, 1, "blob");
            tc_memcpy_octets(octets, sourceBlob->octets, sourceBlob->octetCount);
            *newBlobStruct = *sourceBlob;
            newBlobStruct->octets = octets;
            *_blob = newBlobStruct;

        } break;

        default: {
            CLOG_ERROR("unknown type %d", type->type);
        }
    }

    return 0;
}

int swampCompact(void* state, const SwtiType* stateType, const SwampDynamicMemory* targetMemory, void** compactedState)
{
    #if 1
    if (!swampIsBlittableOrEcs(stateType)) {
        CLOG_ERROR("in this version, only blittable states and Ecs.World can be compacted %s", stateType->name);
    }
    #endif
    if (targetMemory->p != targetMemory->memory) {
        CLOG_ERROR("target memory must be reset");
    }

    SwtiMemorySize size = swtiGetMemorySize(stateType);
    SwtiMemoryAlign align = swtiGetMemoryAlign(stateType);

    void* compactedStateMemory = swampDynamicMemoryAllocDebug(targetMemory, 1, size, align, "state");
    tc_memcpy_octets(compactedStateMemory, state, size);
    *compactedState = compactedStateMemory;

    return compactOrClone(compactedStateMemory, stateType, 0, targetMemory);
}

int swampClone(void* state, const SwtiType* stateType, const SwampDynamicMemory* targetMemory, void** clonedState)
{
    if (!swampIsBlittableOrEcs(stateType)) {
        CLOG_ERROR("in this version, only blittable states and Ecs.World can be compacted");
    }

    if (targetMemory->p != targetMemory->memory) {
        CLOG_ERROR("target memory must be reset");
    }

    SwtiMemorySize size = swtiGetMemorySize(stateType);
    SwtiMemoryAlign align = swtiGetMemoryAlign(stateType);

    void* clonedStateMemory = swampDynamicMemoryAllocDebug(targetMemory, 1, size, align, "state");
    tc_memcpy_octets(clonedStateMemory, state, size);
    *clonedState = clonedStateMemory;

    return compactOrClone(clonedStateMemory, stateType, 1, targetMemory);
}
