/* Target-dependent code for STM8, for GDB.
   Written by Ake Rehnman 2017-02-21,
   ake.rehnman (at) gmail dot com

   Copyright (C) 1996-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "arch-utils.h"
#include "defs.h"
#include "dis-asm.h"
#include "dwarf2/frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "frame.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "inferior.h"
#include "objfiles.h"
#include "progspace.h"
#include "regcache.h"
#include "symfile.h"
#include "target-descriptions.h"
#include "trad-frame.h"
#include <algorithm>

enum stm8_regnum
{
  STM8_PC_REGNUM,
  STM8_A_REGNUM,
  STM8_X_REGNUM,
  STM8_Y_REGNUM,
  STM8_SP_REGNUM,
  STM8_CC_REGNUM,
  // pseudo register
  STM8_FP_REGNUM
};

static const char *stm8_register_names[] = { "pc", "a", "x", "y", "sp", "cc" };

struct stm8_soft_reg
{
  const char *name;
  CORE_ADDR addr;
};

unsigned int stm8_debug = 1;

#define STM8_NUM_REGS ARRAY_SIZE (stm8_register_names)

struct stm8_gdbarch_tdep : gdbarch_tdep_base
{
  struct stm8_soft_reg fp_reg;
};

enum insn_return_kind
{
  RETURN_RET,
  RETURN_RETF,
  RETURN_IRET
};

static int
stm8_convert_register_p (struct gdbarch *gdbarch, int regnum,
                         struct type *type)
{
  if ((regnum == STM8_FP_REGNUM) && (type->length () > 2))
    {
      return 1;
    }
  return 0;
}

/* Read a value of type TYPE from register REGNUM in frame FRAME, and
   return its contents in TO.  */

static int
stm8_register_to_value (frame_info_ptr frame, int regnum, struct type *type,
                        gdb_byte *to, int *optimizedp, int *unavailablep)
{
  struct type *fpreg_type
      = register_type (get_frame_arch (frame), STM8_FP_REGNUM);

  /* We only support ptr values.  */
  if ((type->code () != TYPE_CODE_PTR) && (regnum != STM8_FP_REGNUM)
      && (type->length () >= fpreg_type->length ()))
    {
      warning (
          _ ("Conversion failure in stm8_register_to_value: regnum = %d "),
          regnum);
      *optimizedp = *unavailablep = 0;
      return 0;
    }

  /* Convert to TYPE.  */

  memset (to, 0, type->length ());

  if (!get_frame_register_bytes (
          frame, regnum, 0,
          { to + type->length () - fpreg_type->length (),
            fpreg_type->length () },
          optimizedp, unavailablep))
    return 0;

  *optimizedp = *unavailablep = 0;
  return 1;
}

/* Look in the symbol table for the address of a pseudo register
   in memory.  If we don't find it, pretend the register is not used
   and not available.  */
static void
stm8_get_register_info (struct stm8_soft_reg *reg, const char *name)
{
  struct bound_minimal_symbol msymbol;

  msymbol = lookup_minimal_symbol (name, NULL, NULL);
  if (msymbol.minsym)
    {
      reg->addr = msymbol.value_address ();
      reg->name = xstrdup (name);
    }
  else
    {
      reg->name = 0;
      reg->addr = 0;
    }
}

static void
stm8_initialize_soft_register_info (struct stm8_gdbarch_tdep *tdep)
{
  stm8_get_register_info (&tdep->fp_reg, "_fp_");
  if ((tdep->fp_reg.name == 0) && (current_program_space->symfile_object_file))
    {
      warning (_ ("No frame soft register found in the symbol table (_fp_).\n"
                  "Stack backtrace will not work.\n"));
    }
}

static const char *
stm8_register_name (struct gdbarch *gdbarch, int regnum)
{
  if (regnum >= 0 && regnum < STM8_NUM_REGS)
    return stm8_register_names[regnum];
  if (regnum == STM8_FP_REGNUM)
    return "fp";
  return NULL;
}

static struct type *
stm8_register_type (struct gdbarch *gdbarch, int regnum)
{
  switch (regnum)
    {
    case STM8_PC_REGNUM:
      return builtin_type (gdbarch)->builtin_uint32;
    case STM8_SP_REGNUM:
      return builtin_type (gdbarch)->builtin_uint16;
    case STM8_X_REGNUM:
    case STM8_Y_REGNUM:
      return builtin_type (gdbarch)->builtin_uint16;
    case STM8_FP_REGNUM:
      return builtin_type (gdbarch)->builtin_uint16;
    default:
      return builtin_type (gdbarch)->builtin_uint8;
    }
}

