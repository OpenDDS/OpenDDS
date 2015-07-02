// -*- C++ -*-
/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef wireshark_generator_H
#define wireshark_generator_H

#include "dds_generator.h"

class wireshark_generator : public dds_generator {
public:
  wireshark_generator()
  {}

  bool gen_enum(AST_Enum*, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef*, UTL_ScopedName*, AST_Type*, const char*);

  bool gen_union(AST_Union*, UTL_ScopedName*, const std::vector<AST_UnionBranch*>&,
                 AST_Type*, const char*);

private:
  void write_common(UTL_ScopedName* name,
                    const char* kind,
                    const char* repoid);

  void gen_array(UTL_ScopedName* tdname, AST_Array* arr);
};

#endif
