/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef FIELD_INFO_H
#define FIELD_INFO_H

#include "dds_generator.h"

#include <utl_scoped_name.h>
#include <ast.h>

#include <ace/Bound_Ptr.h>

#include <string>

struct FieldInfo {
  struct EleLen {
    AstTypeClassification::Classification cls_;
    const char* base_name_;
    AstTypeClassification::Classification base_cls_;
    typedef ACE_Strong_Bound_Ptr<const EleLen, ACE_Null_Mutex> Container;
    Container base_container_;
    std::size_t len_;
    explicit EleLen(AST_Type* type);
    bool operator<(const EleLen& o) const;
  };
  typedef std::set<EleLen> EleLenSet;

  static const std::string scope_op;
  static bool cxx11();
  static std::string at_pfx();
  static std::string scoped_type(AST_Type& field_type, const std::string& field_name);
  static std::string underscore(const std::string& scoped);
  static std::string ref(const std::string& scoped, const std::string& const_s = "const ");

  AST_Type* type_;
  const std::string name_;
  const std::string scoped_type_;
  const std::string underscored_;
  const std::string struct_name_;
  const std::string type_name_;
  AST_Type* act_;
  const AstTypeClassification::Classification cls_;
  AST_Array* arr_;
  AST_Sequence* seq_;
#if OPENDDS_HAS_IDL_MAP
  AST_Map* map_;
#endif
  AST_Type* as_base_;
  AST_Type* as_act_;
  const AstTypeClassification::Classification as_cls_;
  const std::string scoped_elem_;
  const std::string underscored_elem_;
  const std::string elem_ref_;
  const std::string elem_const_ref_;

  std::size_t n_elems_;
  std::string length_;

  std::string unwrap_;
  std::string const_unwrap_;
  std::string arg_;
  std::string ref_;
  std::string const_ref_;
  std::string ptr_;

  explicit FieldInfo(AST_Field& field);
  bool is_new(EleLenSet& el_set) const;
  bool anonymous() const;
};

#endif
