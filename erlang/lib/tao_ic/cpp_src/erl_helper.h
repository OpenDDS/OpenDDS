/*
 * $Id$
 */

#ifndef TAO_IC_ERL_HELPER_H
#define TAO_IC_ERL_HELPER_H

#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include "ast_decl.h"

class erl_module {
public:
  static const char *ext;

  explicit erl_module(AST_Decl *);
  ~erl_module(void);

  const char *name(void) const;

  void add_export(const char *);

  void add_import(const char *);

  void add_include(const char *);

  const char *filename(void);

  void generate_header(void);

  void generate_attributes(void);

  void generate_includes(void);

  std::ostream &open_stream(bool = true);

  static const char *to_list(std::vector<const char *> &);

  static const char *to_name(AST_Decl *);

private:
  const char *name_;

  std::vector<const char *> exports_;
  std::vector<const char *> imports_;

  std::vector<const char *> includes_;

  std::ofstream out_;
};

enum erl_type {
  ERL_INTEGER,
  ERL_FLOAT,
  ERL_STRING,
  ERL_CHAR,
  ERL_BINARY,
  ERL_RECORD
};

class erl_literal {
public:
  explicit erl_literal(AST_Expression *);
  ~erl_literal(void);

  erl_type type(void) const;

  const char *c_str(void) const;

private:
  const char *data_;
  erl_type type_;  
};

ostream &operator<<(ostream &os, const erl_value &value);

#endif /* TAO_IC_ERL_HELPER_H */
