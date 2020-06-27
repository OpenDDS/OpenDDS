/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef anonymous_H
#define anonymous_H

#include "dds_generator.h"
#include <utl_scoped_name.h>
#include <ast.h>

#include <string>

struct FieldInfo {
  struct EleLen {
    AST_Type* ele_;
    std::size_t len_;
    EleLen(FieldInfo& af);
    struct Cmp { bool operator()(const EleLen& a, const EleLen& b) const; };
  };
  typedef std::set<EleLen, EleLen::Cmp> EleLenSet;

  static std::string get_type_name(AST_Type& field);
  static std::string string_type(AstTypeClassification::Classification c);
  static std::string to_cxx_type(AST_Type* type, std::size_t& size);

  AST_Type* type_;
  std::string name_;
  AST_Type* act_;
  AstTypeClassification::Classification cls_;
  AST_Array* arr_;
  AST_Sequence* seq_;
  AST_Type* as_base_;
  AST_Type* as_act_;
  AstTypeClassification::Classification as_cls_;

  std::string scoped_type_;
  std::string struct_name_;
  std::string type_name_;

  std::size_t elem_sz_;
  std::string elem_;
  std::size_t n_elems_;
  std::string length_;

  std::string underscored_;
  std::string unwrap_;
  std::string const_unwrap_;
  std::string arg_;
  std::string ref_;
  std::string const_ref_;
  std::string ptr_;

  explicit FieldInfo(AST_Field& field); //for anonymous types
  FieldInfo(UTL_ScopedName* sn, AST_Type* base);
  void init();
  void set_element();
};

#endif
