/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_H
#define TAO_IC_GENERATOR_H

#include <vector>

#include "ast_constant.h"
#include "ast_enum.h"
#include "ast_enum_val.h"

class generator
{
public:
  virtual ~generator(void);

  virtual bool generate_constant(AST_Constant* node);

  virtual bool generate_enum(AST_Enum* node, std::vector<AST_EnumVal*>& values);
};

class generator_composite : public generator
{
public:
  typedef std::vector<generator*>::iterator iterator;

  explicit generator_composite(bool auto_delete = false);

  ~generator_composite(void);

  void add(generator *g);

  void delete_all(void);

  iterator begin(void);

  iterator end(void);

  // Composite operations
  bool generate_constant(AST_Constant* node);

  bool generate_enum(AST_Enum* node, std::vector<AST_EnumVal*>& values);

private:
  bool auto_delete_;

  std::vector<generator*> generators_;
};

#endif /* TAO_IC_GENERATOR_H */
