/* include/opcode/stm8.h -- Assembler for the STM8.
   Written by Ake Rehnman 2017-02-21,
   ake.rehnman (at) gmail dot com

   Copyright (C) 2007-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _STM8_H_
#define _STM8_H_

typedef enum
{
  ST8_END = 0,
  ST8_BIT_0,
  ST8_BIT_1,
  ST8_BIT_2,
  ST8_BIT_3,
  ST8_BIT_4,
  ST8_BIT_5,
  ST8_BIT_6,
  ST8_BIT_7,
  ST8_PCREL,
  ST8_REG_CC,
  ST8_REG_A,
  ST8_REG_X,
  ST8_REG_Y,
  ST8_REG_SP,
  ST8_REG_XL,
  ST8_REG_XH,
  ST8_REG_YL,
  ST8_REG_YH,
  ST8_BYTE,     // IMM8
  ST8_WORD,     // IMM16
  ST8_SHORTMEM, // DIR8
  ST8_LONGMEM,  // DIR16
  ST8_EXTMEM,   // DIR24
  ST8_INDX,
  ST8_INDY,
  ST8_SHORTOFF_X,
  ST8_LONGOFF_X,
  ST8_EXTOFF_X,
  ST8_SHORTOFF_Y,
  ST8_LONGOFF_Y,
  ST8_EXTOFF_Y,
  ST8_SHORTOFF_SP,
  ST8_SHORTPTRW,
  ST8_LONGPTRW,
  ST8_SHORTPTRW_X,
  ST8_LONGPTRW_X,
  ST8_SHORTPTRW_Y,
  ST8_LONGPTRW_Y,
  ST8_LONGPTRE,
  ST8_LONGPTRE_X,
  ST8_LONGPTRE_Y
} stm8_addr_mode_t;

struct stm8_opcodes_s
{
  const char *name;
  stm8_addr_mode_t constraints[5];
  unsigned int bin_opcode;
};

extern const struct stm8_opcodes_s stm8_opcodes[];

extern int stm8_compute_insn_size (struct stm8_opcodes_s opcode);

extern unsigned int stm8_opcode_size (unsigned int number);

#endif /* _STM8_H_ */
