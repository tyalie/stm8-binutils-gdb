/* BFD support for the STM8 processor.
   Copyright (C) 2007-2024 Free Software Foundation, Inc.
   Written by Åke Rehnman (at) gmail dot com

   This file is part of BFD, the Binary File Descriptor library.

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

#include "sysdep.h"

#include "bfd.h"
#include "libbfd.h"

const bfd_arch_info_type bfd_stm8_arch
    = { .bits_per_word = 8,
        .bits_per_address = 32,
        .bits_per_byte = 8,
        .arch = bfd_arch_stm8,
        .mach = bfd_mach_stm8,
        .arch_name = "stm8",
        .printable_name = "stm8",
        .section_align_power = 4,
        .the_default = true,
        .compatible = bfd_default_compatible,
        .scan = bfd_default_scan,
        .fill = bfd_arch_default_fill,
        .next = 0,
        .max_reloc_offset_into_insn = 0 };
