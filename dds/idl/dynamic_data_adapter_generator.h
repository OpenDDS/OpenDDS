/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef dynamic_data_adapter_generator_H
#define dynamic_data_adapter_generator_H

#include "dds_generator.h"

class dynamic_data_adapter_generator : public dds_generator {
public:
  dynamic_data_adapter_generator()
  {}

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(AST_Union* node, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* type, const char* repoid);
};

#endif
