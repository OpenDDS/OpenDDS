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

  virtual bool generate_constant(AST_Constant *) { return true; }
};

class generator_composite : public generator {
public:
  typedef std::vector<generator *>::iterator iterator;
  typedef std::vector<generator *>::const_iterator const_iterator;

  explicit generator_composite(bool = false);
  ~generator_composite(void);

  void add(generator *);

  void delete_all(void);

  iterator begin(void);
  const_iterator begin(void) const;

  iterator end(void);
  const_iterator end(void) const;

  // composite operations
  bool generate_constant(AST_Constant *);

private:
  bool auto_delete_;

  std::vector<generator *> generators_;
};

#endif /* TAO_IC_GENERATOR_H */
