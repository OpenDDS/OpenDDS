/*
 * $Id$
 */

#include "generator_cpp.h"

using namespace std;

generator_cpp::generator_cpp()
{
}

generator_cpp::~generator_cpp()
{
}

bool
generator_cpp::generate_module(AST_Module *node)
{
  return true;
}

bool
generator_cpp::generate_constant(AST_Constant *node)
{
  return true;
}
