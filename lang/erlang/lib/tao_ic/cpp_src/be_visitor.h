/*
 * $Id$
 */

#ifndef BE_VISITOR_H
#define BE_VISITOR_H

#include "ast_argument.h"
#include "ast_array.h"
#include "ast_attribute.h"
#include "ast_component.h"
#include "ast_component_fwd.h"
#include "ast_constant.h"
#include "ast_decl.h"
#include "ast_enum.h"
#include "ast_enum_val.h"
#include "ast_eventtype.h"
#include "ast_eventtype_fwd.h"
#include "ast_exception.h"
#include "ast_expression.h"
#include "ast_factory.h"
#include "ast_field.h"
#include "ast_home.h"
#include "ast_interface.h"
#include "ast_interface_fwd.h"
#include "ast_module.h"
#include "ast_native.h"
#include "ast_operation.h"
#include "ast_predefined_type.h"
#include "ast_root.h"
#include "ast_sequence.h"
#include "ast_string.h"
#include "ast_type.h"
#include "ast_typedef.h"
#include "ast_structure.h"
#include "ast_structure_fwd.h"
#include "ast_union.h"
#include "ast_union_branch.h"
#include "ast_union_fwd.h"
#include "ast_union_label.h"
#include "ast_valuetype.h"
#include "ast_valuetype_fwd.h"
#include "ast_visitor.h"
#include "utl_scope.h"

#ifndef ACE_PRE_5_5
# include "ast_valuebox.h"
#endif /* ACE_PRE_5_5 */

#include "ace_compat.h"
#include "generator.h"

class be_visitor : public ast_visitor
{
public:
  be_visitor();

  ~be_visitor();

  int visit_decl(AST_Decl* node);

  int visit_scope(UTL_Scope* node);

  int visit_type(AST_Type* node);

  int visit_predefined_type(AST_PredefinedType* node);

  int visit_module(AST_Module* node);

  int visit_interface(AST_Interface* node);

  int visit_interface_fwd(AST_InterfaceFwd* node);

  int visit_valuetype(AST_ValueType* node);

  int visit_valuetype_fwd(AST_ValueTypeFwd* node);

  int visit_component(AST_Component* node);

  int visit_component_fwd(AST_ComponentFwd* node);

  int visit_home(AST_Home* node);

  int visit_eventtype(AST_EventType* node);

  int visit_eventtype_fwd(AST_EventTypeFwd* node);

  int visit_factory(AST_Factory* node);

  int visit_structure(AST_Structure* node);

  int visit_structure_fwd(AST_StructureFwd* node);

  int visit_exception(AST_Exception* node);

  int visit_expression(AST_Expression* node);

  int visit_enum(AST_Enum* node);

  int visit_enum_val(AST_EnumVal* node);

  int visit_operation(AST_Operation* node);

  int visit_field(AST_Field* node);

  int visit_argument(AST_Argument* node);

  int visit_attribute(AST_Attribute* node);

  int visit_union(AST_Union* node);

  int visit_union_fwd(AST_UnionFwd* node);

  int visit_union_branch(AST_UnionBranch* node);

  int visit_union_label(AST_UnionLabel* node);

  int visit_constant(AST_Constant* node);

  int visit_array(AST_Array* node);

  int visit_sequence(AST_Sequence* node);

  int visit_string(AST_String* node);

  int visit_typedef(AST_Typedef* node);

  int visit_root(AST_Root* node);

  int visit_native(AST_Native* node);

#ifndef ACE_PRE_5_5
  int visit_valuebox(AST_ValueBox* node);
#endif /* ACE_PRE_5_5 */

private:
  generator_composite generator_;
};

#endif /* BE_VISITOR_H */
