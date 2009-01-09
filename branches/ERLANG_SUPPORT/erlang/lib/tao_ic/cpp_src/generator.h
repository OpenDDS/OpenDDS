/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_H
#define TAO_IC_GENERATOR_H

#include <vector>

#include "ast_constant.h"

class generator {
public:
  virtual ~generator(void) {}

  virtual void generate_constant(AST_Constant *) = 0;
};

class generator_set : std::vector<class generator> {
public:
  generator_set(void);
  ~generator_set(void);

  void add(const generator &);

  void generate_constant(AST_Constant *);
};

#endif /* TAO_IC_GENERATOR_H */
