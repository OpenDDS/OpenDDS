/*
 * $Id$
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
  bool gen_enum(UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* type, const char* repoid);

  void fwd_decl();

  bool first_;
};

#endif
