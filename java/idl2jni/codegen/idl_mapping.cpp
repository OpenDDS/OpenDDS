/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "idl_mapping.h"

#include <ast_annotation_member.h>
#include <utl_identifier.h>
#include <utl_string.h>

using namespace std;

idl_mapping::~idl_mapping() {}

string idl_mapping::scoped_helper(UTL_ScopedName *sn, const char *sep,
                                  bool omit_local)
{
  string sname;

  for (; sn; sn = static_cast<UTL_ScopedName *>(sn->tail())) {
    if (omit_local && !sn->tail())
      break;

    if (sname != "")
      sname += sep;

    if (sn->head()->escaped())
      sname += "_";

    sname += sn->head()->get_string();
  }

  return sname;
}

bool composite_mapping::gen_const(UTL_ScopedName *name, bool nestedInInteface,
                                  AST_Expression::ExprType type, AST_Expression::AST_ExprValue *value)
{
  for (vector<idl_mapping *>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_const(name, nestedInInteface, type, value))
      return false;
  }

  return true;
}

bool composite_mapping::gen_enum(UTL_ScopedName *name,
                                 const std::vector<AST_EnumVal *> &contents, const char *repoid)
{
  for (vector<idl_mapping *>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_enum(name, contents, repoid))
      return false;
  }

  return true;
}

bool composite_mapping::gen_struct(UTL_ScopedName *name,
                                   const vector<AST_Field *> &fields, const char *repoid)
{
  for (vector<idl_mapping *>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_struct(name, fields, repoid))
      return false;
  }

  return true;
}

bool composite_mapping::gen_typedef(UTL_ScopedName *name, AST_Type *base,
                                    const char *repoid)
{
  for (vector<idl_mapping *>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_typedef(name, base, repoid))
      return false;
  }

  return true;
}

bool composite_mapping::gen_interf(UTL_ScopedName *name, bool local,
                                   const std::vector<AST_Interface *> &inherits,
                                   const std::vector<AST_Interface *> &inh_flat,
                                   const std::vector<AST_Attribute *> &attrs,
                                   const std::vector<AST_Operation *> &ops, const char *repoid)
{
  for (vector<idl_mapping *>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_interf(name, local, inherits, inh_flat,
                           attrs, ops, repoid))
      return false;
  }

  return true;
}

bool composite_mapping::gen_interf_fwd(UTL_ScopedName *name)
{
  for (vector<idl_mapping *>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_interf_fwd(name))
      return false;
  }

  return true;
}

bool composite_mapping::gen_native(UTL_ScopedName *name, const char *repoid)
{
  for (vector<idl_mapping *>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_native(name, repoid))
      return false;
  }

  return true;
}

bool composite_mapping::gen_union(UTL_ScopedName *name,
                                  const std::vector<AST_UnionBranch *> &branches, AST_Type *discriminator,
                                  AST_Expression::ExprType udisc_type,
                                  const AST_Union::DefaultValue &default_value, const char *repoid)
{
  for (vector<idl_mapping *>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_union(name, branches, discriminator, udisc_type,
                          default_value, repoid))
      return false;
  }

  return true;
}

bool is_hidden_op_in_java(AST_Operation* op, std::string* impl)
{
  AST_Annotation_Appl* a = op->annotations().find("::OpenDDS::internal::@hidden_op_in_java");
  if (a && impl) {
    *impl = dynamic_cast<AST_Annotation_Member*>((*a)["impl"])->
      value()->ev()->u.strval->get_string();
  }
  return a;
}
