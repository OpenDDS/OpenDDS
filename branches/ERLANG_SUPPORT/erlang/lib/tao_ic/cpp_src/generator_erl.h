/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_ERL_H
#define TAO_IC_GENERATOR_ERL_H

#include "generator.h"

class generator_erl : public generator {
public:
  generator_erl(void);

  ~generator_erl(void);

  bool generate_constant(AST_Constant *);

  bool generate_enum(AST_Enum *, std::vector<AST_EnumVal *> &);
};

#endif /* TAO_IC_GENERATOR_ERL_H */
