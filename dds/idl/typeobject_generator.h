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
  typeobject_generator()
    : produce_output_(false)
    , produce_xtypes_complete_(false)
    , index_(0)
    , get_type_map_declared_(false)
    , typeid_encoding_(0)
  {}

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

  void produce_output(bool flag)
  {
    produce_output_ = flag;
  }

  void produce_xtypes_complete(bool flag)
  {
    produce_xtypes_complete_ = flag;
  }

  void use_old_typeobject_encoding();

private:

  struct Element {
    AST_Type* type;
    size_t index;
    size_t lowlink;
    bool on_stack;
    std::string name;

    bool operator<(const Element& other) const
    {
      return name < other.name;
    }
  };

  void consider(Element& v, AST_Type* type, const std::string& anonymous_name);
  void strong_connect(AST_Type* type, const std::string& anonymous_name);
  void generate_type_identifier(AST_Type* type, bool force_type_object);

  // Generate minimal and complete type objects and type identifiers for the corresponding types.
  void generate_struct_type_identifier(AST_Type* type);
  void generate_union_type_identifier(AST_Type* type);
  void generate_enum_type_identifier(AST_Type* type);
  void generate_array_type_identifier(AST_Type* type, bool force_type_object);
  void generate_sequence_type_identifier(AST_Type* type, bool force_type_object);
#if OPENDDS_HAS_IDL_MAP
  void generate_map_type_identifier(AST_Type* type, bool force_type_object);
#endif
  void generate_alias_type_identifier(AST_Type* type);
  void generate_primitive_type_identifier(AST_Type* type);

  void update_maps(AST_Type* type,
                   const OpenDDS::XTypes::TypeObject& minimal_to,
                   const OpenDDS::XTypes::TypeObject& complete_to);
  void set_builtin_member_annotations(AST_Decl* member,
    OpenDDS::XTypes::Optional<OpenDDS::XTypes::AppliedBuiltinMemberAnnotations>& annotations);

  OpenDDS::XTypes::TypeIdentifier get_minimal_type_identifier(AST_Type* type);
  OpenDDS::XTypes::TypeIdentifier get_complete_type_identifier(AST_Type* type);
  bool generate(AST_Type* node, UTL_ScopedName* name);
  void declare_get_type_map();

  // Both fields must be constructed when an object is created.
  struct TypeObjectPair {
    OpenDDS::XTypes::TypeObject minimal;
    OpenDDS::XTypes::TypeObject complete;
  };

  typedef std::map<AST_Type*, TypeObjectPair> TypeObjectMap;
  TypeObjectMap type_object_map_;

  // Both fields must be constructed when an object is created.
  struct TypeIdentifierPair {
    OpenDDS::XTypes::TypeIdentifier minimal;
    OpenDDS::XTypes::TypeIdentifier complete;
  };

  typedef std::map<AST_Type*, TypeIdentifierPair> HashTypeIdentifierMap;
  HashTypeIdentifierMap hash_type_identifier_map_;

  typedef std::map<AST_Type*, OpenDDS::XTypes::TypeIdentifier> FullyDescriptiveTypeIdentifierMap;
  FullyDescriptiveTypeIdentifierMap fully_desc_type_identifier_map_;

  OpenDDS::XTypes::TypeMap minimal_type_map_;
  OpenDDS::XTypes::TypeMap complete_type_map_;

  bool produce_output_;
  bool produce_xtypes_complete_;
  size_t index_;
  typedef std::vector<AST_Type*> Stack;
  Stack stack_;
  std::map<AST_Type*, Element> element_;
  bool get_type_map_declared_;
  OpenDDS::DCPS::Encoding* typeid_encoding_;
};

#endif
