/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_H
#define TAO_IC_GENERATOR_H

#include <vector>

#include "ast_constant.h"
#include "ast_enum.h"
#include "ast_enum_val.h"
#include "ast_field.h"
#include "ast_structure.h"

class generator
{
public:
  virtual ~generator();

  virtual bool generate_constant(AST_Constant* node);

  virtual bool generate_enum(AST_Enum* node, std::vector<AST_EnumVal*>& v);
                             
  virtual bool generate_structure(AST_Structure* node, std::vector<AST_Field*>& v);
};

class generator_composite : public generator
{
public:
  typedef std::vector<generator*>::iterator iterator;
  typedef std::vector<generator*>::const_iterator const_iterator;

  explicit generator_composite(bool auto_delete = false);

  ~generator_composite();

  void add(generator *gen);

  void delete_all();

  iterator begin();
  const_iterator begin() const;

  iterator end();
  const_iterator end() const;

  // Composite operations
  bool generate_constant(AST_Constant* node);

  bool generate_enum(AST_Enum* node, std::vector<AST_EnumVal*>& v);
                     
  bool generate_structure(AST_Structure* node, std::vector<AST_Field*>& v);

private:
  bool auto_delete_;

  std::vector<generator*> generators_;
};

#endif /* TAO_IC_GENERATOR_H */
