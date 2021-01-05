/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_deserialize_h
#define swamp_deserialize_h

struct swamp_value;
struct swamp_allocator;

#include <stddef.h>
#include <stdint.h>

const struct swamp_value* swamp_deserialize(struct swamp_allocator* allocator, const uint8_t** source,
                                            size_t octet_count);

#endif
