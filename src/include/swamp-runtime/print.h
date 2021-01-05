/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_print_h
#define swamp_print_h

struct swamp_value;

void swamp_value_print(const struct swamp_value* v, const char* prefix);
void swamp_value_print_output(const struct swamp_value* v);
void swamp_value_print_index(const struct swamp_value* v, int index);

#endif
