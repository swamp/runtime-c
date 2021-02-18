/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_core_map_h
#define swamp_core_map_h

#include <swamp-runtime/swamp.h>

SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_map);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_map2);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_indexed_map);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_any);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_list_find);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_list_member);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_filter);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_filter_2);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_filter_map);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_filter_map_2);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_remove);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_remove_2);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_mapcat);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_first);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_concat);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_empty);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_second);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_range);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_length);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_nth);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_rest);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_head);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_reduce);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_foldl);
SWAMP_FUNCTION_EXPOSE_DECLARE(swamp_core_reduce_stop);

#endif
