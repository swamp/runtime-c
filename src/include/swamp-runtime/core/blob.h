/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_blob_h
#define swamp_core_blob_h

#include <swamp-runtime/swamp.h>

// DONT CHANGE ORDER of fields, must be alphanumerical.
SWAMP_STRUCT(vec2)
    swamp_int* x;
    swamp_int* y;
SWAMP_STRUCT_END(vec2)

SWAMP_STRUCT(sz2)
    swamp_int* height;
    swamp_int* width;
SWAMP_STRUCT_END(sz2)

SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_is_empty);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_from_list);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_to_list);

SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_map);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_indexed_map);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_filter_map);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_filter_indexed_map);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_length);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_get);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_set);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_grab);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_get_2d);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_grab_2d);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_set_2d);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_blob_to_string_2d);

#endif
