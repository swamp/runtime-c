/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_log_h
#define swamp_log_h

#include <stdio.h>
#include <stdlib.h>

#define SWAMP_LOG_SHOULD_LOG(level) (level == 1)

#define SWAMP_LOG_INFO(...)                                                                                            \
    fprintf(stderr, __VA_ARGS__);                                                                                      \
    fprintf(stderr, "\n")

#define SWAMP_LOG_DEBUG(...)                                                                                           \
    fprintf(stderr, __VA_ARGS__);                                                                                      \
    fprintf(stderr, "\n")

#define SWAMP_LOG_ERROR(...)                                                                                           \
    fprintf(stderr, __VA_ARGS__);                                                                                      \
    fprintf(stderr, "\n");                                                                                             \
    abort()

#define SWAMP_LOG_SOFT_ERROR(...)                                                                                      \
    fprintf(stderr, __VA_ARGS__);                                                                                      \
    fprintf(stderr, "\n")
#define SWAMP_LOG_OUTPUT(...)                                                                                          \
    printf(__VA_ARGS__);                                                                                               \
    printf("\n")
#define SWAMP_LOG_FILE(f, ...) fprintf(f, __VA_ARGS__);
#define SWAMP_LOG_FILE_NL(f, ...)                                                                                      \
    fprintf(f, __VA_ARGS__);                                                                                           \
    fprintf(f, "\n")

#endif
