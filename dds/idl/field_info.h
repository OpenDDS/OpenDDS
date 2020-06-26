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

  AST_Type* ast_type_;
  AST_Array* arr_;
  AST_Sequence* seq_;
  AST_Type* ast_elem_;
  AST_Type* act_;
  AstTypeClassification::Classification cls_;

  std::string name_;
  std::string type_;
  std::string struct_name_;
  std::string scoped_type_;
  std::string underscores_;

  std::size_t elem_sz_;
  std::string elem_;
  std::size_t n_elems_;
  std::string length_;
  std::string ref_;
  std::string const_ref_;
  std::string ptr_;
  std::string arg_;
  std::string unwrap_;
  std::string const_unwrap_;

  explicit FieldInfo(AST_Field& field); //for anonymous types
  FieldInfo(UTL_ScopedName* sn, AST_Type* base);
  void init();
  void set_element();
  std::string string_type(AstTypeClassification::Classification c);
};

#endif
