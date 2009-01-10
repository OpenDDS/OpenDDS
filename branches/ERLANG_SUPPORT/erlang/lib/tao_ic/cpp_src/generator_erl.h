/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_ERL_H
#define TAO_IC_GENERATOR_ERL_H

#include <iostream>

#include "generator.h"

class generator_erl : public generator {
public:
  generator_erl(void);
  ~generator_erl(void);

  bool generate_module(AST_Module *);

  bool generate_constant(AST_Constant *);
};

#endif /* TAO_IC_GENERATOR_ERL_H */
