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
#include <swamp-dump/dump_ascii.h>
#include <tiny-libc/tiny_libc.h>

static int compactOrClone(void* v, const SwtiType* type, int doClone, SwampDynamicMemory* targetMemory, SwampUnmanagedMemory* targetUnmanagedMemory, SwampUnmanagedMemory* sourceUnmanagedMemory)
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
                int errorCode = compactOrClone((uint8_t*)v + field->memoryOffsetInfo.memoryOffset, field->fieldType, doClone, targetMemory, targetUnmanagedMemory, sourceUnmanagedMemory);
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
                int errorCode = compactOrClone((void*)(p + field->memoryOffsetInfo.memoryOffset), field->fieldType, doClone, targetMemory, targetUnmanagedMemory, sourceUnmanagedMemory);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeArray: {
            const SwtiArrayType* arrayType = ((const SwtiArrayType*) type);
            SwampArray** _array = (SwampArray**) v;
            const SwampArray* array = *_array;
            SwampArray* newArrayStruct = swampDynamicMemoryAllocDebug(targetMemory, 1, sizeof(SwampArray), 8,
                                                                      "SwampArray");

            void* newItems = swampDynamicMemoryAllocDebug(targetMemory, array->count, array->itemSize,
                                                                array->itemAlign, "array items");
            tc_memcpy_octets(newItems, array->value, array->count * array->itemSize);
            *newArrayStruct = *array;
            newArrayStruct->value = newItems;
            uint8_t* p = (uint8_t* ) newArrayStruct->value;
            for (size_t i = 0; i < newArrayStruct->count; i++) {
                int errorCode = compactOrClone((void*)p, arrayType->itemType, doClone, targetMemory, targetUnmanagedMemory, sourceUnmanagedMemory);
                if (errorCode != 0) {
                    return errorCode;
                }

                p += newArrayStruct->itemSize;
            }

            *_array = newArrayStruct;
        } break;
        case SwtiTypeList: {
            const SwtiListType* listType = ((const SwtiListType*) type);
            SwampList** _list = (SwampList**) v;
            const SwampList* list = *_list;
            SwampList* newArrayStruct = swampDynamicMemoryAllocDebug(targetMemory, 1, sizeof(SwampList), 8,
                                                                      "SwampList");

            void* newItems = swampDynamicMemoryAllocDebug(targetMemory, list->count, list->itemSize,
                                                                list->itemAlign, "list items");
            tc_memcpy_octets(newItems, list->value, list->count * list->itemSize);
            *newArrayStruct = *list;
            newArrayStruct->value = newItems;
            uint8_t* p = (uint8_t*) newArrayStruct->value;
            for (size_t i = 0; i < newArrayStruct->count; i++) {
                int errorCode = compactOrClone((void*)p, listType->itemType, doClone, targetMemory, targetUnmanagedMemory, sourceUnmanagedMemory);
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
            const SwampString** _str = (const SwampString**) v;
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
            SwampUnmanaged** _unmanaged = (SwampUnmanaged**) v;
            const SwampUnmanaged* unmanaged = *_unmanaged;

            if (doClone) {
                CLOG_VERBOSE("attempting to clone unmanaged  (%p)", unmanaged);
                CLOG_VERBOSE("attempting to clone unmanaged '%s' (%p)", unmanaged->debugName, unmanaged);
                int result = unmanaged->clone(_unmanaged, targetMemory, targetUnmanagedMemory);
                if (*_unmanaged == 0) {
                    CLOG_ERROR("what happened");
                }
                if (result < 0) {
                    return result;
                }
            } else {
#if 0
            CLOG_VERBOSE("attempting to compact unmanaged '%s' (%p)", unmanaged->debugName, unmanaged);
#endif
                swampUnmanagedMemoryMove(targetUnmanagedMemory, sourceUnmanagedMemory, unmanaged);
                if (*_unmanaged == 0) {
                    CLOG_ERROR("what happened");
                }
                return 0; // unmanaged->compact(_unmanaged, targetMemory, unmanagedMemory);
            }
        } break;
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) type;
            int errorCode = compactOrClone(v, alias->targetType, doClone, targetMemory, targetUnmanagedMemory, sourceUnmanagedMemory);
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

int swampCompact(const void* state, const SwtiType* stateType, SwampDynamicMemory* targetMemory, SwampUnmanagedMemory* targetUnmanagedMemory,  SwampUnmanagedMemory* sourceUnmanagedMemory, void** compactedState)
{
    #if 1
    if (!swampIsBlittableOrEcs(stateType)) {
        CLOG_ERROR("in this version, only blittable states and Ecs.World can be compacted %s", stateType->name);
        return -3;
    }
    #endif
    if (targetMemory->p != targetMemory->memory) {
        CLOG_ERROR("target memory must be reset");
        return -2;
    }


    SwtiMemorySize size = swtiGetMemorySize(stateType);
    SwtiMemoryAlign align = swtiGetMemoryAlign(stateType);

    void* compactedStateMemory = swampDynamicMemoryAllocDebug(targetMemory, 1, size, align, "state");
    tc_memcpy_octets(compactedStateMemory, state, size);
    if (compactedState) {
        *compactedState = compactedStateMemory;
    }

    int result = compactOrClone(compactedStateMemory, stateType, 0, targetMemory, targetUnmanagedMemory, sourceUnmanagedMemory);


    return result;
}

int swampClone(const void* state, const SwtiType* stateType, SwampDynamicMemory* targetMemory, SwampUnmanagedMemory* targetUnmanagedMemory, SwampUnmanagedMemory* sourceUnmanagedMemory, void** clonedState)
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
    if (clonedState) {
        *clonedState = clonedStateMemory;
    }

    int result = compactOrClone(clonedStateMemory, stateType, 1, targetMemory, targetUnmanagedMemory, sourceUnmanagedMemory);

    return result;
}
