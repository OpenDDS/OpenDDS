/*
 * $Id$
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include <vector>

#include "ast_constant.h"
#include "ast_enum.h"
#include "ast_enum_val.h"
#include "ast_field.h"
#include "ast_module.h"
#include "ast_structure.h"
#include "ast_union.h"
#include "ast_union_branch.h"

class generator
{
public:
  generator();

  virtual ~generator();

  virtual bool generate_constant(AST_Constant* node);

  virtual bool generate_enum(AST_Enum* node, std::vector<AST_EnumVal*>& v);
                             
  virtual bool generate_structure(AST_Structure* node, std::vector<AST_Field*>& v);

  virtual bool generate_union(AST_Union* node, std::vector<AST_UnionBranch*>& v);
};

class generator_composite : public generator
{
public:
  typedef std::vector<generator*>::iterator iterator;
  typedef std::vector<generator*>::const_iterator const_iterator;

  generator_composite();

  ~generator_composite();

  void add(generator *g);

  void delete_all();

  iterator begin();
  const_iterator begin() const;

  iterator end();
  const_iterator end() const;

  // Composite operations
  bool generate_constant(AST_Constant* node);

  bool generate_enum(AST_Enum* node, std::vector<AST_EnumVal*>& v);
                     
  bool generate_structure(AST_Structure* node, std::vector<AST_Field*>& v);

  bool generate_union(AST_Union* node, std::vector<AST_UnionBranch*>& v);

private:
  std::vector<generator*> generators_;
};

#endif /* GENERATOR_H */
