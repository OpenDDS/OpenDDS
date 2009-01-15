/*
 * $Id$
 */

#ifndef TAO_IC_ERL_UTILITY_H
#define TAO_IC_ERL_UTILITY_H

#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include "ast_decl.h"
#include "ast_expression.h"

class erl_module {
public:
  static const char *ext;

  explicit erl_module(AST_Decl *);

  ~erl_module(void);

  void add_export(const char *);

  void add_import(const char *);

  void add_include(const char *);

  const char *filename(void) const;

  void generate_header(void);

  void generate_attributes(void);

  void generate_includes(void);

  std::ostream &open_stream(bool = true);

  std::string to_list(std::vector<const char *> &);

  std::string to_module_name(AST_Decl *);

private:
  const std::string module_name_;

  std::string filename_;

  std::ofstream os_;

  // Module exports (list)
  std::vector<const char *> exports_;

  // Module imports (list)
  std::vector<const char *> imports_;

  // Module includes
  std::vector<const char *> includes_;
};

class erl_literal {
public:
  explicit erl_literal(AST_Expression *);
  explicit erl_literal(AST_Expression::AST_ExprValue *);

  ~erl_literal(void);

  std::string str(void) const;

  static std::string to_str(AST_Expression::AST_ExprValue *);

private:
  const std::string str_;
};

ostream &operator<<(ostream &, const erl_literal &);

#endif /* TAO_IC_ERL_UTILITY_H */
