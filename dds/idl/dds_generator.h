/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef dds_generator_H
#define dds_generator_H

#include "utl_scoped_name.h"
#include "ast.h"

#include <string>
#include <vector>

class dds_generator {
public:
  virtual ~dds_generator() = 0;

  virtual bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                         AST_Expression::ExprType type,
                         AST_Expression::AST_ExprValue* value) = 0;

  virtual bool gen_enum(UTL_ScopedName* name,
                        const std::vector<AST_EnumVal*>& contents,
                        const char* repoid) = 0;

  virtual bool gen_struct(UTL_ScopedName* name,
                          const std::vector<AST_Field*>& fields,
                          const char* repoid) = 0;

  virtual bool gen_typedef(UTL_ScopedName* name, AST_Type* base,
                           const char* repoid) = 0;

  virtual bool gen_interf(UTL_ScopedName* name, bool local,
                          const std::vector<AST_Interface*>& inherits,
                          const std::vector<AST_Interface*>& inherits_flat,
                          const std::vector<AST_Attribute*>& attrs,
                          const std::vector<AST_Operation*>& ops,
                          const char* repoid) = 0;

  virtual bool gen_interf_fwd(UTL_ScopedName* name) = 0;

  virtual bool gen_native(UTL_ScopedName* name, const char* repoid) = 0;

  virtual bool gen_union(UTL_ScopedName* name,
                         const std::vector<AST_UnionBranch*>& branches,
                         AST_Type* discriminator,
                         AST_Expression::ExprType udisc_type,
                         const AST_Union::DefaultValue& default_value,
                         const char* repoid) = 0;

  static std::string scoped_helper(UTL_ScopedName* sn, const char* sep);
};

class composite_generator : public dds_generator {
public:
  bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                 AST_Expression::ExprType type,
                 AST_Expression::AST_ExprValue* value);

  bool gen_enum(UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields, const char* repoid);

  bool gen_typedef(UTL_ScopedName* name, AST_Type* base, const char* repoid);

  bool gen_interf(UTL_ScopedName* name, bool local,
                  const std::vector<AST_Interface*>& inherits,
                  const std::vector<AST_Interface*>& inherits_flat,
                  const std::vector<AST_Attribute*>& attrs,
                  const std::vector<AST_Operation*>& ops, const char* repoid);

  bool gen_interf_fwd(UTL_ScopedName* name);

  bool gen_native(UTL_ScopedName* name, const char* repoid);

  bool gen_union(UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* discriminator,
                 AST_Expression::ExprType udisc_type,
                 const AST_Union::DefaultValue& default_value,
                 const char* repoid);

  template <typename InputIterator>
  composite_generator(InputIterator begin, InputIterator end)
  : components_(begin, end) {}

private:
  std::vector<dds_generator*> components_;
};

#endif
