/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_h
#define swamp_h

#include <swamp-runtime/types.h>

typedef struct SwampMachineContext {
    uint8_t* stackMemory;
    size_t maximumStackMemory;
    uint8_t* sp;
    uint8_t* bp;
} SwampMachineContext;

int swampRun(SwampMachineContext* context, const SwampFunc* f, SwampParameters run_parameters,
             SwampResult* result, SwampBool verbose_flag);


#endif
