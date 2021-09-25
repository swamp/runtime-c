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
#define SwampOpcodeEnumCase 0x01
#define SwampOpcodeBranchFalse 0x02
#define SwampOpcodeBranchTrue 0x03
#define SwampOpcodeJump 0x04
// -------------------------------------------------------------
#define SwampOpcodeCall 0x05
#define SwampOpcodeReturn 0x06
#define SwampOpcodeCallExternal 0x07
#define SwampOpcodeTailCall 0x08
#define SwampOpcodeCurry 0x09
// -------------------------------------------------------------
#define SwampOpcodeIntAdd 0x0a
#define SwampOpcodeIntSub 0x0b
#define SwampOpcodeIntMul 0x0c
#define SwampOpcodeIntDiv 0x0d
#define SwampOpcodeIntNegate 0x0e
#define SwampOpcodeFixedMul 0x0f
#define SwampOpcodeFixedDiv 0x10
// -------------------------------------------------------------
#define SwampOpcodeIntEqual 0x11
#define SwampOpcodeIntNotEqual 0x12
#define SwampOpcodeIntLess 0x13
#define SwampOpcodeIntLessEqual 0x14
#define SwampOpcodeIntGreater 0x15
#define SwampOpcodeIntGreaterOrEqual 0x16
// -------------------------------------------------------------
#define SwampOpcodeBoolNot 0x17
// -------------------------------------------------------------
#define SwampOpcodeStringEqual 0x18
#define SwampOpcodeStringNotEqual 0x19



#define SwampOpcodeIntAnd 0x1a
#define SwampOpcodeIntOr 0x1b
#define SwampOpcodeIntXor 0x1c
#define SwampOpcodeIntNot 0x1d
    // -------------------------------------------------------------

#define SwampOpcodeListCreate 0x1e
#define SwampOpcodeArrayCreate 0x1f
// -------------------------------------------------------------
#define SwampOpcodeListConj 0x20
#define SwampOpcodeListAppend 0x21
#define SwampOpcodeStringAppend 0x22
// -------------------------------------------------------------
#define SwampOpcodeLoadInteger 0x23
#define SwampOpcodeLoadBoolean 0x24
#define SwampOpcodeLoadRune 0x25
#define SwampOpcodeLoadZeroMemory 0x26
#define SwampOpcodeMemCopy 0x27
#define SwampOpcodeSetEnum 0x28
#define SwampOpcodeCallExternalWithSizes 0x29
#define SwampOpcodeCmpEnumEqual 0x2a
#define SwampOpcodeCmpEnumNotEqual 0x2b
#define SwampOpcodePatternMatchingInt 0x2c
#define SwampOpcodePatternMatchingString 0x2d
#define SwampOpcodeCallExternalWithExtendedSizes 0x2e
// -------------------------------------------------------------

// -------------------------------------------------------------

// -------------------------------------------------------------
// -------------------------------------------------------------
#define SwampOpcodeIntShiftLeft 0xf8
#define SwampOpcodeIntShiftRight 0xf9
#define SwampOpcodeIntMod 0xfA


// -------------------------------------------------------------
// -------------------------------------------------------------

#endif
