/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds_generator.h"

#include "utl_identifier.h"

using namespace std;
using namespace AstTypeClassification;

dds_generator::~dds_generator() {}

string dds_generator::to_string(Identifier* id)
{
  const string str = id->get_string();
  return id->escaped() ? '_' + str : str;
}

string dds_generator::scoped_helper(UTL_ScopedName* sn, const char* sep)
{
  string sname;

  for (; sn; sn = static_cast<UTL_ScopedName*>(sn->tail())) {
    sname += to_string(sn->head());

    if (sname != "" && sn->tail()) {
      sname += sep;
    }
  }

  return sname;
}

string dds_generator::module_scope_helper(UTL_ScopedName* sn, const char* sep)
{
  string sname;

  for (; sn; sn = static_cast<UTL_ScopedName*>(sn->tail())) {
    if (sn->tail() != 0) {
      sname += to_string(sn->head());

      if (sname != "" && sn->tail()) {
        sname += sep;
      }
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

NestedForLoops::NestedForLoops(const char* type, const char* prefix,
                               AST_Array* arr, std::string& indent,
                               bool followTypedefs)
  : n_(arr->n_dims())
  , indent_(indent)
{
  std::ostringstream index_oss;
  size_t i = 0, j = 0;
  while (true) {
    for (; i < n_; ++i) {
      be_global->impl_ <<
        indent << "for (" << type << ' ' << prefix << i << " = 0; " <<
        prefix << i << " < " << arr->dims()[i - j]->ev()->u.ulval << "; ++" <<
        prefix << i << ") {\n";
      indent += "  ";
      index_oss << "[" << prefix << i << "]";
    }
    if (!followTypedefs) {
      break;
    }
    AST_Type* const base =
      AstTypeClassification::resolveActualType(arr->base_type());
    if (base->node_type() == AST_Decl::NT_array) {
      arr = dynamic_cast<AST_Array*>(base);
      n_ += arr->n_dims();
      j = i;
    } else {
      break;
    }
  }
  index_ = index_oss.str();
}

NestedForLoops::~NestedForLoops()
{
  for (size_t i = 0; i < n_; ++i) {
    indent_.resize(indent_.size() - 2);
    be_global->impl_ << indent_ << "}\n";
  }
}

string type_to_default(AST_Type* type, const string& name, bool is_anonymous, bool is_union)
{
  string val;
  AST_Type* actual_type = resolveActualType(type);
  Classification fld_cls = classify(actual_type);
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  if ((fld_cls & CL_STRUCTURE) || (fld_cls & CL_UNION)) {
    if (is_union) {
    val = "set_default(" + name + "());\n";
    } else {
    val = "set_default(" + name + ");\n";
    }
  } else if (fld_cls & CL_ARRAY) {
    string temp = name;
    if (temp.size() > 2 && temp.substr(temp.size() - 2, 2) == "()") {
      temp.erase(temp.size() - 2);
    }
    temp += "_temp";
    replace(temp.begin(), temp.end(), '.', '_');
    replace(temp.begin(), temp.end(), '[', '_');
    replace(temp.begin(), temp.end(), ']', '_');
    if (use_cxx11) {
      string n = scoped(type->name());
      if (is_anonymous) {
        n = n.substr(0, n.rfind("::") + 2) + "AnonymousType_" + type->local_name()->get_string();
        n = (fld_cls == AST_Decl::NT_sequence) ? (n + "_seq") : n;
      }
      string v = n;
      size_t index = 0;
      while (true) {
        index = v.find("::", index);
        if (index == string::npos) break;
        v.replace(index, 2, "_");
        index += 1;
      }
      string pre;
      if (is_union) {
        pre = "IDL::DistinctType<" + n + ", " + v + "_tag>(tmp)";
      } else {
        pre = "IDL::DistinctType<" + n + ", " + v + "_tag>(" + name + ")";
      }
      val += "      set_default(" + pre + ");\n";
    } else {
      string n = scoped(type->name());
      if (is_anonymous) {
        n = n.substr(0, n.rfind("::") + 2) + "_" + type->local_name()->get_string();
        n = (fld_cls == AST_Decl::NT_sequence) ? (n + "_seq") : n;
      }
      if (is_union) {
        val = n + "_forany " + temp + "(const_cast<"
          + n + "_slice*>(tmp));\n";
      } else {
        val = n + "_forany " + temp + "(const_cast<"
          + n + "_slice*>(" + name + "));\n";
      }
      val += "      set_default(" + temp + ");\n";
      if (is_union) {
        val += name + "(tmp);\n";
      }
    }
  } else if (fld_cls & CL_ENUM) {
    // For now, simply return the first value of the enumeration.
    // Must be changed, if support for @default_literal is desired.
    AST_Enum* enu = dynamic_cast<AST_Enum*>(actual_type);
    UTL_ScopeActiveIterator i(enu, UTL_Scope::IK_decls);
    AST_EnumVal *item = dynamic_cast<AST_EnumVal*>(i.item());
    string enum_val = item->name()->get_string_copy();
    if (use_cxx11) {
      enum_val = scoped(type->name()) + "::" + item->local_name()->get_string();
    }
    if (is_union) {
      val = name + "(" + enum_val + ");\n";
    } else {
      val = name + " = " + enum_val + ";\n";
    }
  } else if (fld_cls & CL_SEQUENCE) {
    string seq_resize_func = (use_cxx11) ? "resize" : "length";
    if (is_union) {
      val = "tmp." + seq_resize_func + "(0);\n";
      val += name + "(tmp);\n";
    } else {
      val = name + "." + seq_resize_func + "(0);\n";
    }
  } else if (fld_cls & CL_STRING) {
    string def_val = (fld_cls & CL_WIDE) ? "L\"\"" : "\"\"";
    if (!use_cxx11 && (fld_cls & CL_WIDE)) def_val = "TAO::WString_Manager::s_traits::default_initializer()";
    if (is_union) {
      val = name + "(" + def_val + ");\n";
    } else {
      val = name + " = " + def_val + ";\n";
    }
  } else if ((fld_cls & CL_PRIMITIVE) || (fld_cls & CL_FIXED)) {
    AST_PredefinedType* pt = dynamic_cast<AST_PredefinedType*>(actual_type);
    if (pt && (pt->pt() == AST_PredefinedType::PT_longdouble)) {
      if (use_cxx11) {
        val = name + " = 0.0L;\n";
      } else {
        val = name + " = ACE_CDR_LONG_DOUBLE_INITIALIZER;\n";
      }
    } else {
      val = name + " = 0;\n";
    }
  } else {
    // TODO: Remove
    abort();
  }
  return val;
}