static enum register_status
stm8_pseudo_register_read (struct gdbarch *gdbarch,
                           readable_regcache *regcache, int regnum,
                           gdb_byte *buf)
{
  struct stm8_gdbarch_tdep *tdep = gdbarch_tdep<stm8_gdbarch_tdep> (gdbarch);
  int regsize = 2;

  switch (regnum)
    {
    case STM8_FP_REGNUM:
      /* Fetch a soft register: translate into a memory read.  */
      memset (buf, 0, regsize);
      if (tdep->fp_reg.name)
        {
          target_read_memory (tdep->fp_reg.addr, buf, 2);
        }
      return REG_VALID;
    default:
      internal_error (_ ("invalid regnum"));
      return REG_UNAVAILABLE;
    }
}

static void
stm8_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
                            int regnum, const gdb_byte *buf)
{
  struct stm8_gdbarch_tdep *tdep = gdbarch_tdep<stm8_gdbarch_tdep> (gdbarch);

  switch (regnum)
    {
    case STM8_FP_REGNUM:
      /* Store a soft register: translate into a memory write.  */
      if (tdep->fp_reg.name)
        {

          target_write_memory (tdep->fp_reg.addr, buf, 2);
        }
      return;
    default:
      internal_error (_ ("invalid regnum"));
      return;
    }
}

struct stm8_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  CORE_ADDR pc;

  /* Do we have a frame?  */
  int frameless_p;

  /* Frame size.  */
  int framesize;

  /* Frame size.  */
  int stackadj;

  /* Frame register.  */
  int fp_regnum;

  /* Return instruction */
  enum insn_return_kind return_kind;

  /* Offsets to saved registers.  */
  int register_offsets[STM8_NUM_REGS];

  /* Table of saved registers.  */
  struct trad_frame_saved_reg *saved_regs;
};

static const unsigned char *
stm8_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr,
                         int *lenptr)
{
  static gdb_byte stm8_breakpoint[] = { 0x8b };

  *lenptr = sizeof (stm8_breakpoint);
  return stm8_breakpoint;
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */
static int
stm8_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  CORE_ADDR pc = *pcptr;

  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_breakpoint_kind_from_pc called %8.8lx\n",
                pc);

  return 1;
}

static const gdb_byte *
stm8_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_sw_breakpoint_from_kind called\n");

  static gdb_byte stm8_breakpoint[] = { 0x8b };

  *size = 1;

  return stm8_breakpoint;
}

static int dwarf2_to_reg_map[6] = {
  0 /* r0  */, 1 /* r1  */,
  2 /* r2  */, 3 /* r3  */, /*  0- 3 */
  4 /* r4  */, 5            /* r5  */
};

static int
stm8_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_dwarf2_reg_to_regnum called\n");
  if (reg >= 0 && reg < sizeof (dwarf2_to_reg_map))
    return dwarf2_to_reg_map[reg];
  if (reg == 31)
    return STM8_FP_REGNUM;
  return -1;
}

static void
stm8_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_write_pc called\n");
  regcache_cooked_write_unsigned (regcache, STM8_PC_REGNUM, pc);
}

static CORE_ADDR
stm8_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  gdb_byte buf[4];
  CORE_ADDR pc;

  frame_unwind_register (next_frame, STM8_PC_REGNUM, buf);
  pc = extract_typed_address (buf, builtin_type (gdbarch)->builtin_func_ptr);
  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_unwind_pc called: pc=%8.8lx\n", pc);
  return pc;
}

static CORE_ADDR
stm8_unwind_sp (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  ULONGEST sp;

  sp = frame_unwind_register_unsigned (next_frame, STM8_SP_REGNUM);
  sp = gdbarch_addr_bits_remove (gdbarch, sp);
  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_unwind_sp called: sp=%8.8lx\n", sp);
  return sp;
}

