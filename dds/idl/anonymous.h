/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef anonymous_H
#define anonymous_H

#include "dds_generator.h"
#include "utl_scoped_name.h"
#include "ast.h"

#include <string>

struct Field {
  static bool is_anonymous_array(AST_Type& field);
  static bool is_anonymous_sequence(AST_Type& field);
  static bool is_anonymous_type(AST_Type& field);
  static std::string get_anonymous_type_name(const std::string& scoped_name);
  static std::string get_type_name(AST_Type& field);

  AST_Type* ast_type;
  AST_Array* arr;
  AST_Sequence* seq;
  AST_Type* ast_elem;
  std::string struct_name;
  std::string scoped_type;
  std::string underscores;
  std::string name;
  std::string type;
  AstTypeClassification::Classification cls;
  std::size_t elem_sz;
  std::string elem;
  std::size_t n_elems;
  std::string length;
  std::string ref;
  std::string const_ref;
  std::string arg;
  std::string unwrap;
  std::string const_unwrap;

  Field(AST_Field& field); //for anonymous types
  Field(UTL_ScopedName* sn, AST_Type* base);
  void init();
  void set_element();
  std::string string_type(AstTypeClassification::Classification c);
};

#endif
