/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef swamp_opcodes_h
#define swamp_opcodes_h

#include <stdint.h>

// -------------------------------------------------------------
// Opcodes
// -------------------------------------------------------------
#define swamp_opcode_create_struct 0x01
#define swamp_opcode_update_struct 0x02
#define swamp_opcode_struct_get 0x03
#define swamp_opcode_reg_to_reg 0x04
#define swamp_opcode_list_conj 0x05
// -------------------------------------------------------------
#define swamp_opcode_enum_case 0x06
#define swamp_opcode_branch_false 0x07
#define swamp_opcode_jump 0x08
// -------------------------------------------------------------
#define swamp_opcode_call 0x09
#define swamp_opcode_return 0x0a
#define swamp_opcode_call_external 0x0b
#define swamp_opcode_call_external_with_id 0x25
#define swamp_opcode_tail_call 0x0c
// -------------------------------------------------------------
#define swamp_opcode_int_add 0x0d
#define swamp_opcode_int_sub 0x0e
#define swamp_opcode_int_mul 0x0f
#define swamp_opcode_int_div 0x10
// -------------------------------------------------------------
#define swamp_opcode_int_eql 0x11
#define swamp_opcode_int_neql 0x12
#define swamp_opcode_int_less 0x13
#define swamp_opcode_int_lessequal 0x14
#define swamp_opcode_int_greater 0x15
#define swamp_opcode_int_gte 0x16
// -------------------------------------------------------------
#define swamp_opcode_int_and 0x17
#define swamp_opcode_int_or 0x18
#define swamp_opcode_int_xor 0x19
#define swamp_opcode_int_not 0x1a
// -------------------------------------------------------------
#define swamp_opcode_bool_not 0x1b
#define swamp_opcode_branch_true 0x1c
#define swamp_opcode_enum_case_pattern_matching 0x1d
#define swamp_opcode_cmp_equal 0x1e
#define swamp_opcode_cmp_not_equal 0x1f
#define swamp_opcode_curry 0x20
#define swamp_opcode_create_list 0x21
#define swamp_opcode_list_append 0x22
#define swamp_opcode_create_enum 0x23
#define swamp_opcode_string_append 0x24
// -------------------------------------------------------------
#define swamp_opcode_fixed_mul 0x25
#define swamp_opcode_fixed_div 0x26
#define swamp_opcode_int_negate 0x27
#define swamp_opcode_struct_split 0x28
// -------------------------------------------------------------

// -------------------------------------------------------------
// -------------------------------------------------------------
#define swamp_opcode_int_shl 0xf8
#define swamp_opcode_int_shr 0xf9
#define swamp_opcode_int_mod 0xfA
// -------------------------------------------------------------
// -------------------------------------------------------------

#endif
