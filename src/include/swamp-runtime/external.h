/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_EXTERNAL_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_EXTERNAL_H

#define SWAMP_FUNCTION(NAME)                                                                                           \
    static const void* NAME(struct swamp_machine_context* allocator, const void** arguments, int argument_count)
#define SWAMP_FUNCTION_EXPOSE(NAME)                                                                                    \
    const void* NAME(struct swamp_machine_context* allocator, const void** arguments, int argument_count)
#define SWAMP_FUNCTION_EXPOSE_DECLARE(NAME)                                                                            \
    const void* NAME(struct swamp_machine_context* machine_context, const void** arguments, int argument_count)

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_EXTERNAL_H
