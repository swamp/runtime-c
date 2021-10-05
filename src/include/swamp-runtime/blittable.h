/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_BLITTABLE_H
#define SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_BLITTABLE_H

struct SwtiType;

int swampIsBlittableOrEcs(const struct SwtiType* stateType);

#endif // SWAMP_RUNTIME_SRC_INCLUDE_SWAMP_RUNTIME_BLITTABLE_H
