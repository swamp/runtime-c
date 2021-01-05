/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_serialize_h
#define swamp_serialize_h

struct swamp_value;
#include <stddef.h>
#include <stdint.h>

int swamp_serialize(const struct swamp_value* v, uint8_t** target, size_t max_size);

#endif
