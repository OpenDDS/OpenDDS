/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ts_generator_H
#define ts_generator_H

#include "dds_generator.h"

#include <string>

namespace java_ts_generator {
  void generate(UTL_ScopedName* name);
}

namespace face_ts_generator {
  void generate(UTL_ScopedName* name);
}

class ts_generator : public dds_generator {
public:
  ts_generator();

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef*, UTL_ScopedName*, AST_Type*, const char*)
  { return true; }

  bool gen_union(AST_Union*, UTL_ScopedName*, const std::vector<AST_UnionBranch*>&,
                 AST_Type*, const char*);

private:
  std::string idl_template_;
};

#endif