/*
static struct value *
stm8_dwarf2_prev_register (frame_info_ptr this_frame,
                void **this_cache, int regnum)
{
        CORE_ADDR pc;

        switch (regnum)
        {
            case STM8_PC_REGNUM:
              pc = frame_unwind_register_unsigned (this_frame,
AARCH64_PC_REGNUM); return frame_unwind_got_constant (this_frame, regnum, lr);

        default:
                internal_error (_("Unexpected register %d"), regnum);
        }
}

static void
stm8_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
                struct dwarf2_frame_state_reg *reg,
                frame_info_ptr this_frame)
{
        switch (regnum)
        {
        case STM8_PC_REGNUM:
                reg->how = DWARF2_FRAME_REG_FN;
                reg->loc.fn = stm8_dwarf2_prev_register;
                break;
        case STM8_SP_REGNUM:
                reg->how = DWARF2_FRAME_REG_CFA;
                break;
        }
}
*/

/* Assuming THIS_FRAME is a dummy, return the frame ID of that
   dummy frame.  The frame ID's base needs to match the TOS value
   saved by save_dummy_frame_tos(), and the PC match the dummy frame's
   breakpoint.  */

static struct frame_id
stm8_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  gdb_printf (gdb_stdlog, "stm8_dummy_id called\n");

  return frame_id_build (
      get_frame_register_signed (this_frame, STM8_SP_REGNUM),
      get_frame_pc (this_frame));
}

/* Allocate and initialize a frame cache.  */

static struct stm8_frame_cache *
stm8_alloc_frame_cache (void)
{
  struct stm8_frame_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct stm8_frame_cache);

  //  /* Base address.  */
  cache->base = 0;
  cache->pc = 0;

  /* Frameless until proven otherwise.  */
  cache->frameless_p = 1;

  return cache;
}

/* Figure out what return type this function has.  */
static enum insn_return_kind
stm8_get_return_insn (CORE_ADDR pc)
{
  const char *name;
  CORE_ADDR func_addr, func_end;

  if (find_pc_partial_function (pc, &name, &func_addr, &func_end))
    {
      gdb_byte buf[1];
      // func_end is pointing to last insn+1 ???
      if (!target_read_code (func_end - 1, buf, sizeof (buf)))
        {
          switch (buf[0])
            {
            case 0x81:
              return RETURN_RET;
            case 0x87:
              return RETURN_RETF;
            case 0x80:
              return RETURN_IRET;
            }
        }
    }

  // defaut to RET
  if (stm8_debug)
    gdb_printf (gdb_stdlog,
                "WARNING: stm8_get_return_insn: No return instruction found "
                "in function %s start_addr = %8.8lx end_addr = %8.8lx\n",
                name, func_addr, func_end);
  return RETURN_RET;
}

/* Find the start of this function.  */
static CORE_ADDR
stm8_analyze_prologue (struct gdbarch *gdbarch, CORE_ADDR pc,
                       CORE_ADDR current_pc, struct stm8_frame_cache *cache)
{
  const char *name;
  CORE_ADDR func_addr, func_end, stop;
  unsigned long insn;
  int done;
  gdb_byte buf[4];

  if (stm8_debug)
    gdb_printf (gdb_stdlog,
                "stm8_analyze_prologue called (pc=%8.8lx current_pc=%8.8lx)\n",
                pc, current_pc);

  /* Initialize info about frame.  */
  cache->framesize = 0;
  cache->fp_regnum = STM8_FP_REGNUM;
  cache->frameless_p = 1;
  cache->stackadj = 0;

  /* Find the start of this function.  */
  find_pc_partial_function (pc, &name, &func_addr, &func_end);
  if (func_addr < pc)
    pc = func_addr;

  if (current_pc < pc)
    return current_pc;

  /* Start decoding the prologue.  We start by checking two special cases:

1. We're about to return
2. We're at the first insn of the prologue.

If we're about to return, our frame has already been deallocated.
If we are stopped at the first instruction of a prologue,
then our frame has not yet been set up.  */

  /* Get the first insn from memory.  */
  if (target_read_code (pc, buf, sizeof (buf)))
    return current_pc;

  // RET?
  if (buf[0] == 0x81)
    {
      if (stm8_debug)
        gdb_printf (gdb_stdlog, "stm8_analyze_prologue: current_pc is RTS so "
                                "we dont have a frame!\n");
      return pc;
    }

  /* Start at beginning of function and analyze until we get to the
current pc, or the end of the function, whichever is first.  */
  stop = (current_pc < func_end ? current_pc : func_end);

  if (stm8_debug)
    gdb_printf (gdb_stdlog,
                "stm8_analyze_prologue: name=%s, func_addr=%s, stop=%s\n",
                name, paddress (gdbarch, func_addr), paddress (gdbarch, stop));

  /* scan the prologue */
  pc = func_addr;
  done = 0;
  while ((pc < stop) && !done)
    {
      if (target_read_code (pc, buf, sizeof (buf)))
        return current_pc;

      insn = buf[0];
      if (buf[0] == 0x90)
        insn = (insn << 8) + buf[1];

      switch (insn)
        {
        case 0x3b:
          // PUSH extmem
          cache->framesize += 1;
          pc += 3;
          break;
        case 0x9096:
          // LDW Y, SP
          pc += 2;
          break;
        case 0x90cf:
          // LDW extmem, Y
          pc += 4;
          cache->frameless_p = 0;
          break;
        case 0x52:
          // SUB SP,#xx?
          cache->framesize += buf[1];
          pc += 2;
          break;
        case 0x88:
          // PUSH A?
          cache->framesize += 1;
          pc++;
          break;
        case 0x89:
          // PUSH X?
          cache->framesize += 2;
          pc++;
          break;
        case 0x9089:
          // PUSH Y?
          cache->framesize += 2;
          pc += 2;
          break;
        default:
          done = 1;
        }
    }

  if (stm8_debug)
    if (cache->frameless_p)
      gdb_printf (gdb_stdlog, "stm8_analyze_prologue: this function is "
                              "frameless! No pc adjustment was done.\n");

  return pc;
}

