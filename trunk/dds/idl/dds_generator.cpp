/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds_generator.h"

#include "utl_identifier.h"

using namespace std;

dds_generator::~dds_generator() {}

string dds_generator::scoped_helper(UTL_ScopedName* sn, const char* sep)
{
  string sname;

  for (; sn; sn = static_cast<UTL_ScopedName*>(sn->tail())) {
    if (sn->head()->escaped())
      sname += "_";

    sname += sn->head()->get_string();

    if (sname != "" && sn->tail())
      sname += sep;
  }

  return sname;
}

bool composite_generator::gen_const(UTL_ScopedName* name, bool nestedInInteface,
  AST_Expression::ExprType type, AST_Expression::AST_ExprValue* value)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_const(name, nestedInInteface, type, value))
      return false;
  }

  return true;
}

bool composite_generator::gen_enum(UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_enum(name, contents, repoid))
      return false;
  }

  return true;
}

bool composite_generator::gen_struct(UTL_ScopedName* name,
  const vector<AST_Field*>& fields, const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_struct(name, fields, repoid))
      return false;
  }

  return true;
}

bool composite_generator::gen_typedef(UTL_ScopedName* name, AST_Type* base,
                                      const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_typedef(name, base, repoid))
      return false;
  }

  return true;
}

bool composite_generator::gen_interf(UTL_ScopedName* name, bool local,
  const std::vector<AST_Interface*>& inherits,
  const std::vector<AST_Interface*>& inh_flat,
  const std::vector<AST_Attribute*>& attrs,
  const std::vector<AST_Operation*>& ops, const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_interf(name, local, inherits, inh_flat,
                           attrs, ops, repoid))
      return false;
  }

  return true;
}

bool composite_generator::gen_interf_fwd(UTL_ScopedName* name)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_interf_fwd(name))
      return false;
  }

  return true;
}

bool composite_generator::gen_native(UTL_ScopedName* name, const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_native(name, repoid))
      return false;
  }

  return true;
}

bool composite_generator::gen_union(UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
  const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_union(name, branches, discriminator, repoid))
      return false;
  }

  return true;
}
