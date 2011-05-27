/*
 * $Id$
 */

#include "ace/Log_Msg.h"

#include "ace_compat.h"
#include "be_visitor.h"
#include "generator_cpp.h"
#include "generator_erl.h"

using namespace std;

namespace
{
template <typename T>
void
find_children(UTL_Scope* node, vector<T*>& v, AST_Decl::NodeType type)
{
  for (UTL_ScopeActiveIterator it(node, UTL_Scope::IK_decls);
       !it.is_done(); it.next())
  {
    AST_Decl* item = it.item();
    if (item->node_type() == type)
    {
      v.push_back(T::narrow_from_decl(item));
    }
  }
}
} // namespace

be_visitor::be_visitor()
{
  generator_.add(new generator_cpp);
  generator_.add(new generator_erl);
}

be_visitor::~be_visitor()
{
  generator_.delete_all();
}

int
be_visitor::visit_scope(UTL_Scope* node)
{
  for (UTL_ScopeActiveIterator it (node, UTL_Scope::IK_decls);
       !it.is_done(); it.next())
  {
    AST_Decl *item = it.item();
    if (item == 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: visit_scope()")
                        ACE_TEXT(" invalid scope!\n")), -1);
    }

    if (item->imported()) continue; // ignore #include

    if (item->ast_accept(this) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: visit_scope()")
                        ACE_TEXT(" ast_accept failed!\n")), -1);
    }
  }
  return 0;
}

int
be_visitor::visit_root(AST_Root* node)
{
  if (visit_scope(node) != 0)
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_root()")
                      ACE_TEXT(" visit_scope failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_module(AST_Module* node)
{
  if (visit_scope(node) != 0)
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_module()")
                      ACE_TEXT(" visit_scope failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_constant(AST_Constant* node)
{
  if (!generator_.generate_constant(node))
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_constant()")
                      ACE_TEXT(" generate_constant failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_enum_val(AST_EnumVal*)
{
  return 0;
}

int
be_visitor::visit_enum(AST_Enum* node)
{
  vector<AST_EnumVal*> v;
  find_children(node, v, AST_Decl::NT_enum_val);

  if (!generator_.generate_enum(node, v))
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_enum()")
                      ACE_TEXT(" generate_enum failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_structure(AST_Structure* node)
{
  vector<AST_Field*> v;
  find_children(node, v, AST_Decl::NT_field);

  if (!generator_.generate_structure(node, v))
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_structure()")
                      ACE_TEXT(" generate_structure failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_structure_fwd(AST_StructureFwd*)
{
  return 0;
}

int
be_visitor::visit_union_label(AST_UnionLabel*)
{
  return 0;
}

int
be_visitor::visit_union_branch(AST_UnionBranch*)
{
  return 0;
}

int
be_visitor::visit_union(AST_Union* node)
{
  vector<AST_UnionBranch*> v;
  find_children(node, v, AST_Decl::NT_union_branch);

  if (!generator_.generate_union(node, v))
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: visit_union()")
                      ACE_TEXT(" generate_union failed!\n")), -1);
  }
  return 0;
}

int
be_visitor::visit_union_fwd(AST_UnionFwd*)
{
  return 0;
}

//

int
be_visitor::visit_native(AST_Native*)
{
  return 0;
}

int
be_visitor::visit_typedef(AST_Typedef*)
{
  return 0;
}

int
be_visitor::visit_exception(AST_Exception*)
{
  return 0;
}

int
be_visitor::visit_interface(AST_Interface*)
{
  return 0;
}

int
be_visitor::visit_interface_fwd(AST_InterfaceFwd*)
{
  return 0;
}

int
be_visitor::visit_predefined_type(AST_PredefinedType*)
{
  return 0;
}

int
be_visitor::visit_type(AST_Type*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_type()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_valuetype(AST_ValueType*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_valuetype()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_valuetype_fwd(AST_ValueTypeFwd*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_valuetype_fwd()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_component(AST_Component*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_component()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_component_fwd(AST_ComponentFwd*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_component_fwd()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_home(AST_Home*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_home()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_eventtype(AST_EventType*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_eventtype()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_eventtype_fwd(AST_EventTypeFwd*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_eventtype_fwd()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_factory(AST_Factory*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_factory()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_expression(AST_Expression*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_expression()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_operation(AST_Operation*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_operation()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_field(AST_Field*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_field()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_argument(AST_Argument*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_argument()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_attribute(AST_Attribute*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_attribute()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_sequence(AST_Sequence*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_sequence()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_array(AST_Array*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_array()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_string(AST_String*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_string()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

int
be_visitor::visit_decl(AST_Decl*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_decl()")
                    ACE_TEXT(" not implemented!\n")), -1);
}

#ifndef ACE_PRE_5_5
int
be_visitor::visit_valuebox(AST_ValueBox*)
{
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("%N:%l: visit_valuebox()")
                    ACE_TEXT(" not implemented!\n")), -1);
}
#endif /* ACE_PRE_5_5 */
