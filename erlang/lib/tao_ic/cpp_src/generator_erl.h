/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_ERL_H
#define TAO_IC_GENERATOR_ERL_H

#include "generator.h"

class generator_erl : public generator {
public:
  generator_erl(void);
  virtual ~generator_erl(void);

  virtual void generate_constant(AST_Constant *);
};

#endif /* TAO_IC_GENERATOR_ERL_H */
