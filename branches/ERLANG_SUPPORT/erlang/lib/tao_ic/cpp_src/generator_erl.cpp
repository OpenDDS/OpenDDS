/*
 * $Id$
 */

#include "ast_expression.h"

#include "erl_utility.h"
#include "generator_erl.h"

using namespace std;

generator_erl::generator_erl()
{}

generator_erl::~generator_erl()
{}

bool generator_erl::generate_constant(AST_Constant* node)
{
  erl_module module(node);

  module.add_export("value/0");

  ostream& os = module.open_stream();
  if (!os) return false; // bad stream

  os << "value() -> " << erl_literal(node->constant_value()) << "." << endl;

  return true;
}

bool generator_erl::generate_enum(AST_Enum* node, vector<AST_EnumVal*>& values)
{
  erl_module module(node);

  // Generate exports (arity always 0)
  for (vector<AST_EnumVal*>::iterator it(values.begin());
       it != values.end(); ++it)
  {
    string s = erl_name(*it);
    module.add_export(s + "/0");
  }

  module.add_export("from_int/1");
  module.add_export("value/1");

  ostream& os = module.open_stream();
  if (!os) return false; // bad stream

  // Generate functions
  for (vector<AST_EnumVal*>::iterator it(values.begin());
       it != values.end(); ++it)
  {
    os << erl_name(*it) << "() -> {?MODULE, " <<
          erl_literal((*it)->constant_value()) << "}." << endl;
  }

  os << endl
     << "from_int(I) -> {?MODULE, I}." << endl
     << endl
     << "value(E) -> {?MODULE, I} = E, I." << endl;

  return true;
}
