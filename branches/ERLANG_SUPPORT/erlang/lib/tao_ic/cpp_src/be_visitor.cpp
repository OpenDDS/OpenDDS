/*
 * $Id$
 */

#include "ace_compat.h"
#include "be_visitor.h"

be_visitor::be_visitor()
{
}

be_visitor::~be_visitor()
{
}

int
be_visitor::visit_decl(AST_Decl *node)
{
  return 0;
}

int
be_visitor::visit_scope(UTL_Scope *node)
{
  return 0;
}

int
be_visitor::visit_type(AST_Type *node)
{
  return 0;
}

int
be_visitor::visit_predefined_type(AST_PredefinedType *node)
{
  return 0;
}
int
be_visitor::visit_module(AST_Module *node)
{
  return 0;
}

int
be_visitor::visit_interface(AST_Interface *node)
{
  return 0;
}

int
be_visitor::visit_interface_fwd(AST_InterfaceFwd *node)
{
  return 0;
}

int
be_visitor::visit_valuetype(AST_ValueType *node)
{
  return 0;
}

int
be_visitor::visit_valuetype_fwd(AST_ValueTypeFwd *node)
{
  return 0;
}

int
be_visitor::visit_component(AST_Component *node)
{
  return 0;
}

int
be_visitor::visit_component_fwd(AST_ComponentFwd *node)
{
  return 0;
}

int
be_visitor::visit_home(AST_Home *node)
{
  return 0;
}

int
be_visitor::visit_eventtype(AST_EventType *node)
{
  return 0;
}

int
be_visitor::visit_eventtype_fwd(AST_EventTypeFwd *node)
{
  return 0;
}

int
be_visitor::visit_factory(AST_Factory *node)
{
  return 0;
}

int
be_visitor::visit_structure(AST_Structure *node)
{
  return 0;
}

int
be_visitor::visit_structure_fwd(AST_StructureFwd *node)
{
  return 0;
}

int
be_visitor::visit_exception(AST_Exception *node)
{
  return 0;
}

int
be_visitor::visit_expression(AST_Expression *node)
{
  return 0;
}

int
be_visitor::visit_enum(AST_Enum *node)
{
  return 0;
}

int
be_visitor::visit_enum_val(AST_EnumVal *node)
{
  return 0;
}

int
be_visitor::visit_operation(AST_Operation *node)
{
  return 0;
}

int
be_visitor::visit_field(AST_Field *node)
{
  return 0;
}

int
be_visitor::visit_argument(AST_Argument *node)
{
  return 0;
}

int
be_visitor::visit_attribute(AST_Attribute *node)
{
  return 0;
}

int
be_visitor::visit_union(AST_Union *node)
{
  return 0;
}

int
be_visitor::visit_union_fwd(AST_UnionFwd *node)
{
  return 0;
}

int
be_visitor::visit_union_branch(AST_UnionBranch *node)
{
  return 0;
}

int
be_visitor::visit_union_label(AST_UnionLabel *node)
{
  return 0;
}

int
be_visitor::visit_constant(AST_Constant *node)
{
  return 0;
}

int
be_visitor::visit_array(AST_Array *node)
{
  return 0;
}

int
be_visitor::visit_sequence(AST_Sequence *node)
{
  return 0;
}

int
be_visitor::visit_string(AST_String *node)
{
  return 0;
}

int
be_visitor::visit_typedef(AST_Typedef *node)
{
  return 0;
}

int
be_visitor::visit_root(AST_Root *node)
{
  return 0;
}

int
be_visitor::visit_native(AST_Native *node)
{
  return 0;
}

#ifndef ACE_PRE_5_5
int
be_visitor::visit_valuebox(AST_ValueBox *node)
{
  return 0;
}
#endif /* ACE_PRE_5_5 */
