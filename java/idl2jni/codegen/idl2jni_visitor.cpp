/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ast_argument.h"
#include "ast_attribute.h"
#include "ast_component_fwd.h"
#include "ast_enum.h"
#include "ast_enum_val.h"
#include "ast_eventtype.h"
#include "ast_eventtype_fwd.h"
#include "ast_exception.h"
#include "ast_factory.h"
#include "ast_home.h"
#include "ast_interface.h"
#include "ast_module.h"
#include "ast_native.h"
#include "ast_operation.h"
#include "ast_predefined_type.h"
#include "ast_root.h"
#include "ast_sequence.h"
#include "ast_structure.h"
#include "ast_union.h"
#include "ast_valuetype.h"
#include "ast_valuetype_fwd.h"
#include "utl_identifier.h"
#include "utl_string.h"
#include "utl_exceptlist.h"
#include "utl_err.h"
#include "nr_extern.h"

#include "idl2jni_visitor.h"
#include "idl_mapping.h"

#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

namespace {

idl_mapping_java java_;
idl_mapping_jni jni_;
idl_mapping *mappings_[] = {&java_, &jni_};
const size_t N_MAP = sizeof(mappings_) / sizeof(mappings_[0]);

composite_mapping map_target_(&mappings_[0], &mappings_[N_MAP]);

template <typename T>
void scope2vector(vector<T *> &v, UTL_Scope *s, AST_Decl::NodeType nt)
{
  UTL_ScopeActiveIterator it(s, UTL_Scope::IK_decls);

  for (; !it.is_done(); it.next()) {
    AST_Decl *item = it.item();

    if (item->node_type() == nt) {
      v.push_back(T::narrow_from_decl(item));
    }
  }
}

} // namespace

idl2jni_visitor::idl2jni_visitor(AST_Decl *scope,
                                 CORBA::Boolean in_reopened)
  : scope_(scope),
    in_reopened_(in_reopened)
{
}

idl2jni_visitor::~idl2jni_visitor()
{
}

//Helper functions

void idl2jni_visitor::addToToplevels(AST_Decl *node)
{
  toplevels_seen_.insert(node->repoID());
}

//visit_* functions

int
idl2jni_visitor::visit_root(AST_Root *node)
{
  error_ = false;
  toplevels_seen_.clear();

  if (this->visit_scope(node) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%N:%l) idl2jni_visitor::visit_root -")
                      ACE_TEXT(" visit_scope failed\n")), -1);
  }

  return (error_) ? -1 : 0;
}

int
idl2jni_visitor::visit_scope(UTL_Scope *node)
{
  if (node->nmembers() > 0) {
    UTL_ScopeActiveIterator si(node, UTL_Scope::IK_decls);
    AST_Decl *d = 0;

    while (!si.is_done()) {
      d = si.item();

      if (d == 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%N:%l) idl2jni_visitor::visit_")
                          ACE_TEXT("scope - bad node in this scope\n")), -1);
      }

      if (d->node_type() == AST_Decl::NT_pre_defined) {
        si.next();
        continue;
      }

      if (d->ast_accept(this) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%N:%l) idl2jni_visitor::visit_")
                          ACE_TEXT("scope - failed to accept visitor\n")), -1);
      }

      si.next();
    }
  }

  return 0;
}

int
idl2jni_visitor::visit_module(AST_Module *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("MODULE", name);

  ACE_UNUSED_ARG(g);

  if (this->visit_scope(node) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%N:%l) idl2jni_visitor::visit_module -")
                      ACE_TEXT(" visit_scope failed\n")), -1);
  }

  return 0;
}

int
idl2jni_visitor::visit_interface(AST_Interface *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("INTERFACE", name);

  ACE_UNUSED_ARG(g);

  addToToplevels(node);

  // The type returned by AST_Interface::inherits() changed
  // to a AST_Type** in TAO 1.7.6.  This requires an explicit
  // dynamic cast for each element.
  //vector<AST_Interface *> inherits(node->inherits(),
  //                                 node->inherits() + node->n_inherits());
  vector<AST_Interface *> inherits(node->n_inherits());
  for (int i = 0; i < node->n_inherits(); i++) {
    inherits[i] = dynamic_cast<AST_Interface *>(*(node->inherits()+i));
  }

  vector<AST_Interface *> inherits_flat(node->inherits_flat(),
                                        node->inherits_flat() + node->n_inherits_flat());

  vector<AST_Attribute *> attrs;

  scope2vector(attrs, node, AST_Decl::NT_attr);

  vector<AST_Operation *> ops;

  scope2vector(ops, node, AST_Decl::NT_op);

  error_ |= !map_target_.gen_interf(node->name(), node->is_local(),
                                    inherits, inherits_flat, attrs, ops, node->repoID());

  if (this->visit_scope(node) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%N:%l) idl2jni_visitor::visit_interface ")
                      ACE_TEXT("- visit_scope failed\n")), -1);
  }

  return 0;
}

