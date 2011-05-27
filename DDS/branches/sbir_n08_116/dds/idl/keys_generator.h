/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef keys_generator_H
#define keys_generator_H

#include "dds_generator.h"

class keys_generator : public dds_generator {
public:
  bool gen_const(UTL_ScopedName*, bool, AST_Expression::ExprType,
                 AST_Expression::AST_ExprValue*)
  { return true; }

  bool gen_enum(UTL_ScopedName*,
                const std::vector<AST_EnumVal*>&, const char*)
  { return true; }

  bool gen_struct(UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields, const char* repoid);

  bool gen_typedef(UTL_ScopedName*, AST_Type*, const char*)
  { return true; }

  bool gen_interf(UTL_ScopedName*, bool,
                  const std::vector<AST_Interface*>&,
                  const std::vector<AST_Interface*>&,
                  const std::vector<AST_Attribute*>&,
                  const std::vector<AST_Operation*>&, const char*)
  { return true; }

  bool gen_interf_fwd(UTL_ScopedName*)
  { return true; }

  bool gen_native(UTL_ScopedName*, const char*)
  { return true; }

  bool gen_union(UTL_ScopedName*, const std::vector<AST_UnionBranch*>&,
                 AST_Type*, AST_Expression::ExprType,
                 const AST_Union::DefaultValue&, const char*)
  { return true; }
};

#endif
