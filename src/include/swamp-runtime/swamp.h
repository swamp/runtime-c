/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_h
#define swamp_h

#include <swamp-runtime/types.h>
#include <swamp-runtime/dynamic_memory.h>
#include <swamp-runtime/stack_memory.h>

struct SwampMachineContext;

int swampRun(SwampResult* result, struct SwampMachineContext* context, const SwampFunc* f, SwampParameters run_parameters,
             SwampBool verbose_flag);


#endif
