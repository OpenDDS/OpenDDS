/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef metaclass_generator_H
#define metaclass_generator_H

#include "dds_generator.h"

class metaclass_generator : public dds_generator {
public:
  metaclass_generator()
    : first_struct_(true)
  {}

  bool gen_enum(AST_Enum* node, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(AST_Union* node, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* type, const char* repoid);

private:
  bool first_struct_;
};

#endif
