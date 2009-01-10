/*
 * $Id$
 */

#include "generator_erl.h"

using namespace std;

generator_erl::generator_erl()
{
}

generator_erl::~generator_erl()
{
}

bool
generator_erl::generate_module(AST_Module *node)
{
  return true;
}

bool
generator_erl::generate_constant(AST_Constant *node)
{
  return true;
}
