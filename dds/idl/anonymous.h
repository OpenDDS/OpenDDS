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

struct Field {
  static bool is_anonymous_array(AST_Type& field);
  static bool is_anonymous_sequence(AST_Type& field);
  static bool is_anonymous_type(AST_Type& field);
  static std::string get_anonymous_type_name(const std::string& scoped_name);
  static std::string get_type_name(AST_Type& field);

  AST_Type* ast_type_;
  AST_Array* arr_;
  AST_Sequence* seq_;
  AST_Type* ast_elem_;
  std::string struct_name_;
  std::string scoped_type_;
  std::string underscores_;
  std::string name_;
  std::string type_;
  AstTypeClassification::Classification cls_;
  std::size_t elem_sz_;
  std::string elem_;
  std::size_t n_elems_;
  std::string length_;
  std::string ref_;
  std::string const_ref_;
  std::string arg_;
  std::string unwrap_;
  std::string const_unwrap_;

  explicit Field(AST_Field& field); //for anonymous types
  Field(UTL_ScopedName* sn, AST_Type* base);
  void init();
  void set_element();
  std::string string_type(AstTypeClassification::Classification c);
};

#endif
