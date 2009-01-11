/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_H
#define TAO_IC_GENERATOR_H

#include <vector>

#include "ast_constant.h"
#include "ast_module.h"
#include "utl_identifier.h"

class generator {
public:
  virtual ~generator(void) {}

  virtual bool generate_module(AST_Module *) = 0;

  virtual bool generate_constant(AST_Constant *) = 0;
};

class generator_composite : public generator {
public:
  typedef std::vector<generator *>::iterator iterator;
  typedef std::vector<generator *>::const_iterator const_iterator;

  generator_composite(bool);
  ~generator_composite(void);

  void add(generator *);

  void delete_all(void);

  iterator begin(void);
  const_iterator begin(void) const;

  iterator end(void);
  const_iterator end(void) const;

  // composite operations
  bool generate_module(AST_Module *);

  bool generate_constant(AST_Constant *);

private:
  bool auto_delete_;
  std::vector<generator *> generators_;
};

#endif /* TAO_IC_GENERATOR_H */
