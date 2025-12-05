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

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(AST_Union* node, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* type, const char* repoid);

private:
  bool first_struct_;

  static std::string gen_union_branch(const std::string&, AST_Decl* branch, const std::string&,
                                      AST_Type* br_type, const std::string&, bool, Intro&, const std::string&);
  static void generate_anon_fields(AST_Structure* node);
};

#endif
