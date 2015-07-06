/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef keys_generator_H
#define keys_generator_H

#include "dds_generator.h"

class keys_generator : public dds_generator {
public:
  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef*, UTL_ScopedName*, AST_Type*, const char*)
  { return true; }

  bool gen_union(AST_Union*, UTL_ScopedName*, const std::vector<AST_UnionBranch*>&,
                 AST_Type*, const char*)
  { return true; }
};

#endif