/* Return PC of first real instruction of the function starting at
   START_PC.  */

static CORE_ADDR
stm8_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_skip_prologue called: start_pc=%8.8lx\n",
                start_pc);

  struct symtab_and_line sal;
  CORE_ADDR func_start, func_end, ostart_pc;
  struct stm8_frame_cache cache;

  /* This is the preferred method, find the end of the prologue by
using the debugging information.  Debugging info does not always
give the right answer since parameters are stored on stack after this.
Always analyze the prologue.  */
  if (find_pc_partial_function (start_pc, NULL, &func_start, &func_end))
    {
      sal = find_pc_line (func_start, 0);

      if (sal.end < func_end && start_pc <= sal.end)
        start_pc = sal.end;
    }

  ostart_pc
      = stm8_analyze_prologue (gdbarch, func_start, 0xffffffffUL, &cache);

  if (stm8_debug)
    gdb_printf (gdb_stdlog,
                "stm8_skip_prologue: start_pc=%8.8lx ostart_pc=%8.8lx\n",
                start_pc, ostart_pc);

  if (ostart_pc > start_pc)
    return ostart_pc;
  return start_pc;
}

/* Normal frames.

  The STM8 frame is typically:

     args
     return lo addr
     return hi addr
     prev fp lo
     prev fp hi
     locals <- fp
sp-> empty
 */

