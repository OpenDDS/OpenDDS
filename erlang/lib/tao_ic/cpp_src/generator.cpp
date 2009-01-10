/*
 * $Id$
 */

#include <vector>

#include "generator.h"

using namespace std;

generator_composite::generator_composite(bool auto_delete = false)
  : auto_delete_(auto_delete)
{
}

generator_composite::~generator_composite()
{
  if (this->auto_delete_) {
    this->delete_all();
  }
}

void
generator_composite::add(generator *gen)
{
  this->generators_.push_back(gen);
}

void
generator_composite::delete_all()
{
  generator_composite::iterator it = this->begin();
  while (it != this->end()) {
    delete *it++;
  }
  this->generators_.clear();
}

generator_composite::iterator
generator_composite::begin()
{
  return this->generators_.begin();
}

generator_composite::const_iterator
generator_composite::begin() const
{
  return this->generators_.begin();
}

generator_composite::iterator
generator_composite::end()
{
  return this->generators_.end();
}

generator_composite::const_iterator
generator_composite::end() const
{
  return this->generators_.end();
}

bool
generator_composite::generate_module(AST_Module *node)
{
  generator_composite::iterator it = this->begin();
  for (; it != this->end(); ++it) {
    if (!(*it)->generate_module(node)) {
      return false;
    }
  }
  return true;
}

bool
generator_composite::generate_constant(AST_Constant *node)
{
  generator_composite::iterator it = this->begin();
  for (; it != this->end(); ++it) {
    if (!(*it)->generate_constant(node)) {
      return false;
    }
  }
  return true;
}
