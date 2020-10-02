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
    size_t group;
    std::string name;

    Element() {}

    Element(AST_Type* a_type, size_t a_group, const std::string& a_name)
      : type(a_type)
      , group(a_group)
      , name(a_name)
    {}
  };

  size_t compute_dependencies(AST_Type* type, const std::string& anonymous_name);
  void generate_minimal_type_identifier(AST_Type* type, bool force_type_object);
  static bool name_sorter(const Element& x, const Element& y);
  void generate_minimal(AST_Type* type);
  OpenDDS::XTypes::TypeObject get_minimal_type_object(AST_Type* type);
  OpenDDS::XTypes::TypeIdentifier get_minimal_type_identifier(AST_Type* type);
  bool generate(AST_Type* node, UTL_ScopedName* name);

  typedef std::map<AST_Type*, OpenDDS::XTypes::TypeObject> MinimalTypeObjectMap;
  MinimalTypeObjectMap minimal_type_object_map_;
  typedef std::map<AST_Type*, OpenDDS::XTypes::TypeIdentifier> MinimalTypeIdentifierMap;
  MinimalTypeIdentifierMap minimal_type_identifier_map_;
  OpenDDS::XTypes::TypeMap minimal_type_map_;

  typedef std::vector<Element> SortedDependencies;
  SortedDependencies sorted_dependencies_;
  std::set<AST_Type*> in_sorted_dependencies_;
  std::map<AST_Type*, size_t> in_progress_;
  size_t group_counter_ = 0;
};

#endif
