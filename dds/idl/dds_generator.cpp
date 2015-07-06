/*
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

string dds_generator::module_scope_helper(UTL_ScopedName* sn, const char* sep)
{
  string sname;

  for (; sn; sn = static_cast<UTL_ScopedName*>(sn->tail())) {
    if (sn->tail() != 0) {
      if (sn->head()->escaped())
        sname += "_";

      sname += sn->head()->get_string();

      if (sname != "" && sn->tail())
        sname += sep;
    }
  }

  return sname;
}

void composite_generator::gen_prologue()
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    (*it)->gen_prologue();
  }
}

void composite_generator::gen_epilogue()
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    (*it)->gen_epilogue();
  }
}

bool composite_generator::gen_const(UTL_ScopedName* name, bool nestedInInteface,
  AST_Constant* constant)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!constant->imported() || (*it)->do_included_files())
      if (!(*it)->gen_const(name, nestedInInteface, constant))
        return false;
  }

  return true;
}

bool composite_generator::gen_enum(AST_Enum* node, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!node->imported() || (*it)->do_included_files())
      if (!(*it)->gen_enum(node, name, contents, repoid))
        return false;
  }

  return true;
}

bool composite_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const vector<AST_Field*>& fields, AST_Type::SIZE_TYPE size,
  const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!node->imported() || (*it)->do_included_files())
      if (!(*it)->gen_struct(node, name, fields, size, repoid))
        return false;
  }

  return true;
}

bool composite_generator::gen_struct_fwd(UTL_ScopedName* name,
  AST_Type::SIZE_TYPE size)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_struct_fwd(name, size))
      return false;
  }

  return true;
}

bool composite_generator::gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* base,
                                      const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!node->imported() || (*it)->do_included_files())
      if (!(*it)->gen_typedef(node, name, base, repoid))
        return false;
  }

  return true;
}

bool composite_generator::gen_interf(AST_Interface* node, UTL_ScopedName* name, bool local,
  const std::vector<AST_Interface*>& inherits,
  const std::vector<AST_Interface*>& inh_flat,
  const std::vector<AST_Attribute*>& attrs,
  const std::vector<AST_Operation*>& ops, const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!node->imported() || (*it)->do_included_files())
      if (!(*it)->gen_interf(node, name, local, inherits, inh_flat,
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

bool composite_generator::gen_native(AST_Native* node, UTL_ScopedName* name, const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!node->imported() || (*it)->do_included_files())
      if (!(*it)->gen_native(node, name, repoid))
        return false;
  }

  return true;
}

bool composite_generator::gen_union(AST_Union* node,
                                    UTL_ScopedName* name,
                                    const std::vector<AST_UnionBranch*>& branches,
                                    AST_Type* discriminator,
                                    const char* repoid)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!node->imported() || (*it)->do_included_files())
      if (!(*it)->gen_union(node, name, branches, discriminator, repoid))
        return false;
  }

  return true;
}

bool composite_generator::gen_union_fwd(AST_UnionFwd* uf, UTL_ScopedName* name,
  AST_Type::SIZE_TYPE size)
{
  for (vector<dds_generator*>::iterator it(components_.begin());
       it != components_.end(); ++it) {
    if (!(*it)->gen_union_fwd(uf, name, size))
      return false;
  }

  return true;
}
