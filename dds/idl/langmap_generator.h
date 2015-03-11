/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef langmap_generator_H
#define langmap_generator_H

#include "dds_generator.h"

/// Generate code for the IDL -> Programming Language mapping
/// For example, IDL structs -> C++ structs, etc.
/// Enabled by the -L* command line options
class langmap_generator : public dds_generator {

public:
  langmap_generator() {}
  void init();

private:
  bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                 AST_Expression::ExprType type,
                 AST_Expression::AST_ExprValue* value);

  bool gen_enum(UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields, const char* repoid);

  bool gen_typedef(UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* type, const char* repoid);

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
};

#endif
