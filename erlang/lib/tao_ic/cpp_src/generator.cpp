/*
 * $Id$
 */

#include <vector>

#include "generator.h"

using namespace std;

generator::~generator()
{
}

bool
generator::generate_constant(AST_Constant *)
{
  return true;
}

bool
generator::generate_enum(AST_Enum *, vector<AST_EnumVal *> &)
{
  return true;
}

generator_composite::generator_composite(bool auto_delete)
  : auto_delete_(auto_delete)
{
}

generator_composite::~generator_composite()
{
  if (auto_delete_) {
    delete_all();
  }
}

void
generator_composite::add(generator *gen)
{
  generators_.push_back(gen);
}

void
generator_composite::delete_all()
{
  generator_composite::iterator it = begin();
  while (it != end()) {
    delete *it++;
  }
  generators_.clear();
}

generator_composite::iterator
generator_composite::begin()
{
  return generators_.begin();
}

generator_composite::iterator
generator_composite::end()
{
  return generators_.end();
}

bool
generator_composite::generate_constant(AST_Constant *node)
{
  generator_composite::iterator it(begin());
  for (; it != end(); ++it) {
    if (!(*it)->generate_constant(node)) {
      return false;
    }
  }
  return true;
}

bool
generator_composite::generate_enum(AST_Enum *node,
                                   vector<AST_EnumVal *> &values)
{
  generator_composite::iterator it(begin());
  for (; it != end(); ++it) {
    if (!(*it)->generate_enum(node, values)) {
      return false;
    }
  }
  return true;
}
