/* STM8-specific support for 32-bit ELF
   Copyright (C) 2007-2024 Free Software Foundation, Inc.
   Written by �ke Rehnman (at) gmail dot com

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
#include "elf-bfd.h"
#include "libbfd.h"
#include "libiberty.h"

#include "elf/stm8.h"

bfd_reloc_status_type
bfd_elf_stm8_spec_reloc (bfd *abfd ATTRIBUTE_UNUSED, arelent *reloc_entry,
                         asymbol *symbol, void *data ATTRIBUTE_UNUSED,
                         asection *input_section, bfd *output_bfd,
                         char **error_message ATTRIBUTE_UNUSED);

static reloc_howto_type elf32_stm8_howto_table_1[] = {
  HOWTO (R_STM8_NONE,            /* type */
         0,                      /* rightshift */
         3,                      /* size (0 = byte, 1 = short, 2 = long) */
         0,                      /* bitsize */
         false,                  /* pc_relative */
         0,                      /* bitpos */
         complain_overflow_dont, /* complain_on_overflow */
         bfd_elf_generic_reloc,  /* special_function */
         "R_STM8_NONE",          /* name */
         false,                  /* partial_inplace */
         0,                      /* src_mask */
         0,                      /* dst_mask */
         false),                 /* pcrel_offset */

  /* 8 bit offset.  */
  HOWTO (R_STM8_8,                   /* type */
         0,                          /* rightshift */
         0,                          /* size (0 = byte, 1 = short, 2 = long) */
         8,                          /* bitsize */
         false,                      /* pc_relative */
         0,                          /* bitpos */
         complain_overflow_unsigned, /* complain_on_overflow */
         bfd_elf_generic_reloc,      /* special_function */
         "R_STM8_8",                 /* name */
         false,                      /* partial_inplace */
         0x0,                        /* src_mask */
         0xff,                       /* dst_mask */
         false),                     /* pcrel_offset */

  /* A 16 bit absolute relocation.  */
  HOWTO (R_STM8_16,                  /* type */
         0,                          /* rightshift */
         1,                          /* size (0 = byte, 1 = short, 2 = long) */
         16,                         /* bitsize */
         false,                      /* pc_relative */
         0,                          /* bitpos */
         complain_overflow_unsigned, /* complain_on_overflow */
         bfd_elf_generic_reloc,      /* special_function */
         "R_STM8_16",                /* name */
         false,                      /* partial_inplace */
         0x0,                        /* src_mask */
         0xffff,                     /* dst_mask */
         false),                     /* pcrel_offset */

  // use this with unpatched bfd_get_reloc_size
  HOWTO (R_STM8_24,      /* type */
         0,              /* rightshift */
         2,              /* size (0 = byte, 1 = short, 2 = long) */
         24,             /* bitsize */
         false,          /* pc_relative */
         0, /* bitpos */ // the relocation use bfd_get_32 so our relocation end
                         // up in the upper 24 bits and so relocation value has
                         // to be shifted 8 bits to the left
         complain_overflow_unsigned, /* complain_on_overflow */
         // bfd_elf_stm8_spec_reloc,     /* special_function */
         bfd_elf_generic_reloc, /* special_function */
         "R_STM8_24",           /* name */
         false,                 /* partial_inplace */
         0xff000000,            /* src_mask */
         0x00ffffff,            /* dst_mask */
         false),                /* pcrel_offset */

  HOWTO (R_STM8_32,                  /* type */
         0,                          /* rightshift */
         2,                          /* size (0 = byte, 1 = short, 2 = long) */
         32,                         /* bitsize */
         false,                      /* pc_relative */
         0,                          /* bitpos */
         complain_overflow_unsigned, /* complain_on_overflow */
         bfd_elf_generic_reloc,      /* special_function */
         "R_STM8_32",                /* name */
         false,                      /* partial_inplace */
         0x0,                        /* src_mask */
         0xffffffff,                 /* dst_mask */
         false),                     /* pcrel_offset */

  /* A 8 bit PC relative relocation.  */
  HOWTO (R_STM8_8_PCREL,           /* type */
         0,                        /* rightshift */
         0,                        /* size (0 = byte, 1 = short, 2 = long) */
         8,                        /* bitsize */
         true,                     /* pc_relative */
         0,                        /* bitpos */
         complain_overflow_signed, /* complain_on_overflow */
         bfd_elf_generic_reloc,    /* special_function */
         "R_STM8_8_PCREL",         /* name */
         false,                    /* partial_inplace */
         0x0,                      /* src_mask */
         0xff,                     /* dst_mask */
         true),                    /* pcrel_offset */
};

