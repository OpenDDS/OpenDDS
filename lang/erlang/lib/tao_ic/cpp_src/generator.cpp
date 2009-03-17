/*
 * $Id$
 */

#include "generator.h"

using namespace std;

generator::generator()
{
}

generator::~generator()
{
}

bool
generator::generate_constant(AST_Constant*)
{
  return true;
}

bool
generator::generate_enum(AST_Enum*, vector<AST_EnumVal*>&)
{
  return true;
}

bool
generator::generate_structure(AST_Structure*, vector<AST_Field*>&)
{
  return true;
}

bool
generator::generate_union(AST_Union*, vector<AST_UnionBranch*>&)
{
  return true;
}


generator_composite::generator_composite()
{
}

generator_composite::~generator_composite()
{
}

void
generator_composite::add(generator* g)
{
  generators_.push_back(g);
}

void
generator_composite::delete_all()
{
  for (iterator it(begin()); it != end(); ++it)
  {
    delete *it;
  }
  generators_.clear();
}

generator_composite::iterator
generator_composite::begin()
{
  return generators_.begin();
}

generator_composite::const_iterator
generator_composite::begin() const
{
  return generators_.begin();
}

generator_composite::iterator
generator_composite::end()
{
  return generators_.end();
}

generator_composite::const_iterator
generator_composite::end() const
{
  return generators_.end();
}

bool
generator_composite::generate_constant(AST_Constant* node)
{
  for (iterator it(begin()); it != end(); ++it)
  {
    if (!(*it)->generate_constant(node))
    {
      return false;
    }
  }
  return true;
}

bool
generator_composite::generate_enum(AST_Enum* node, vector<AST_EnumVal*>& v)
{
  for (iterator it(begin()); it != end(); ++it)
  {
    if (!(*it)->generate_enum(node, v))
    {
      return false;
    }
  }
  return true;
}

bool
generator_composite::generate_structure(AST_Structure* node, vector<AST_Field*>& v)
{
  for (iterator it(begin()); it != end(); ++it)
  {
    if (!(*it)->generate_structure(node, v))
    {
      return false;
    }
  }
  return true;
}

bool
generator_composite::generate_union(AST_Union* node, vector<AST_UnionBranch*>& v)
{
  for (iterator it(begin()); it != end(); ++it)
  {
    if (!(*it)->generate_union(node, v))
    {
      return false;
    }
  }
  return true;
}
