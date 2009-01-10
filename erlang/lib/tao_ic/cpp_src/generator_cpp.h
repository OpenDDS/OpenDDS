/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_CPP_H
#define TAO_IC_GENERATOR_CPP_H

#include "generator.h"

class generator_cpp : public generator {
public:
  generator_cpp(void);
  ~generator_cpp(void);

  bool generate_module(AST_Module *);

  bool generate_constant(AST_Constant *);
};

#endif /* TAO_IC_GENERATOR_CPP_H */
