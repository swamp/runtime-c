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
#define SwampOpcodeStructCreate 0x01
#define SwampOpcodeStructUpdate 0x02
#define SwampOpcodeListConj 0x05
// -------------------------------------------------------------
#define SwampOpcodeEnumCase 0x06
#define SwampOpcodeBranchFalse 0x07
#define SwampOpcodeJump 0x08
// -------------------------------------------------------------
#define SwampOpcodeCall 0x09
#define SwampOpcodeReturn 0x0a
#define SwampOpcodeCallExternal 0x0b
#define SwampOpcodeTailCall 0x0c
// -------------------------------------------------------------
#define SwampOpcodeIntAdd 0x0d
#define SwampOpcodeIntSub 0x0e
#define SwampOpcodeIntMul 0x0f
#define SwampOpcodeIntDiv 0x10
#define SwampOpcodeIntNegate 0x27
// -------------------------------------------------------------
#define SwampOpcodeIntEqual 0x11
#define SwampOpcodeIntNotEqual 0x12
#define SwampOpcodeIntLess 0x13
#define SwampOpcodeIntLessEqual 0x14
#define SwampOpcodeIntGreater 0x15
#define SwampOpcodeIntGreaterOrEqual 0x16
// -------------------------------------------------------------
#define SwampOpcodeIntAnd 0x17
#define SwampOpcodeIntOr 0x18
#define SwampOpcodeIntXor 0x19
#define SwampOpcodeIntNot 0x1a
// -------------------------------------------------------------
#define SwampOpcodeBoolNot 0x1b
#define SwampOpcodeBranchTrue 0x1c
#define SwampOpcodeCasePatternMatching 0x1d
#define SwampOpcodeCompareEqual 0x1e
#define SwampOpcodeCompareNotEqual 0x1f
#define SwampOpcodeCurry 0x20
#define SwampOpcodeListCreate 0x21
#define SwampOpcodeListAppend 0x22
#define SwampOpcodeStringAppend 0x24
// -------------------------------------------------------------
#define SwampOpcodeFixedMul 0x25
#define SwampOpcodeFixedDiv 0x26
// -------------------------------------------------------------

// -------------------------------------------------------------
// -------------------------------------------------------------
#define SwampOpcodeIntShiftLeft 0xf8
#define SwampOpcodeIntShiftRight 0xf9
#define SwampOpcodeIntMod 0xfA

#define SwampOpcodeCopyFromZeroMemory 0xfc
#define SwampOpcodeArrayCreate 0xfd
// -------------------------------------------------------------
// -------------------------------------------------------------

#endif
