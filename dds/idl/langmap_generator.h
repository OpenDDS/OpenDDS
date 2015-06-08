/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef langmap_generator_H
#define langmap_generator_H

#include "dds_generator.h"

class GeneratorBase;

/// Generate code for the IDL -> Programming Language mapping
/// For example, IDL structs -> C++ structs, etc.
/// Enabled by the -L* command line options
class langmap_generator : public dds_generator {

public:
  langmap_generator() : generator_(0) {}
  void init();

private:
  bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                 AST_Constant* constant);

  bool gen_enum(AST_Enum*, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_struct_fwd(UTL_ScopedName* name,
                      AST_Type::SIZE_TYPE size);

  bool gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(AST_Union*, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* discriminator,
                 const char* repoid);

  bool gen_union_fwd(AST_UnionFwd*, UTL_ScopedName* name,
                     AST_Type::SIZE_TYPE size);

  bool gen_interf_fwd(UTL_ScopedName* name);

  GeneratorBase* generator_;
};

#endif