int
idl2jni_visitor::visit_structure(AST_Structure *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("STRUCT", name);

  ACE_UNUSED_ARG(g);

  addToToplevels(node);

  vector<AST_Field *> fields;

  size_t nfields = node->nfields();

  fields.reserve(nfields);

  for (ACE_CDR::ULong i = 0; i < nfields; ++i) {
    AST_Field **f;
    node->field(f, i);
    fields.push_back(*f);
  }

  error_ |= !map_target_.gen_struct(node->name(), fields, node->repoID());
  return 0;
}

int
idl2jni_visitor::visit_exception(AST_Exception *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("EXCEPTION", name);

  ACE_UNUSED_ARG(g);

  addToToplevels(node);

  cerr << "WARNING: Exceptions are not currently supported.\n";

  return 0;
}

int
idl2jni_visitor::visit_typedef(AST_Typedef *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("TYPEDEF", name);

  ACE_UNUSED_ARG(g);

  addToToplevels(node);

  error_ |= !map_target_.gen_typedef(node->name(), node->base_type(),
                                     node->repoID());

  return 0;
}

int
idl2jni_visitor::visit_enum(AST_Enum *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("ENUM", name);

  ACE_UNUSED_ARG(g);

  addToToplevels(node);

  vector<AST_EnumVal *> contents;

  scope2vector(contents, node, AST_Decl::NT_enum_val);

  error_ |= !map_target_.gen_enum(node->name(), contents, node->repoID());

  return 0;
}

int
idl2jni_visitor::visit_interface_fwd(AST_InterfaceFwd *node)
{
  const char *name = node->local_name()->get_string();
  BE_Comment_Guard g("INTERFACE-FWD", name);
  ACE_UNUSED_ARG(g);

  error_ |= !map_target_.gen_interf_fwd(node->name());
  return 0;
}

int
idl2jni_visitor::visit_constant(AST_Constant *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("CONST", name);

  ACE_UNUSED_ARG(g);

  addToToplevels(node);

  AST_Decl *d = ScopeAsDecl(node->defined_in());

  bool nested = d && (d->node_type() == AST_Decl::NT_interface);

  error_ |= !map_target_.gen_const(node->name(), nested, node->et(),
                                   node->constant_value()->ev());

  return 0;
}

int
idl2jni_visitor::visit_native(AST_Native *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("NATIVE", name);

  ACE_UNUSED_ARG(g);

  error_ |= !map_target_.gen_native(node->name(), node->repoID());

  return 0;
}

int
idl2jni_visitor::visit_union(AST_Union *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  const char *name = node->local_name()->get_string();

  BE_Comment_Guard g("UNION", name);

  ACE_UNUSED_ARG(g);

  addToToplevels(node);

  vector<AST_UnionBranch *> branches;

  size_t nfields = node->nfields();

  branches.reserve(nfields);

  for (ACE_CDR::ULong i = 0; i < nfields; ++i) {
    AST_Field **f;
    node->field(f, i);
    AST_UnionBranch *ub = AST_UnionBranch::narrow_from_decl(*f);

    if (!ub) {
      std::cerr << "ERROR - expected union to contain UnionBranches\n";
      error_ = true;
      return 0;
    }

    branches.push_back(ub);
  }

  AST_Union::DefaultValue defval;
  node->default_value(defval);

  error_ |= !map_target_.gen_union(node->name(), branches, node->disc_type(),
                                   node->udisc_type(), defval, node->repoID());
  return 0;
}

// *** All methods below here are unimplemented (or trivially implemented) ***

int
idl2jni_visitor::visit_sequence(AST_Sequence *)
{
  //sequences always appear as typedefs, see visit_typedef ()
  return 0;
}

int
idl2jni_visitor::visit_operation(AST_Operation *)
{
  // operations are taken care of by visit_interface()
  return 0;
}

