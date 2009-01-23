/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_ERL_H
#define TAO_IC_GENERATOR_ERL_H

#include "generator.h"

class generator_erl : public generator
{
public:
  generator_erl(void);

  ~generator_erl(void);

  bool generate_constant(AST_Constant* node);

  bool generate_enum(AST_Enum* node, std::vector<AST_EnumVal*>& values);
};

#endif /* TAO_IC_GENERATOR_ERL_H */
