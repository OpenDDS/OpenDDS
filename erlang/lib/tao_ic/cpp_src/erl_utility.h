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

class erl_name {
public:
  explicit erl_name(AST_Decl *, bool = true);

  ~erl_name(void);

  std::string str(void) const;

  operator std::string() const;

private:
  std::string str_;

  friend std::ostream &operator<<(std::ostream &, const erl_name &);
};

class erl_module {
public:
  static const char *ext;

  explicit erl_module(AST_Decl *);

  ~erl_module(void);

  const char *filename(void) const;

  void add_export(const std::string &);

  void add_import(const std::string &);

  void add_include(const std::string &);

  void generate_header(void);

  void generate_attributes(void);

  void generate_includes(void);

  std::ostream &open_stream(bool = true);

private:
  erl_name name_;

  std::string filename_;

  std::ofstream os_;

  // Module exports (list)
  std::vector<std::string> exports_;

  // Module imports (list)
  std::vector<std::string> imports_;

  // Module includes
  std::vector<std::string> includes_;

  friend std::ostream &operator<<(std::ostream &, const erl_module &);
};

class erl_literal {
public:
  explicit erl_literal(AST_Expression *);

  ~erl_literal(void);

  std::string str(void) const;

  static std::string to_str(AST_Expression *);

private:
  std::string str_;

  friend std::ostream &operator<<(std::ostream &, const erl_literal &);
};

std::string to_list(std::vector<std::string> &);

#endif /* TAO_IC_ERL_UTILITY_H */
