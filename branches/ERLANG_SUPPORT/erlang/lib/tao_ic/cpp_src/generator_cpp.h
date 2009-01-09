/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_CPP_H
#define TAO_IC_GENERATOR_CPP_H

#include "generator.h"

class generator_cpp : public generator {
public:
  generator_cpp(void);
  virtual ~generator_cpp(void);

  virtual void generate_constant(AST_Constant *);
};

#endif /* TAO_IC_GENERATOR_CPP_H */
