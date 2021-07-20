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

const std::string cxx_escape = "_cxx_";

bool dds_generator::cxx_escaped(const std::string& s)
{
  return s.substr(0, cxx_escape.size()) == cxx_escape;
}

std::string dds_generator::valid_var_name(const std::string& str)
{
  // Replace invalid characters with a single underscore
  // Replace existing "_" with "_9" to prevent collision
  const std::string invalid_chars("<>()[]*.: ");
  std::string s;
  char last_char = '\0';
  for (size_t i = 0; i < str.size(); ++i) {
    const char c = str[i];
    if (invalid_chars.find(c) == std::string::npos) {
      s.push_back(c);
      last_char = c;
      if (c == '_') {
        s.push_back('9');
        last_char = '9';
      }
    } else if (last_char != '_') {
      s.push_back('_');
      last_char = '_';
    }
  }

  // Remove underscores at start and end
  if (s.size() > 1 && s[0] == '_') {
    if (!cxx_escaped(s)) {
      s.erase(0, 1);
    }
  }
  if (s.size() > 1 && s[s.size() - 1] == '_') {
    s.erase(s.size() - 1, 1);
  }

  return s;
}

std::string dds_generator::get_tag_name(const std::string& base_name, bool nested_key_only)
{
  return valid_var_name(base_name) + (nested_key_only ? "_nested_key_only" : "") + "_tag";
}

string dds_generator::to_string(Identifier* id, EscapeContext ec)
{
  string str = id->get_string();
  switch (ec) {
  case EscapeContext_ForGenIdl:
    if (id->escaped()) {
      // If it was escaped in the input, it must be escaped in generated IDL.
      if (cxx_escaped(str)) {
        // Strip "_cxx"
        str = str.substr(cxx_escape.size() - 1);
      } else {
        str = '_' + str;
      }
    }
    break;
  case EscapeContext_FromGenIdl:
  case EscapeContext_StripEscapes:
    if (id->escaped() && cxx_escaped(str)) {
      // If this is a C++ keyword that was inserted into generated IDL, the
      // "_cxx_" was stripped.
      str = str.substr(cxx_escape.size());
    }
    break;
  case EscapeContext_Normal:
    break;
  }
  return str;
}

string dds_generator::scoped_helper(UTL_ScopedName* sn, const char* sep, EscapeContext ec)
{
  string sname;

  for (; sn; sn = static_cast<UTL_ScopedName*>(sn->tail())) {
    const bool not_last = sn->tail();
    sname += to_string(sn->head(),
      (ec == EscapeContext_FromGenIdl && not_last) ? EscapeContext_Normal : ec);

    if (sname.size() && not_last) {
      sname += sep;
    }
  }

  return sname;
}

string dds_generator::module_scope_helper(UTL_ScopedName* sn, const char* sep, EscapeContext ec)
{
  string sname;

  for (; sn && sn->tail(); sn = static_cast<UTL_ScopedName*>(sn->tail())) {
    sname += to_string(sn->head(), ec == EscapeContext_FromGenIdl ? EscapeContext_Normal : ec);

    if (sname.size()) {
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

string type_to_default_array(const std::string& indent, AST_Type* type, const string& name,
  bool is_anonymous, bool is_union, bool use_cxx11, Classification fld_cls)
{
  string val;
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
    val += indent + "set_default(IDL::DistinctType<" + n + ", " + dds_generator::get_tag_name(n) + ">(" +
      (is_union ? "tmp" : name) + "));\n";
  } else {
    string n = scoped(type->name());
    if (is_anonymous) {
      n = n.substr(0, n.rfind("::") + 2) + "_" + type->local_name()->get_string();
      n = (fld_cls == AST_Decl::NT_sequence) ? (n + "_seq") : n;
    }
    val = indent + n + "_forany " + temp + "(const_cast<"
      + n + "_slice*>(" + (is_union ? "tmp": name) + "));\n";
    val += indent + "set_default(" + temp + ");\n";
    if (is_union) {
      val += indent + name + "(tmp);\n";
    }
  }
  return val;
}

string type_to_default(const std::string& indent, AST_Type* type, const string& name,
  bool is_anonymous, bool is_union)
{
  AST_Type* actual_type = resolveActualType(type);
  Classification fld_cls = classify(actual_type);
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  string def_val;
  std::string pre = " = ";
  std::string post;
  if (is_union) {
    pre = "(";
    post = ")";
  }
  if (fld_cls & (CL_STRUCTURE | CL_UNION)) {
    return indent + "set_default(" + name + (is_union ? "()" : "") + ");\n";
  } else if (fld_cls & CL_ARRAY) {
    return type_to_default_array(
      indent, type, name, is_anonymous, is_union, use_cxx11, fld_cls);
  } else if (fld_cls & CL_ENUM) {
    // For now, simply return the first value of the enumeration.
    // Must be changed, if support for @default_literal is desired.
    AST_Enum* enu = dynamic_cast<AST_Enum*>(actual_type);
    UTL_ScopeActiveIterator i(enu, UTL_Scope::IK_decls);
    AST_EnumVal *item = dynamic_cast<AST_EnumVal*>(i.item());
    if (use_cxx11) {
      def_val = scoped(type->name()) + "::" + item->local_name()->get_string();
    } else {
      def_val = scoped(item->name());
    }
  } else if (fld_cls & CL_SEQUENCE) {
    string seq_resize_func = (use_cxx11) ? "resize" : "length";
    if (is_union) {
      return indent + "tmp." + seq_resize_func + "(0);\n"
        + indent + name + "(tmp);\n";
    } else {
      return indent + name + "." + seq_resize_func + "(0);\n";
    }
  } else if (fld_cls & CL_STRING) {
    def_val = (fld_cls & CL_WIDE) ? "L\"\"" : "\"\"";
    if (!use_cxx11 && (fld_cls & CL_WIDE)) def_val = "TAO::WString_Manager::s_traits::default_initializer()";
  } else if (fld_cls & (CL_PRIMITIVE | CL_FIXED)) {
    AST_PredefinedType* pt = dynamic_cast<AST_PredefinedType*>(actual_type);
    if (pt && (pt->pt() == AST_PredefinedType::PT_longdouble)) {
      if (use_cxx11) {
        def_val = "0.0L";
      } else {
        string temp_val = "ACE_CDR::LongDouble val = ACE_CDR_LONG_DOUBLE_INITIALIZER";
        if (is_union) {
          def_val = "val";
          return indent + temp_val + ";\n" +
            indent + name + pre + def_val + post + ";\n";
        } else {
          def_val = "ACE_CDR_LONG_DOUBLE_ASSIGNMENT(" + name + ", val)";
          return indent + "{\n"
            "  " + indent + temp_val + ";\n"
            "  " + indent + def_val + ";\n" +
            indent + "}\n";
        }
      }
    } else {
      def_val =  "0";
    }
  }
  return indent + name + pre + def_val + post + ";\n";
}
