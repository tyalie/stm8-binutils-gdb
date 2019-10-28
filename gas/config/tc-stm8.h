/* This file is tc-stm8.h
   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Written by Ake Rehnman 2017-02-21,
   ake.rehnman (at) gmail dot com

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* By convention, you should define this macro in the `.h' file.  For
   example, `tc-m68k.h' defines `TC_M68K'.  You might have to use this
   if it is necessary to add CPU specific code to the object format
   file.  */
#define TC_STM8

/* This macro is the BFD target name to use when creating the output
   file.  This will normally depend upon the `OBJ_FMT' macro.  */
#define TARGET_FORMAT "elf32-stm8"

/* This macro is the BFD architecture to pass to `bfd_set_arch_mach'.  */
#define TARGET_ARCH bfd_arch_stm8

/* This macro is the BFD machine number to pass to
   `bfd_set_arch_mach'.  If it is not defined, GAS will use 0.  */
#define TARGET_MACH 0

/* You should define this macro to be non-zero if the target is big
   endian, and zero if the target is little endian.  */
#define TARGET_BYTES_BIG_ENDIAN 1

/* If you define this macro, GAS will warn about the use of
   nonstandard escape sequences in a string.  */
#define ONLY_STANDARD_ESCAPES

#define DIFF_EXPR_OK /* .-foo gets turned into PC relative relocs */

/* If you define this macro, it should return the offset between the
   address of a PC relative fixup and the position from which the PC
   relative adjustment should be made.  On many processors, the base
   of a PC relative instruction is the next instruction, so this
   macro would return the length of an instruction.  */
#define MD_PCREL_FROM_SECTION(FIX, SEC) md_pcrel_from_section (FIX, SEC)
extern long md_pcrel_from_section (struct fix *, segT);

#define NEED_INDEX_OPERATOR
extern int stm8_need_index_operator (void);
//#define md_need_index_operator stm8_need_index_operator

#define NUMBERS_WITH_SUFFIX 1

//#define UNDEFINED_DIFFERENCE_OK
//#define TC_VALIDATE_FIX_SUB(fix, seg) stm8_validate_fix_sub (fix)
// extern int stm8_validate_fix_sub (struct fix *);

//#define STM8_FIXUP 128

extern operatorT stm8_operator (const char *name, unsigned int operands,
                                char *);
#define md_operator stm8_operator

extern void stm8_check_label (symbolS *labelsym);
//#define tc_check_label(ls)	stm8_check_label (ls)

#define tc_symbol_new_hook(sym) stm8_symbol_new_hook (sym)
extern void stm8_symbol_new_hook (symbolS *labelsym);

#define TC_SYMFIELD_TYPE int

#define md_register_arithmetic 0

// int split_words(char *str, char **chunks);
// int getnumber(const char *str, int *out);
// int gethex(const char *str, int *out);
// int getnum(const char *str, int *out);
// int read_arg(char *str, expressionS *exps);
// int read_args(char *str, expressionS exps[]);
// unsigned int bytes_count(unsigned int number);
// int compute_insn_size(struct stm8_opcodes_s opcode);
// int cmpspec(stm8_addr_mode_t addr_mode[], expressionS exps[], int count);
// void stm8_bfd_out(struct stm8_opcodes_s op, expressionS exp[], int count,
// char *frag); char* match_parentheses(char* str); int read_arg_ptr(char *str,
// expressionS *exps); int is_ptr_w(stm8_addr_mode_t type);