static struct stm8_frame_cache *
stm8_frame_cache (frame_info_ptr next_frame, void **this_cache)
{
  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_frame_cache called\n");

  struct stm8_frame_cache *cache;
  struct gdbarch *gdbarch = get_frame_arch (next_frame);
  CORE_ADDR start_pc, current_pc, current_sp, fp;
  int retsize;

  if (*this_cache)
    return (struct stm8_frame_cache *)*this_cache;

  cache = stm8_alloc_frame_cache ();
  *this_cache = cache;

  cache->saved_regs = trad_frame_alloc_saved_regs (next_frame);

  start_pc = get_frame_func (next_frame);

  cache->return_kind = stm8_get_return_insn (start_pc);

  current_pc = get_frame_pc (next_frame);
  current_sp = get_frame_sp (next_frame);

  /* Analyze the function prologue.  */
  if (start_pc != 0)
    stm8_analyze_prologue (gdbarch, start_pc, current_pc,
                           (struct stm8_frame_cache *)*this_cache);

  /* get our fp by unwinding it from the next frame
   * if we don't have a fp we use sp instead but if there
   * are arguments on the stack unwinding will be
   * unpredictable.
   */
  // fp = frame_unwind_register_unsigned (next_frame, cache->fp_regnum);
  fp = get_frame_register_unsigned (next_frame, cache->fp_regnum);
  if (fp == 0)
    {
      cache->base = 0;
      cache->frameless_p = 1;
      // return (struct stm8_frame_cache *) (*this_cache);
    }

  switch (cache->return_kind)
    {
    case RETURN_RET:
      retsize = 2;
      break;
    case RETURN_RETF:
      retsize = 3;
      break;
    case RETURN_IRET:
      retsize = 9;
      break;
    }

  if (cache->frameless_p)
    {
      cache->base = current_sp + cache->framesize;
      if (cache->return_kind == RETURN_IRET)
        cache->saved_regs[STM8_PC_REGNUM].set_addr (cache->base + 1 + 6);
      else
        cache->saved_regs[STM8_PC_REGNUM].set_addr (cache->base + 1);

      cache->saved_regs[STM8_SP_REGNUM].set_value (cache->base + retsize);
    }
  else
    {
      /* fp points to our base */
      cache->base = fp;
      if (cache->return_kind == RETURN_IRET)
        cache->saved_regs[STM8_PC_REGNUM].set_addr (cache->base + 1 + 2 + 6);
      else
        cache->saved_regs[STM8_PC_REGNUM].set_addr (cache->base + 1 + 2);
      cache->saved_regs[STM8_FP_REGNUM].set_addr (fp + 1);
      cache->saved_regs[STM8_SP_REGNUM].set_value (cache->base + retsize + 2);
    }

  if (stm8_debug)
    {
      gdb_printf (
          gdb_stdlog,
          "stm8_frame_cache: base=%4.4lx curr_pc=%4.4lx curr_sp=%4.4lx "
          "fp_regnum=%d fp=%4.4lx framesize=%4.4x stackadj=%4.4x retsize=%d\n",
          cache->base, current_pc, current_sp, cache->fp_regnum, fp,
          cache->framesize, cache->stackadj, retsize);

      CORE_ADDR frame_pc;
      CORE_ADDR frame_sp;
      CORE_ADDR frame_fp;
      frame_pc = value_as_long (trad_frame_get_prev_register (
          next_frame, cache->saved_regs, STM8_PC_REGNUM));
      frame_sp = value_as_long (trad_frame_get_prev_register (
          next_frame, cache->saved_regs, STM8_SP_REGNUM));

      // this is stupid, trad_frame_get_prev_register can't get a
      // register value unless we have a valid frame id
      // hopefully this will get resolved in the future :)
      if (cache->saved_regs[STM8_FP_REGNUM].addr () > 0)
        frame_fp = value_as_long (trad_frame_get_prev_register (
            next_frame, cache->saved_regs, STM8_FP_REGNUM));
      else
        frame_fp = fp;

      frame_pc = frame_pc >> 16;
      gdb_printf (gdb_stdlog, "stm8_frame_cache: pc=%8.8lx *pc=%8.8lx\n",
                  cache->saved_regs[STM8_PC_REGNUM].addr (), frame_pc);
      gdb_printf (gdb_stdlog, "stm8_frame_cache: sp=%8.8lx *sp=%8.8lx\n",
                  cache->saved_regs[STM8_SP_REGNUM].addr (), frame_sp);
      gdb_printf (gdb_stdlog, "stm8_frame_cache: fp=%8.8lx *fp=%8.8lx\n",
                  cache->saved_regs[STM8_FP_REGNUM].addr (), frame_fp);
    }

  return (struct stm8_frame_cache *)(*this_cache);
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */
static void
stm8_frame_this_id (frame_info_ptr next_frame, void **this_cache,
                    struct frame_id *this_id)
{
  struct stm8_frame_cache *cache = stm8_frame_cache (next_frame, this_cache);
  CORE_ADDR func;
  CORE_ADDR base;

  func = get_frame_func (next_frame);
  base = cache->base;

  /* This marks the outermost frame.  */
  if (base == 0)
    return;

  (*this_id) = frame_id_build (base, func);

  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_frame_this_id: base=%8.8lx pc=%8.8lx\n",
                base, func);
}

static struct value *
stm8_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
                          int regnum)
{
  struct value *value;
  struct stm8_frame_cache *info = stm8_frame_cache (this_frame, this_cache);

  value = trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);

  /* Take into account the STM8 specific call.
   * Different lengths if it is CALL or CALLF  */
  if (regnum == STM8_PC_REGNUM)
    {
      CORE_ADDR pc = value_as_long (value);
      if (info->return_kind == RETURN_RET)
        pc >>= 16;
      else
        pc >>= 8;

      release_value (value);
      value->decref ();

      value = frame_unwind_got_constant (this_frame, regnum, pc);
    }

  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_frame_prev_register: regnum(%d)=%8.8lx\n",
                regnum, value_as_long (value));

  return value;
}

