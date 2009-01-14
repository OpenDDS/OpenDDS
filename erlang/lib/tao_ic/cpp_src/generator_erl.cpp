/*
 * $Id$
 */

#include "ast_expression.h"
#include "utl_string.h"

#include "erl_helper.h"
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

  ostream &out = module.open_stream();
  if (!out) return false; // bad stream

  out << "value() -> " << erl_literal(node->constant_value()) << '.' << endl;
  return true;
}

//
//  AST_Expression::AST_ExprValue *ev = node->constant_value()->ev();
//  switch (ev->et) {
//  case AST_Expression::EV_short:
//    out << ev->u.sval;
//    break;
//  case AST_Expression::EV_ushort:
//    out << ev->u.usval;
//    break;
//  case AST_Expression::EV_long:
//    out << ev->u.lval;
//    break;
//  case AST_Expression::EV_ulong:
//    out << ev->u.ulval;
//    break;
//#ifndef ACE_LACKS_LONGLONG_T
//  case AST_Expression::EV_longlong:
//    out << ev->u.llval;
//    break;
//  case AST_Expression::EV_ulonglong:
//    out << ev->u.ullval;
//    break;
//#endif /* ACE_LACKS_LONGLONG_T */
//
//  case AST_Expression::EV_float:
//    out << showpoint << ev->u.fval;
//    break;
//  case AST_Expression::EV_double:
//    out << showpoint << ev->u.dval;
//    break;
//
//  case AST_Expression::EV_char:
//    out << '$' << ev->u.cval;
//    break;
//  case AST_Expression::EV_wchar:
//    out << '$' << ev->u.wcval;
//    break;
//
//  case AST_Expression::EV_octet:
//    out << "<<" << unsigned(ev->u.oval) << ">>";
//    break;
//
//  case AST_Expression::EV_bool:
//    out << boolalpha << ev->u.bval;
//    break;
//
//  case AST_Expression::EV_string:
//    out << '"' << ev->u.strval->get_string() << '"';
//    break;
//  case AST_Expression::EV_wstring:
//    out << '"' << ev->u.wstrval << '"';
//    break;
//
//  default:
//    return false; // not supported
//  }

