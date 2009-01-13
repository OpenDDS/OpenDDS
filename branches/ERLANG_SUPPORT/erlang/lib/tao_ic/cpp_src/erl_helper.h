/*
 * $Id$
 */

#ifndef TAO_IC_ERL_HELPER_H
#define TAO_IC_ERL_HELPER_H

#include <fstream>
#include <string>
#include <vector>

#include "ast_decl.h"

class erl_helper {
public:
  static const char *ext;

  explicit erl_helper(AST_Decl *);
  ~erl_helper(void);

  void add_export(const char *);

  void add_import(const char *);

  void add_include(const char *);

  const char *filename(void);

  void generate_header(void);

  void generate_attributes(void);

  void generate_includes(void);

  std::ofstream &open_stream(bool = true);

  static const char *to_list(std::vector<const char *> &);

  static const char *to_module(AST_Decl *);

private:
  const char *module_;

  std::vector<const char *> exports_;

  std::vector<const char *> imports_;

  std::vector<const char *> includes_;

  std::ofstream out_;
};

#endif /* TAO_IC_ERL_HELPER_H */
