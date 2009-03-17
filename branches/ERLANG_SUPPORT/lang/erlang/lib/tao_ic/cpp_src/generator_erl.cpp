/*
 * $Id$
 */

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
generator_erl::generate_constant(AST_Constant* node)
{
  // Generate module (.erl)
  erl_module module(node->name());

  module.add_export("value/0");

  ostream& os = module.open_stream();
  if (!os) return false; // bad stream

  /// Generate functions
  os << "value() -> " << erl_literal(node->constant_value()) << "." << endl;

  return true;
}

bool
generator_erl::generate_enum(AST_Enum* node, vector<AST_EnumVal*>& v)
{
  // Generate module (.erl)
  erl_module module(node->name());

  /// AST_EnumVal exports (arity always 0)
  module.add_exports(v.begin(), v.end(), 0);
  
  module.add_export("from_int/1");
  module.add_export("value/1");

  ostream& os = module.open_stream();
  if (!os) return false; // bad stream

  /// Generate functions
  for (vector<AST_EnumVal*>::iterator it(v.begin()); it != v.end(); ++it)
  {
    os << erl_identifier((*it)->local_name()) << "() -> {?MODULE, " <<
          erl_literal((*it)->constant_value()) << "}." << endl;
  }

  os << endl
     << "from_int(I) -> {?MODULE, I}." << endl
     << endl
     << "value({?MODULE, I}) -> I." << endl;

  return true;
}

bool
generator_erl::generate_structure(AST_Structure* node, vector<AST_Field*>& v)
{
  erl_header header(node->name());
  erl_module module(node->name());

  erl_identifier_list fields(v.begin(), v.end());
 
  { // Generate header (.hrl)
    ostream& os = header.open_stream();
    if (!os) return false; // bad stream

    os << "-record(" << module << ", {" << fields << "})." << endl;
  }

  { // Generate module (.erl)
    module.add_include(header.basename());
    
    module.add_export("id/0");
    module.add_export("new/0");
    module.add_export("new", fields.size());

    ostream& os = module.open_stream();
    if (!os) return false; // bad stream

    /// Generate functions
    os << "id() -> \"" << node->repoID() << "\"." << endl
       << endl;

    os << "new() -> #" << module << "{}." << endl
       << endl;

    os << "new(" << fields.as_param_list() << ") -> #" << module <<
          "{" << fields.as_init_list() << "}." << endl;
  }

  return true;
}

bool
generator_erl::generate_union(AST_Union* node, vector<AST_UnionBranch*>& v)
{
  // Generate module (.erl)
  erl_module module(node->name());

  ostream& os = module.open_stream();
  if (!os) return false; // bad stream

  return true;
}