int
idl2jni_visitor::visit_field(AST_Field *)
{
  // fields are taken care of by visit_interface() for arguments and attributes
  return 0;
}

int
idl2jni_visitor::visit_attribute(AST_Attribute *)
{
  // attributes are taken care of by visit_interface ()
  return 0;
}

int
idl2jni_visitor::visit_array(AST_Array *)
{
  //arrays always appear as typedefs, see visit_typedef ()
  return 0;
}

//begin IDL syntactic elements that are not currently supported

int
idl2jni_visitor::visit_valuetype(AST_ValueType *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  cout << "ERROR: valuetypes not currently supported" << endl;

  return -1;
}

int
idl2jni_visitor::visit_valuetype_fwd(AST_ValueTypeFwd *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  cout << "ERROR: valuetypes not currently supported" << endl;

  return -1;
}

int
idl2jni_visitor::visit_component(AST_Component *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  cout << "ERROR: components not currently supported" << endl;

  return -1;
}

int
idl2jni_visitor::visit_component_fwd(AST_ComponentFwd *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  cout << "ERROR: components not currently supported" << endl;

  return -1;
}

int
idl2jni_visitor::visit_eventtype(AST_EventType *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  cout << "ERROR: components not currently supported" << endl;

  return -1;
}

int
idl2jni_visitor::visit_eventtype_fwd(AST_EventTypeFwd *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  cout << "ERROR: components not currently supported" << endl;

  return -1;
}

int
idl2jni_visitor::visit_home(AST_Home *node)
{
  if (node->imported() && !be_global->do_included_files()) {
    return 0;
  }

  cout << "ERROR: homes not currently supported" << endl;

  return -1;
}

int
idl2jni_visitor::visit_factory(AST_Factory *)
{
  cout << "ERROR: factories not currently supported" << endl;
  return -1;
}

//no need to implement these at this level

int
idl2jni_visitor::visit_predefined_type(AST_PredefinedType *)
{
  return 0;
}

int
idl2jni_visitor::visit_string(AST_String *)
{
  return 0;
}

int idl2jni_visitor::visit_union_fwd(AST_UnionFwd *)
{
  return 0;
}

int idl2jni_visitor::visit_union_branch(AST_UnionBranch *)
{
  return 0;
}

int idl2jni_visitor::visit_union_label(AST_UnionLabel *)
{
  return 0;
}

int idl2jni_visitor::visit_enum_val(AST_EnumVal *)
{
  return 0;
}

int idl2jni_visitor::visit_expression(AST_Expression *)
{
  return 0;
}

int idl2jni_visitor::visit_structure_fwd(AST_StructureFwd *)
{
  return 0;
}

int idl2jni_visitor::visit_type(AST_Type *)
{
  return 0;
}

int idl2jni_visitor::visit_argument(AST_Argument *)
{
  return 0;
}

int idl2jni_visitor::visit_decl(AST_Decl *)
{
  return 0;
}

int idl2jni_visitor::visit_valuebox(AST_ValueBox *)
{
  return 0;
}

int
idl2jni_visitor::visit_template_module (AST_Template_Module *)
{
  return 0;
}

int
idl2jni_visitor::visit_template_module_inst (AST_Template_Module_Inst *)
{
  return 0;
}

int
idl2jni_visitor::visit_template_module_ref (AST_Template_Module_Ref *)
{
  return 0;
}

int
idl2jni_visitor::visit_param_holder(AST_Param_Holder *)
{
  return 0;
}

int idl2jni_visitor::visit_porttype(AST_PortType *)
{
  return 0;
}

int idl2jni_visitor::visit_provides(AST_Provides *)
{
  return 0;
}

int idl2jni_visitor::visit_uses(AST_Uses *)
{
  return 0;
}

int idl2jni_visitor::visit_publishes(AST_Publishes *)
{
  return 0;
}

int idl2jni_visitor::visit_emits(AST_Emits *)
{
  return 0;
}

int idl2jni_visitor::visit_consumes(AST_Consumes *)
{
  return 0;
}

int idl2jni_visitor::visit_extended_port(AST_Extended_Port *)
{
  return 0;
}

int idl2jni_visitor::visit_mirror_port(AST_Mirror_Port *)
{
  return 0;
}

int idl2jni_visitor::visit_connector(AST_Connector *)
{
  return 0;
}

int idl2jni_visitor::visit_finder(AST_Finder *)
{
  return 0;
}
