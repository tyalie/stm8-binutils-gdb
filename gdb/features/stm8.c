/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: stm8.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_stm8 (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.stm8.core");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "cc_flags", 1);
  tdesc_add_flag (type_with_fields, 0, "C");
  tdesc_add_flag (type_with_fields, 1, "Z");
  tdesc_add_flag (type_with_fields, 2, "N");
  tdesc_add_flag (type_with_fields, 3, "I0");
  tdesc_add_flag (type_with_fields, 4, "H");
  tdesc_add_flag (type_with_fields, 5, "I1");
  tdesc_add_flag (type_with_fields, 7, "V");

  tdesc_create_reg (feature, "a", regnum++, 1, NULL, 8, "int");
  tdesc_create_reg (feature, "x", regnum++, 1, NULL, 16, "int");
  tdesc_create_reg (feature, "y", regnum++, 1, NULL, 16, "int");
  tdesc_create_reg (feature, "sp", regnum++, 1, NULL, 16, "data_ptr");
  tdesc_create_reg (feature, "pc", regnum++, 1, NULL, 24, "code_ptr");
  tdesc_create_reg (feature, "cc", regnum++, 1, NULL, 8, "int");
  return regnum;
}
