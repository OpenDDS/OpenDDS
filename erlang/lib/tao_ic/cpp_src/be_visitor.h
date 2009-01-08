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

class be_visitor : public ast_visitor {
public:
  be_visitor(void);
  virtual ~be_visitor(void);

  virtual int visit_decl(AST_Decl *);

  virtual int visit_scope(UTL_Scope *);

  virtual int visit_type(AST_Type *);

  virtual int visit_predefined_type(AST_PredefinedType *);

  virtual int visit_module(AST_Module *);

  virtual int visit_interface(AST_Interface *);

  virtual int visit_interface_fwd(AST_InterfaceFwd *);

  virtual int visit_valuetype(AST_ValueType *);

  virtual int visit_valuetype_fwd(AST_ValueTypeFwd *);

  virtual int visit_component(AST_Component *);

  virtual int visit_component_fwd(AST_ComponentFwd *);

  virtual int visit_home(AST_Home *);

  virtual int visit_eventtype(AST_EventType *);

  virtual int visit_eventtype_fwd(AST_EventTypeFwd *);

  virtual int visit_factory(AST_Factory *);

  virtual int visit_structure(AST_Structure *);

  virtual int visit_structure_fwd(AST_StructureFwd *);

  virtual int visit_exception(AST_Exception *);

  virtual int visit_expression(AST_Expression *);

  virtual int visit_enum(AST_Enum *);

  virtual int visit_enum_val(AST_EnumVal *);

  virtual int visit_operation(AST_Operation *);

  virtual int visit_field(AST_Field *);

  virtual int visit_argument(AST_Argument *);

  virtual int visit_attribute(AST_Attribute *);

  virtual int visit_union(AST_Union *);

  virtual int visit_union_fwd(AST_UnionFwd *);

  virtual int visit_union_branch(AST_UnionBranch *);

  virtual int visit_union_label(AST_UnionLabel *);

  virtual int visit_constant(AST_Constant *);

  virtual int visit_array(AST_Array *);

  virtual int visit_sequence(AST_Sequence *);

  virtual int visit_string(AST_String *);

  virtual int visit_typedef(AST_Typedef *);

  virtual int visit_root(AST_Root *);

  virtual int visit_native(AST_Native *);

#ifndef ACE_PRE_5_5
  virtual int visit_valuebox(AST_ValueBox *);
#endif /* ACE_PRE_5_5 */
};

#endif /* TAO_IC_BE_VISITOR_H */