static void
stm8_store_return_value (struct type *type, struct regcache *regcache,
                         const gdb_byte *valbuf)
{
  int len;

  len = type->length ();

  if (len == 1)
    regcache->raw_write_part (STM8_A_REGNUM, 0, 1, valbuf);
  else if (len == 2)
    regcache->raw_write_part (STM8_X_REGNUM, 0, 2, valbuf);
  else if (len == 4)
    {
      regcache->raw_write_part (STM8_X_REGNUM, 0, 2, valbuf + 2);
      regcache->raw_write_part (STM8_Y_REGNUM, 0, 2, valbuf);
    }
  else
    error (_ ("return of value > 4 is not supported."));
}

/* Given a return value in `regcache' with a type `type',
   extract and copy its value into `valbuf'.  */

static void
stm8_extract_return_value (struct type *type, struct regcache *regcache,
                           void *valbuf)
{
  gdb_byte buf[2];

  switch (type->length ())
    {
    case 1:
      regcache->raw_read (STM8_A_REGNUM, buf);
      memcpy (valbuf, buf, 1);
      break;

    case 2:
      regcache->raw_read (STM8_X_REGNUM, buf);
      memcpy (valbuf, buf, 2);
      break;

    case 4:
      regcache->raw_read (STM8_X_REGNUM, buf);
      memcpy ((char *)valbuf + 2, buf, 2);
      regcache->raw_read (STM8_Y_REGNUM, buf);
      memcpy (valbuf, buf, 2);
      break;

    default:
      error (_ ("bad size for return value"));
    }
}

