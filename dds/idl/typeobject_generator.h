/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef typeobject_generator_H
#define typeobject_generator_H

#include "dds_generator.h"

class typeobject_generator : public dds_generator {
public:
  bool gen_enum(AST_Enum* node, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(AST_Union* node, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* type, const char* repoid);

  void gen_type_flag_str(std::string& type_flag_str, ExtensibilityKind exten, bool can_be_mutable, AST_Decl* node);

  void gen_member_flag_str(std::string& member_flag_str, TryConstructFailAction trycon, AST_Decl* node);

  static std::string tag_type(UTL_ScopedName* name);
};

#endif
