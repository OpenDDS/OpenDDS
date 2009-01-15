/*
 * $Id$
 */

#include "ast_expression.h"

#include "erl_utility.h"
#include "generator_erl.h"

using namespace std;

generator_erl::generator_erl()
{
}

generator_erl::~generator_erl()
{
}
        
bool
generator_erl::generate_constant(AST_Constant *node)
{
  erl_module module(node);

  module.add_export("value/0");

  ostream &os = module.open_stream();
  if (!os) return false;

  os << "value() -> " << erl_literal(node->constant_value()) << "." << endl;

  return true;
}
