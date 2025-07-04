/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef marshal_generator_H
#define marshal_generator_H

#include "dds_generator.h"

class marshal_generator : public dds_generator {
public:
  bool gen_enum(AST_Enum* node, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* base, const char* repoid);

  bool gen_union(AST_Union* node, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* discriminator,
                 const char* repoid);

  static void generate_dheader_code(const std::string& code, bool dheader_required,
                                    bool is_ser_func = true, const char* indent = "  ");

  static void gen_field_getValueFromSerialized(AST_Structure* node, const std::string& clazz);

  static void gen_map_skip_over(AST_Map* map);

private:
  void gen_union_default(AST_UnionBranch* branch, const std::string& varname);
};

#endif
