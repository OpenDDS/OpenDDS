/*
 * $Id$
 */

#ifndef TAO_IC_BE_VISITOR_H
#define TAO_IC_BE_VISITOR_H

#include "ast_argument.h"
#include "ast_array.h"
#include "ast_attribute.h"
#include "ace_compat.h"
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

#ifndef ACE_PRE_5_5
# include "ast_valuebox.h"
#endif /* ACE_PRE_5_5 */

#include "ast_valuetype.h"
#include "ast_valuetype_fwd.h"
#include "ast_visitor.h"
#include "utl_scope.h"

#include "generator.h"

class be_visitor : public ast_visitor {
public:
  be_visitor(void);
  ~be_visitor(void);

  int visit_decl(AST_Decl *);

  int visit_scope(UTL_Scope *);

  int visit_type(AST_Type *);

  int visit_predefined_type(AST_PredefinedType *);

  int visit_module(AST_Module *);

  int visit_interface(AST_Interface *);

  int visit_interface_fwd(AST_InterfaceFwd *);

  int visit_valuetype(AST_ValueType *);

  int visit_valuetype_fwd(AST_ValueTypeFwd *);

  int visit_component(AST_Component *);

  int visit_component_fwd(AST_ComponentFwd *);

  int visit_home(AST_Home *);

  int visit_eventtype(AST_EventType *);

  int visit_eventtype_fwd(AST_EventTypeFwd *);

  int visit_factory(AST_Factory *);

  int visit_structure(AST_Structure *);

  int visit_structure_fwd(AST_StructureFwd *);

  int visit_exception(AST_Exception *);

  int visit_expression(AST_Expression *);

  int visit_enum(AST_Enum *);

  int visit_enum_val(AST_EnumVal *);

  int visit_operation(AST_Operation *);

  int visit_field(AST_Field *);

  int visit_argument(AST_Argument *);

  int visit_attribute(AST_Attribute *);

  int visit_union(AST_Union *);

  int visit_union_fwd(AST_UnionFwd *);

  int visit_union_branch(AST_UnionBranch *);

  int visit_union_label(AST_UnionLabel *);

  int visit_constant(AST_Constant *);

  int visit_array(AST_Array *);

  int visit_sequence(AST_Sequence *);

  int visit_string(AST_String *);

  int visit_typedef(AST_Typedef *);

  int visit_root(AST_Root *);

  int visit_native(AST_Native *);

#ifndef ACE_PRE_5_5
  int visit_valuebox(AST_ValueBox *);
#endif /* ACE_PRE_5_5 */

private:
  generator_composite generator_;
};

#endif /* TAO_IC_BE_VISITOR_H */
