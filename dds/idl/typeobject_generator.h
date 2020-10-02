/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef typeobject_generator_H
#define typeobject_generator_H

#include "dds_generator.h"
#include "dds/DCPS/XTypes/TypeObject.h"

class typeobject_generator : public dds_generator {
public:
  void gen_prologue();

  void gen_epilogue();

  bool gen_enum(AST_Enum* node, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* type, const char* repoid);

  bool gen_union(AST_Union* node, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* type, const char* repoid);

  static std::string tag_type(UTL_ScopedName* name);

private:

  struct Element {
    AST_Type* type;
    size_t index;
    size_t lowlink;
    bool on_stack;
    std::string name;
  };

  void consider(Element& v, AST_Type* type, const std::string& anonymous_name);
  void strong_connect(AST_Type* type, const std::string& anonymous_name);
  void generate_minimal_type_identifier(AST_Type* type, bool force_type_object);
  static bool name_sorter(const Element& x, const Element& y);
  OpenDDS::XTypes::TypeObject get_minimal_type_object(AST_Type* type);
  OpenDDS::XTypes::TypeIdentifier get_minimal_type_identifier(AST_Type* type);
  bool generate(AST_Type* node, UTL_ScopedName* name);

  typedef std::map<AST_Type*, OpenDDS::XTypes::TypeObject> MinimalTypeObjectMap;
  MinimalTypeObjectMap minimal_type_object_map_;
  typedef std::map<AST_Type*, OpenDDS::XTypes::TypeIdentifier> MinimalTypeIdentifierMap;
  MinimalTypeIdentifierMap minimal_type_identifier_map_;
  OpenDDS::XTypes::TypeMap minimal_type_map_;

  size_t index_ = 0;
  typedef std::vector<AST_Type*> Stack;
  Stack stack_;
  std::map<AST_Type*, Element> element_;
};

#endif
