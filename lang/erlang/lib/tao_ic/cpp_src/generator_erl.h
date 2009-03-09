/*
 * $Id$
 */

#ifndef TAO_IC_GENERATOR_ERL_H
#define TAO_IC_GENERATOR_ERL_H

#include "generator.h"

class generator_erl : public generator
{
public:
  generator_erl();

  ~generator_erl();

  bool generate_module(AST_Module* node, std::vector<AST_Constant*>& v);

  bool generate_enum(AST_Enum* node, std::vector<AST_EnumVal*>& v);

  bool generate_structure(AST_Structure* node, std::vector<AST_Field*>& v);
};

#endif /* TAO_IC_GENERATOR_ERL_H */
