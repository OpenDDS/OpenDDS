/*
 * $Id$
 */

#include "ace/Log_Msg.h"

#include "ace_compat.h"
#include "be_visitor.h"
#include "generator_cpp.h"
#include "generator_erl.h"

using namespace std;

namespace {
template <typename T>
void find_children(UTL_Scope *node,
                   vector<T *> &nodes,
                   AST_Decl::NodeType node_type)
{
  for (UTL_ScopeActiveIterator it(node, UTL_Scope::IK_decls);
       !it.is_done(); it.next()) {
  
    AST_Decl *item = it.item();
  
    if (item->node_type() == node_type) {
      nodes.push_back(T::narrow_from_decl(item));
    }
  }
}
} // namespace

be_visitor::be_visitor()
  : generator_(true) // auto_delete
{
  generator_.add(new generator_cpp);
  generator_.add(new generator_erl);
}

be_visitor::~be_visitor()
{
}

int
be_visitor::visit_root(AST_Root *node)
{
  if (visit_scope(node) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_root()")
                      ACE_TEXT(" visit_scope failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_scope(UTL_Scope *node)
{
  for (UTL_ScopeActiveIterator it (node, UTL_Scope::IK_decls);
       !it.is_done(); it.next()) {
    
    AST_Decl *item = it.item();

    if (item == 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: visit_scope()")
                        ACE_TEXT(" invalid scope!\n")), -1);
    }

    if (item->ast_accept(this) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: visit_scope()")
                        ACE_TEXT(" ast_accept failed!\n")), -1);
    }
  }
  
  return 0;
}

int
be_visitor::visit_module(AST_Module *node)
{
  if (visit_scope(node) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_module()")
                      ACE_TEXT(" visit_scope failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_constant(AST_Constant *node)
{
  if (!generator_.generate_constant(node)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_constant()")
                      ACE_TEXT(" generate_constant failed!\n")), -1);
  }  
  return 0;
}

int
be_visitor::visit_enum(AST_Enum *node)
{
  vector<AST_EnumVal *> values;
  find_children(node, values, AST_Decl::NT_enum_val);

  if (!generator_.generate_enum(node, values)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_enum()")
                      ACE_TEXT(" generate_enum failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_enum_val(AST_EnumVal *node)
{
  return 0;
}

int
be_visitor::visit_exception(AST_Exception *node)
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
be_visitor::visit_native(AST_Native *node)
{
  return 0;
}

int
be_visitor::visit_typedef(AST_Typedef *node)
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
be_visitor::visit_type(AST_Type *)
{
  return 0; // not implemented
}

int
be_visitor::visit_predefined_type(AST_PredefinedType *)
{
  return 0; // not implemented
}

int
be_visitor::visit_valuetype(AST_ValueType *)
{
  return 0; // not implemented
}

int
be_visitor::visit_valuetype_fwd(AST_ValueTypeFwd *)
{
  return 0; // not implemented
}

int
be_visitor::visit_component(AST_Component *)
{
  return 0; // not implemented
}

int
be_visitor::visit_component_fwd(AST_ComponentFwd *)
{
  return 0; // not implemented
}

int
be_visitor::visit_home(AST_Home *)
{
  return 0; // not implemented
}

int
be_visitor::visit_eventtype(AST_EventType *)
{
  return 0; // not implemented
}

int
be_visitor::visit_eventtype_fwd(AST_EventTypeFwd *)
{
  return 0; // not implemented
}

int
be_visitor::visit_factory(AST_Factory *)
{
  return 0; // not implemented
}

int
be_visitor::visit_expression(AST_Expression *)
{
  return 0; // not implemented
}

int
be_visitor::visit_operation(AST_Operation *)
{
  return 0; // not implemented
}

int
be_visitor::visit_field(AST_Field *)
{
  return 0; // not implemented
}

int
be_visitor::visit_argument(AST_Argument *)
{
  return 0; // not implemented
}

int
be_visitor::visit_attribute(AST_Attribute *)
{
  return 0; // not implemented
}

int
be_visitor::visit_sequence(AST_Sequence *)
{
  return 0; // not implemented
}

int
be_visitor::visit_array(AST_Array *)
{
  return 0; // not implemented
}

int
be_visitor::visit_string(AST_String *)
{
  return 0; // not implemented
}

int
be_visitor::visit_decl(AST_Decl *)
{
  return 0; // not implemented
}

#ifndef ACE_PRE_5_5
int
be_visitor::visit_valuebox(AST_ValueBox *)
{
  return 0; // not implemented
}
#endif /* ACE_PRE_5_5 */
