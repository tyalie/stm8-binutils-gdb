/* Disassemble STM8 instructions.
   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Contributed by Ake Rehnman

   This file is part of libopcodes.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"

#include "dis-asm.h"
#include "libiberty.h"
#include "opintl.h"
#include <assert.h>

#include "opcode/stm8.h"

disassemble_info *dinfo;
bfd_vma daddr;
int instrlen;
bfd_vma lastlabeladdr;
unsigned char buffer[16];

int stm8_dis (bfd_vma addr, unsigned int op);
int stm8_operands (char *s, unsigned char buf[], stm8_addr_mode_t arg);
const char *find_symbol (unsigned int addr);

static int
fetch_data (unsigned char *buf, bfd_vma addr, disassemble_info *info, int n)
{
  int r;

  r = info->read_memory_func (addr, buf, n, info);
  return r;
}

const char *
find_symbol (unsigned int addr)
{
  int i;

  for (i = 0; i < (dinfo->symtab_size); i++)
    {
      if ((dinfo->symtab[i]->value + dinfo->symtab[i]->section->vma) == addr)
        return dinfo->symtab[i]->name;
    }
  lastlabeladdr = addr;
  return 0;
}

int
stm8_operands (char *s, unsigned char buf[], stm8_addr_mode_t arg)
{
  unsigned int val;
  const char *sym;

  switch (arg)
    {
    case ST8_REG_A:
      sprintf (s, "A");
      break;
    case ST8_REG_X:
      sprintf (s, "X");
      break;
    case ST8_REG_Y:
      sprintf (s, "Y");
      break;
    case ST8_REG_SP:
      sprintf (s, "SP");
      break;
    case ST8_REG_CC:
      sprintf (s, "CC");
      break;
    case ST8_REG_XL:
      sprintf (s, "XL");
      break;
    case ST8_REG_XH:
      sprintf (s, "XH");
      break;
    case ST8_REG_YL:
      sprintf (s, "YL");
      break;
    case ST8_REG_YH:
      sprintf (s, "YH");
      break;
    case ST8_BIT_0:
      sprintf (s, "#0");
      break;
    case ST8_BIT_1:
      sprintf (s, "#1");
      break;
    case ST8_BIT_2:
      sprintf (s, "#2");
      break;
    case ST8_BIT_3:
      sprintf (s, "#3");
      break;
    case ST8_BIT_4:
      sprintf (s, "#4");
      break;
    case ST8_BIT_5:
      sprintf (s, "#5");
      break;
    case ST8_BIT_6:
      sprintf (s, "#6");
      break;
    case ST8_BIT_7:
      sprintf (s, "#7");
      break;
    case ST8_BYTE:
      val = buf[0];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "#%s", sym);
      else
        sprintf (s, "#0x%2.2x", val);
      return 1;
      break;
    case ST8_WORD:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "#%s", sym);
      else
        sprintf (s, "#0x%4.4x", val);
      return 2;
      break;
    case ST8_PCREL:
      val = (char)buf[0];
      val += (daddr + instrlen);
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "%s", sym);
      else
        sprintf (s, "0x%4.4x", val);
      return 1;
      break;
    case ST8_SHORTMEM:
      val = buf[0];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "%s.s", sym);
      else
        sprintf (s, "0x%2.2x", val);
      return 1;
      break;
    case ST8_INDX:
      sprintf (s, "(X)");
      break;
    case ST8_SHORTOFF_X:
      val = buf[0];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "(%s.s,X)", sym);
      else
        sprintf (s, "(0x%2.2x,X)", val);
      return 1;
      break;
    case ST8_INDY:
      sprintf (s, "(Y)");
      break;
    case ST8_SHORTOFF_Y:
      val = buf[0];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "(%s.s,Y)", sym);
      else
        sprintf (s, "(0x%2.2x,Y)", val);
      return 1;
      break;
    case ST8_SHORTOFF_SP:
      val = buf[0];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "(%s,SP)", sym);
      else
        sprintf (s, "(0x%2.2x,SP)", val);
      return 1;
      break;
    case ST8_SHORTPTRW:
      val = buf[0];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "[%s.s]", sym);
      else
        sprintf (s, "[0x%2.2x]", val);
      return 1;
      break;
    case ST8_SHORTPTRW_X:
      val = buf[0];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "([%s.s],X)", sym);
      else
        sprintf (s, "([0x%2.2x],X)", val);
      return 1;
      break;
    case ST8_SHORTPTRW_Y:
      val = buf[0];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "([%s.s],Y)", sym);
      else
        sprintf (s, "([0x%2.2x],Y)", val);
      return 1;
      break;
    case ST8_LONGMEM:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "%s", sym);
      else
        sprintf (s, "0x%4.4x", val);
      return 2;
      break;
    case ST8_LONGOFF_X:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "(%s,X)", sym);
      else
        sprintf (s, "(0x%4.4x,X)", val);
      return 2;
      break;
    case ST8_LONGOFF_Y:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "(%s,Y)", sym);
      else
        sprintf (s, "(0x%4.4x,Y)", val);
      return 2;
      break;
    case ST8_LONGPTRW:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "[%s.w]", sym);
      else
        sprintf (s, "[0x%4.4x.w]", val);
      return 2;
      break;
    case ST8_LONGPTRW_X:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "([%s.w],X)", sym);
      else
        sprintf (s, "([0x%4.4x.w],X)", val);
      return 2;
      break;
    case ST8_LONGPTRW_Y:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "([%s.w],Y)", sym);
      else
        sprintf (s, "([0x%4.4x.w],Y)", val);
      return 2;
      break;
    case ST8_LONGPTRE:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "[%s.e]", sym);
      else
        sprintf (s, "[0x%4.4x.e]", val);
      return 2;
      break;
    case ST8_LONGPTRE_X:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "([%s.e],X)", sym);
      else
        sprintf (s, "([0x%4.4x.e],X)", val);
      return 2;
      break;
    case ST8_LONGPTRE_Y:
      val = (buf[0] << 8) + buf[1];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "([%s.e],Y)", sym);
      else
        sprintf (s, "([0x%4.4x.e],Y)", val);
      return 2;
      break;
    case ST8_EXTMEM:
      val = (buf[0] << 16) + (buf[1] << 8) + buf[2];
      //		if (find_symbol_x(val))
      //			dinfo->print_address_func(val,dinfo);
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "%s", sym);
      else
        sprintf (s, "0x%6.6x", val);
      return 3;
      break;
    case ST8_EXTOFF_X:
      val = (buf[0] << 16) + (buf[1] << 8) + buf[2];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "(%s,X)", sym);
      else
        sprintf (s, "(0x%6.6x,X)", val);
      return 3;
      break;
    case ST8_EXTOFF_Y:
      val = (buf[0] << 16) + (buf[1] << 8) + buf[2];
      sym = find_symbol (val);
      if (sym)
        sprintf (s, "(%s,Y)", sym);
      else
        sprintf (s, "(0x%6.6x,Y)", val);
      return 3;
      break;
    case ST8_END:
      break;
    }
  return 0;
}

int
stm8_dis (bfd_vma addr, unsigned int op)
{
  unsigned char buf[8];
  unsigned char *bufp;
  char s[256];
  int i = 0;
  char c;
  int operandlen;
  int operand, dir;

  while (stm8_opcodes[i].name)
    {
      if (op == stm8_opcodes[i].bin_opcode)
        {
          dinfo->fprintf_func (dinfo->stream, "%s", stm8_opcodes[i].name);
          operandlen = stm8_compute_insn_size (stm8_opcodes[i])
                       - stm8_opcode_size (op);
          instrlen += operandlen;
          if (fetch_data (buf, addr, dinfo, operandlen))
            return 0;

          lastlabeladdr = 0;
          c = ' ';

          for (int curr_operand = 0;
               curr_operand < stm8_num_opcode_operands (stm8_opcodes[i]);
               curr_operand++)
            {
              bufp = buf;
              dir = 1;
              operand = 0;

              /* mov insn operands are reversed */
              if ((op == 0x35) || (op == 0x45) || (op == 0x55))
                {
                  dir = -1;
                  operand = stm8_num_opcode_operands (stm8_opcodes[i]) - 1;
                }

              for (int j = 0; j < stm8_num_opcode_operands (stm8_opcodes[i]);
                   j++, operand += dir)
                {
                  s[0] = 0;
                  bufp += stm8_operands (s, bufp,
                                         stm8_opcodes[i].constraints[operand]);
                  if (operand == curr_operand)
                    {
                      if (s[0])
                        {
                          dinfo->fprintf_func (dinfo->stream, "%c%s", c, s);
                          c = ',';
                        }
                      break;
                    }
                }
            }

          if (lastlabeladdr)
            {
              dinfo->fprintf_func (dinfo->stream, " ;");
              dinfo->print_address_func (lastlabeladdr, dinfo);
            }
          return operandlen;
        }
      i++;
    }
  return 0;
}

#define PDY 0x90
#define PIX 0x92
#define PIY 0x91
#define PWSP 0x72

int
print_insn_stm8 (bfd_vma addr, disassemble_info *info)
{
  unsigned int op;

  instrlen = 0;
  dinfo = info;
  daddr = addr;
  if (!fetch_data (buffer, addr, info, 1))
    {
      op = buffer[0];
      instrlen++;
      if ((buffer[0] == PDY) || (buffer[0] == PIX) || (buffer[0] == PIY)
          || (buffer[0] == PWSP))
        {
          if (fetch_data (buffer, addr, info, 2))
            return -1;
          instrlen++;
          op = buffer[0] << 8;
          op += buffer[1];
        }
      stm8_dis (addr + instrlen, op);
    }
  return instrlen;
}
