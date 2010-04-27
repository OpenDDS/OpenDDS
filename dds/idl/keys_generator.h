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
  bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                 AST_Expression::ExprType type,
                 AST_Expression::AST_ExprValue* value)
  { return true; }

  bool gen_enum(UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid)
  { return true; }

  bool gen_struct(UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields, const char* repoid);

  bool gen_typedef(UTL_ScopedName* name, AST_Type* base, const char* repoid)
  { return true; }

  bool gen_interf(UTL_ScopedName* name, bool local,
                  const std::vector<AST_Interface*>& inherits,
                  const std::vector<AST_Interface*>& inherits_flat,
                  const std::vector<AST_Attribute*>& attrs,
                  const std::vector<AST_Operation*>& ops, const char* repoid)
  { return true; }

  bool gen_interf_fwd(UTL_ScopedName* name)
  { return true; }

  bool gen_native(UTL_ScopedName* name, const char* repoid)
  { return true; }

  bool gen_union(UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* discriminator,
                 AST_Expression::ExprType udisc_type,
                 const AST_Union::DefaultValue& default_value,
                 const char* repoid)
  { return true; }
};

#endif
