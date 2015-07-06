/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef v8_generator_H
#define v8_generator_H

#include "dds_generator.h"

class v8_generator : public dds_generator {

public:
  v8_generator() : first_(true) {}

private:
  bool gen_enum(AST_Enum*, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(AST_Union*, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* type, const char* repoid);

  void fwd_decl();

  bool first_;
};

#endif