static enum return_value_convention
stm8_return_value (struct gdbarch *gdbarch, struct value *function,
                   struct type *valtype, struct regcache *regcache,
                   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (valtype->code () == TYPE_CODE_STRUCT
      || valtype->code () == TYPE_CODE_UNION
      || valtype->code () == TYPE_CODE_ARRAY || valtype->length () > 4)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    {
      if (readbuf != NULL)
        stm8_extract_return_value (valtype, regcache, readbuf);
      if (writebuf != NULL)
        stm8_store_return_value (valtype, regcache, writebuf);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

static const struct frame_unwind stm8_frame_unwind
    = { "stm8 prologue",
        NORMAL_FRAME,
        default_frame_unwind_stop_reason,
        stm8_frame_this_id,
        stm8_frame_prev_register,
        NULL,
        default_frame_sniffer };

static CORE_ADDR
stm8_frame_base_address (frame_info_ptr next_frame, void **this_cache)
{
  struct stm8_frame_cache *cache = stm8_frame_cache (next_frame, this_cache);

  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_frame_base_address: fb=%8.8lx\n",
                cache->base);

  return cache->base;
}

static CORE_ADDR
stm8_frame_args_address (frame_info_ptr this_frame, void **this_cache)
{
  CORE_ADDR addr;
  struct stm8_frame_cache *info = stm8_frame_cache (this_frame, this_cache);

  addr = info->base;
  if (info->return_kind == RETURN_IRET)
    addr += 12; // 2 bytes fp + 9 bytes regs + 1
  else if (info->return_kind == RETURN_RETF)
    addr += 6; // 2 bytes fp + 3 bytes pc + 1
  else
    addr += 5; // 2 bytes fp + 2 bytes pc + 1

  if (stm8_debug)
    gdb_printf (gdb_stdlog, "stm8_frame_args_address: addr = %8.8lx\n", addr);

  return addr;
}

static const struct frame_base stm8_frame_base
    = { &stm8_frame_unwind, stm8_frame_base_address, stm8_frame_base_address,
        stm8_frame_args_address };

static const struct frame_base *
stm8_frame_base_sniffer (frame_info_ptr this_frame)
{
  return &stm8_frame_base;
}

struct target_desc *tdesc_stm8;
static void
initialize_tdesc_stm8 (void)
{
  target_desc_up tdesc = allocate_target_description ();
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (tdesc.get (), "org.gnu.gdb.stm8.core");
  tdesc_create_reg (feature, "pc", 0, 1, "general", 32, "uint32");
  tdesc_create_reg (feature, "a", 1, 1, "general", 8, "uint8");
  tdesc_create_reg (feature, "x", 2, 1, "general", 16, "uint16");
  tdesc_create_reg (feature, "y", 3, 1, "general", 16, "uint16");
  tdesc_create_reg (feature, "sp", 4, 1, "general", 16, "uint16");
  tdesc_create_reg (feature, "cc", 5, 1, "general", 8, "uint16");

  tdesc_stm8 = tdesc.release ();
}

/* Initialize the gdbarch structure for the STM8.  */

static struct gdbarch *
stm8_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct stm8_gdbarch_tdep *tdep;
  tdesc_arch_data_up tdesc_data;
  const struct target_desc *tdesc = info.target_desc;

  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    {
      tdep = gdbarch_tdep<stm8_gdbarch_tdep> (arches->gdbarch);
      stm8_initialize_soft_register_info (tdep);
      return arches->gdbarch;
    }
  if (tdesc == NULL)
    tdesc = tdesc_stm8;

  /* Check any target description for validity.  */
  if (tdesc_has_registers (tdesc))
    {
      const struct tdesc_feature *feature;
      int valid_p;

      feature = tdesc_find_feature (tdesc, "org.gnu.gdb.stm8.core");
      if (feature == NULL)
        return NULL;
      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      for (int i = 0; i < STM8_NUM_REGS; i++)
        valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
                                            stm8_register_names[i]);
      if (!valid_p)
        return NULL;
    }

  /* None found, create a new architecture from the information provided.  */
  gdbarch = gdbarch_alloc (&info, gdbarch_tdep_up (new stm8_gdbarch_tdep));
  tdep = gdbarch_tdep<stm8_gdbarch_tdep> (gdbarch);

  /* Initialize soft registers */
  stm8_initialize_soft_register_info (tdep);

  set_gdbarch_num_regs (gdbarch, STM8_NUM_REGS);
  set_gdbarch_register_name (gdbarch, stm8_register_name);
  set_gdbarch_register_type (gdbarch, stm8_register_type);
  set_tdesc_pseudo_register_type (gdbarch, stm8_register_type);
  set_tdesc_pseudo_register_name (gdbarch, stm8_register_name);

  set_gdbarch_num_pseudo_regs (gdbarch, 1);
  set_gdbarch_pseudo_register_read (gdbarch, stm8_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
                                                stm8_pseudo_register_write);

  set_gdbarch_convert_register_p (gdbarch, stm8_convert_register_p);
  set_gdbarch_register_to_value (gdbarch, stm8_register_to_value);

  /* Register numbers of various important registers.  */
  set_gdbarch_sp_regnum (gdbarch, STM8_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, STM8_PC_REGNUM);
  set_gdbarch_ps_regnum (gdbarch, STM8_CC_REGNUM);

  /* Map Dwarf2 registers to GDB registers.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, stm8_dwarf2_reg_to_regnum);

  /* Call dummy code.  */
  set_gdbarch_call_dummy_location (gdbarch, ON_STACK);

  set_gdbarch_return_value (gdbarch, stm8_return_value);

  set_gdbarch_skip_prologue (gdbarch, stm8_skip_prologue);

  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch, stm8_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, stm8_sw_breakpoint_from_kind);

  set_gdbarch_breakpoint_from_pc (gdbarch, stm8_breakpoint_from_pc);

  set_gdbarch_print_insn (gdbarch, print_insn_stm8);

  set_gdbarch_write_pc (gdbarch, stm8_write_pc);

  set_gdbarch_unwind_pc (gdbarch, stm8_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, stm8_unwind_sp);
  set_gdbarch_dummy_id (gdbarch, stm8_dummy_id);

  frame_base_set_default (gdbarch, &stm8_frame_base);

  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &stm8_frame_unwind);
  frame_base_append_sniffer (gdbarch, dwarf2_frame_base_sniffer);
  frame_base_append_sniffer (gdbarch, stm8_frame_base_sniffer);

  if (tdesc_data)
    tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  return gdbarch;
}

static void
show_stm8_debug (struct ui_file *file, int from_tty,
                 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _ ("stm8 debugging is %s.\n"), value);
}

void _initialize_stm8_tdep ();
void
_initialize_stm8_tdep ()
{
  stm8_debug = 0;
  gdbarch_register (bfd_arch_stm8, stm8_gdbarch_init);
  initialize_tdesc_stm8 ();

  add_setshow_zuinteger_cmd ("stm8", class_maintenance, &stm8_debug, _ ("\
Set stm8 debugging."),
                             _ ("\
Show stm8 debugging."),
                             _ ("\
When non-zero, stm8 debugging is enabled."),
                             NULL, show_stm8_debug, &setdebuglist,
                             &showdebuglist);
}