// stupid bfd_elf_generic_reloc cant handle 24-bit relocations
// so we have to write our own...
bfd_reloc_status_type
bfd_elf_stm8_spec_reloc (bfd *abfd ATTRIBUTE_UNUSED, arelent *reloc_entry,
                         asymbol *symbol, void *data ATTRIBUTE_UNUSED,
                         asection *input_section ATTRIBUTE_UNUSED,
                         bfd *output_bfd,
                         char **error_message ATTRIBUTE_UNUSED)
{
  if (output_bfd != NULL && (symbol->flags & BSF_SECTION_SYM) == 0
      && (!reloc_entry->howto->partial_inplace || reloc_entry->addend == 0))
    {
      reloc_entry->address += input_section->output_offset + 1;
      return bfd_reloc_ok;
    }

  if (output_bfd == NULL)
    return bfd_reloc_continue;
  reloc_entry->address += input_section->output_offset - 1;
  return bfd_reloc_continue;
}

static reloc_howto_type *
elf32_stm8_howto_from_type (unsigned int r_type)
{
  if (r_type < ARRAY_SIZE (elf32_stm8_howto_table_1))
    return &elf32_stm8_howto_table_1[r_type];

  return NULL;
}

static bool
elf32_stm8_info_to_howto (bfd *abfd ATTRIBUTE_UNUSED, arelent *bfd_reloc,
                          Elf_Internal_Rela *elf_reloc)
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (elf_reloc->r_info);
  bfd_reloc->howto = elf32_stm8_howto_from_type (r_type);
  return true;
}

struct elf32_stm8_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned char elf_reloc_val;
};

/* All entries in this list must also be present in elf32_stm8_howto_table.  */
static const struct elf32_stm8_reloc_map elf32_stm8_reloc_map[] = {
  { BFD_RELOC_NONE, R_STM8_NONE }, { BFD_RELOC_8, R_STM8_8 },
  { BFD_RELOC_16, R_STM8_16 },     { BFD_RELOC_24, R_STM8_24 },
  { BFD_RELOC_32, R_STM8_32 },     { BFD_RELOC_8_PCREL, R_STM8_8_PCREL }
};

static reloc_howto_type *
elf32_stm8_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
                              bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (elf32_stm8_reloc_map); i++)
    if (elf32_stm8_reloc_map[i].bfd_reloc_val == code)
      return elf32_stm8_howto_from_type (
          elf32_stm8_reloc_map[i].elf_reloc_val);

  return NULL;
}

static reloc_howto_type *
elf32_stm8_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (elf32_stm8_howto_table_1); i++)
    if (elf32_stm8_howto_table_1[i].name != NULL
        && strcasecmp (elf32_stm8_howto_table_1[i].name, r_name) == 0)
      return &elf32_stm8_howto_table_1[i];

  return NULL;
}

static bool
elf32_stm8_init_file_header (bfd *abfd, struct bfd_link_info *info)
{
  if (!_bfd_elf_init_file_header (abfd, info))
    return false;

  Elf_Internal_Ehdr *i_ehdrp = elf_elfheader (abfd);
  i_ehdrp->e_ident[EI_ABIVERSION] = 0;

  return true;
}

static bool
elf32_stm8_modify_segment_map (bfd *abfd,
                               struct bfd_link_info *info ATTRIBUTE_UNUSED)
{
  struct elf_segment_map *m;
  return true;
  m = elf_seg_map (abfd);
  while (m)
    {
      m->includes_filehdr = 0;
      m->includes_phdrs = 0;
      m = m->next;
    }
  return true;
}

#define ELF_ARCH bfd_arch_stm8
#define ELF_TARGET_ID GENERIC_ELF_DATA
#define ELF_MACHINE_CODE EM_STM8
#define ELF_MAXPAGESIZE 1

#define TARGET_BIG_SYM stm8_elf32_vec
#define TARGET_BIG_NAME "elf32-stm8"

#define elf_info_to_howto elf32_stm8_info_to_howto
#define elf_info_to_howto_rel NULL

#define elf_backend_init_file_header elf32_stm8_init_file_header
#define elf_backend_modify_segment_map elf32_stm8_modify_segment_map

#define bfd_elf32_bfd_reloc_type_lookup elf32_stm8_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup elf32_stm8_reloc_name_lookup

#include "elf32-target.h"
