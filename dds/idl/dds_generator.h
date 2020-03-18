/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef dds_generator_H
#define dds_generator_H

#include "be_extern.h"

#include "utl_scoped_name.h"
#include "utl_identifier.h"
#include "utl_string.h"

#include "ast.h"
#include "ast_component_fwd.h"
#include "ast_eventtype_fwd.h"
#include "ast_structure_fwd.h"
#include "ast_union_fwd.h"
#include "ast_valuetype_fwd.h"

#include "ace/CDR_Base.h"

#include <string>
#include <vector>
#include <cstring>

#include "../DCPS/RestoreOutputStreamState.h"

class dds_generator {
public:
  virtual ~dds_generator() = 0;

  virtual bool do_included_files() const { return false; }

  virtual void gen_prologue() {}

  virtual void gen_epilogue() {}

  virtual bool gen_const(UTL_ScopedName* /*name*/,
                         bool /*nestedInInteface*/,
                         AST_Constant* /*constant*/)
  { return true; }

  virtual bool gen_enum(AST_Enum* /*node*/, UTL_ScopedName* /*name*/,
                        const std::vector<AST_EnumVal*>& /*contents*/,
                        const char* /*repoid*/)
  { return true; }

  virtual bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                          const std::vector<AST_Field*>& fields,
                          AST_Type::SIZE_TYPE size,
                          const char* repoid) = 0;

  virtual bool gen_struct_fwd(UTL_ScopedName* /*name*/,
                              AST_Type::SIZE_TYPE /*size*/)
  { return true; }

  virtual bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* base,
                           const char* repoid) = 0;

  virtual bool gen_interf(AST_Interface* /*node*/, UTL_ScopedName* /*name*/, bool /*local*/,
                          const std::vector<AST_Interface*>& /*inherits*/,
                          const std::vector<AST_Interface*>& /*inherits_flat*/,
                          const std::vector<AST_Attribute*>& /*attrs*/,
                          const std::vector<AST_Operation*>& /*ops*/,
                          const char* /*repoid*/)
  { return true; }

  virtual bool gen_interf_fwd(UTL_ScopedName* /*name*/)
  { return true; }

  virtual bool gen_native(AST_Native* /*node*/, UTL_ScopedName* /*name*/, const char* /*repoid*/)
  { return true; }

  virtual bool gen_union(AST_Union* node, UTL_ScopedName* name,
                         const std::vector<AST_UnionBranch*>& branches,
                         AST_Type* discriminator,
                         const char* repoid) = 0;

  virtual bool gen_union_fwd(AST_UnionFwd* /*node*/, UTL_ScopedName* /*name*/,
                             AST_Type::SIZE_TYPE /*size*/)
  { return true; }

  static std::string scoped_helper(UTL_ScopedName* sn, const char* sep);
  static std::string module_scope_helper(UTL_ScopedName* sn, const char* sep);
};

class composite_generator : public dds_generator {
public:
  void gen_prologue();

  void gen_epilogue();

  bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                 AST_Constant* constant);

  bool gen_enum(AST_Enum* node, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_struct_fwd(UTL_ScopedName* name, AST_Type::SIZE_TYPE size);

  bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* base, const char* repoid);

  bool gen_interf(AST_Interface* node, UTL_ScopedName* name, bool local,
                  const std::vector<AST_Interface*>& inherits,
                  const std::vector<AST_Interface*>& inherits_flat,
                  const std::vector<AST_Attribute*>& attrs,
                  const std::vector<AST_Operation*>& ops, const char* repoid);

  bool gen_interf_fwd(UTL_ScopedName* name);

  bool gen_native(AST_Native* node, UTL_ScopedName* name, const char* repoid);

  bool gen_union(AST_Union* node, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* discriminator,
                 const char* repoid);

  bool gen_union_fwd(AST_UnionFwd*, UTL_ScopedName* name, AST_Type::SIZE_TYPE size);

  composite_generator() : components_() {}

  template <typename InputIterator>
  composite_generator(InputIterator begin, InputIterator end)
  : components_(begin, end) {}

  void add_generator(dds_generator* gen) { components_.push_back(gen); }

private:
  std::vector<dds_generator*> components_;
};

// common utilities for all "generator" derived classes

struct NamespaceGuard {
  NamespaceGuard()
  {
    be_global->header_ << "OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL\nnamespace OpenDDS { namespace DCPS {\n\n";
    be_global->impl_ << "OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL\nnamespace OpenDDS { namespace DCPS {\n\n";
  }
  ~NamespaceGuard()
  {
    be_global->header_ << "}  }\nOPENDDS_END_VERSIONED_NAMESPACE_DECL\n\n";
    be_global->impl_ << "}  }\nOPENDDS_END_VERSIONED_NAMESPACE_DECL\n\n";
  }
};

struct ScopedNamespaceGuard  {
  ScopedNamespaceGuard(UTL_ScopedName* name, std::ostream& os,
                       const char* keyword = "namespace")
    : os_(os)
    , semi_()
    , n_(0)
  {
    for (n_ = 0; name->tail();
         name = static_cast<UTL_ScopedName*>(name->tail())) {
      const char* str = name->head()->get_string();
      if (str && str[0]) {
        ++n_;
        os_ << keyword << (name->head()->escaped() ? " _" : " ")
            << str << " {\n";
      }
    }
    if (std::strcmp(keyword, "module") == 0) semi_ = ";";
  }

  ~ScopedNamespaceGuard()
  {
    for (int i = 0; i < n_; ++i) os_ << '}' << semi_ << '\n';
  }

  std::ostream& os_;
  std::string semi_;
  int n_;
};

struct Function {
  bool has_arg_;
  std::string preamble_;

  Function(const char* name, const char* returntype)
    : has_arg_(false)
  {
    using std::string;
    ACE_CString ace_exporter = be_global->export_macro();
    bool use_exp = ace_exporter != "";
    string exporter = use_exp ? (string(" ") + ace_exporter.c_str()) : "";
    be_global->header_ << ace_exporter << (use_exp ? "\n" : "")
      << returntype << " " << name << "(";
    be_global->impl_ << returntype << " " << name << "(";
  }

  void addArg(const char* name, const std::string& type)
  {
    std::string sig = (has_arg_ ? ", " : "") + type + (name[0] ? " " : "")
      + (name[0] ? name : "");
    be_global->header_ << sig;
    be_global->impl_ << sig;
    if (name[0]) {
      preamble_ += "  ACE_UNUSED_ARG(" + std::string(name) + ");\n";
    }
    has_arg_ = true;
  }

  void endArgs()
  {
    be_global->header_ << ");\n\n";
    be_global->impl_ << ")\n{\n" << preamble_;
  }

  ~Function()
  {
    be_global->impl_ << "}\n\n";
  }
};

inline std::string scoped(UTL_ScopedName* sn)
{
  return dds_generator::scoped_helper(sn, "::");
}

inline std::string module_scope(UTL_ScopedName* sn)
{
  return dds_generator::module_scope_helper(sn, "::");
}

namespace AstTypeClassification {
  inline AST_Type* resolveActualType(AST_Type* element)
  {
    if (element->node_type() == AST_Decl::NT_typedef) {
      AST_Typedef* td = AST_Typedef::narrow_from_decl(element);
      return td->primitive_base_type();
    }

    switch(element->node_type()) {
    case AST_Decl::NT_interface_fwd:
    {
      AST_InterfaceFwd* td = AST_InterfaceFwd::narrow_from_decl(element);
      return td->full_definition();
    }
    case AST_Decl::NT_valuetype_fwd:
    {
      AST_ValueTypeFwd* td = AST_ValueTypeFwd::narrow_from_decl(element);
      return td->full_definition();
    }
    case AST_Decl::NT_union_fwd:
    {
      AST_UnionFwd* td = AST_UnionFwd::narrow_from_decl(element);
      return td->full_definition();
    }
    case AST_Decl::NT_struct_fwd:
    {
      AST_StructureFwd* td = AST_StructureFwd::narrow_from_decl(element);
      return td->full_definition();
    }
    case AST_Decl::NT_component_fwd:
    {
      AST_ComponentFwd* td = AST_ComponentFwd::narrow_from_decl(element);
      return td->full_definition();
    }
    case AST_Decl::NT_eventtype_fwd:
    {
      AST_EventTypeFwd* td = AST_EventTypeFwd::narrow_from_decl(element);
      return td->full_definition();
    }
    default:
      return element;
    }
  }

  typedef size_t Classification;
  const Classification CL_UNKNOWN = 0, CL_SCALAR = 1, CL_PRIMITIVE = 2,
    CL_STRUCTURE = 4, CL_STRING = 8, CL_ENUM = 16, CL_UNION = 32, CL_ARRAY = 64,
    CL_SEQUENCE = 128, CL_WIDE = 256, CL_BOUNDED = 512, CL_INTERFACE = 1024,
    CL_FIXED = 2048;

  inline Classification classify(AST_Type* type)
  {
    type = AstTypeClassification::resolveActualType(type);
    switch (type->node_type()) {
    case AST_Decl::NT_pre_defined: {
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
      switch (p->pt()) {
      case AST_PredefinedType::PT_any:
      case AST_PredefinedType::PT_object:
        return CL_UNKNOWN;
      case AST_PredefinedType::PT_wchar:
        return CL_SCALAR | CL_PRIMITIVE | CL_WIDE;
      default:
        return CL_SCALAR | CL_PRIMITIVE;
      }
    }
    case AST_Decl::NT_array:
      return CL_ARRAY;
    case AST_Decl::NT_union:
      return CL_UNION;
    case AST_Decl::NT_string:
    case AST_Decl::NT_wstring:
      return CL_SCALAR | CL_STRING |
        ((AST_String::narrow_from_decl(type)->max_size()->ev()->u.ulval == 0)
        ? 0 : CL_BOUNDED) |
        ((type->node_type() == AST_Decl::NT_wstring) ? CL_WIDE : 0);
    case AST_Decl::NT_sequence:
      return CL_SEQUENCE |
        ((AST_Sequence::narrow_from_decl(type)->unbounded()) ? 0 : CL_BOUNDED);
    case AST_Decl::NT_struct:
      return CL_STRUCTURE;
    case AST_Decl::NT_enum:
      return CL_SCALAR | CL_ENUM;
    case AST_Decl::NT_interface:
      return CL_INTERFACE;
#ifdef ACE_HAS_CDR_FIXED
    case AST_Decl::NT_fixed:
      return CL_FIXED;
#endif
    default:
      return CL_UNKNOWN;
    }
  }
}

struct NestedForLoops {
  NestedForLoops(const char* type, const char* prefix, AST_Array* arr,
                 std::string& indent, bool followTypedefs = false);
  ~NestedForLoops();

  size_t n_;
  std::string& indent_;
  std::string index_;
};

enum WrapDirection {WD_OUTPUT, WD_INPUT};

inline
std::string wrapPrefix(AST_Type* type, WrapDirection wd)
{
  switch (type->node_type()) {
  case AST_Decl::NT_pre_defined: {
    AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
    switch (p->pt()) {
    case AST_PredefinedType::PT_char:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_char(" : "ACE_InputCDR::to_char(";
    case AST_PredefinedType::PT_wchar:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_wchar(" : "ACE_InputCDR::to_wchar(";
    case AST_PredefinedType::PT_octet:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_octet(" : "ACE_InputCDR::to_octet(";
    case AST_PredefinedType::PT_boolean:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_boolean(" : "ACE_InputCDR::to_boolean(";
    default:
      return "";
    }
  }
  case AST_Decl::NT_string:
    return (wd == WD_OUTPUT)
      ? "ACE_OutputCDR::from_string(" : "ACE_InputCDR::to_string(";
  case AST_Decl::NT_wstring:
    return (wd == WD_OUTPUT)
      ? "ACE_OutputCDR::from_wstring(" : "ACE_InputCDR::to_wstring(";
  default:
    return "";
  }
}

inline
std::string getWrapper(const std::string& name, AST_Type* type, WrapDirection wd)
{
  using namespace AstTypeClassification;
  if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
    const Classification cls = classify(type);
    if ((cls & (CL_BOUNDED | CL_STRING)) == (CL_BOUNDED | CL_STRING)) {
      return (wd == WD_OUTPUT ? "Serializer::FromBoundedString" : "Serializer::ToBoundedString")
        + std::string(cls & CL_WIDE ? "<wchar_t>(" : "<char>(") + name + ')';
    }
  }
  std::string pre = wrapPrefix(type, wd);
  return (pre.empty()) ? name : (pre + name + ')');
}

inline
std::string getEnumLabel(AST_Expression* label_val, AST_Type* disc)
{
  std::string e = scoped(disc->name()),
    label = label_val->n()->last_component()->get_string();
  if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
    return e + "::" + label;
  }
  const size_t colon = e.rfind("::");
  if (colon == std::string::npos) {
    return label;
  }
  return e.replace(colon + 2, std::string::npos, label);
}

inline
std::ostream& operator<<(std::ostream& o,
                         const AST_Expression::AST_ExprValue& ev)
{
  OpenDDS::DCPS::RestoreOutputStreamState ross(o);
  switch (ev.et) {
  case AST_Expression::EV_octet:
    return o << static_cast<int>(ev.u.oval);
  case AST_Expression::EV_short:
    return o << ev.u.sval;
  case AST_Expression::EV_ushort:
    return o << ev.u.usval << 'u';
  case AST_Expression::EV_long:
    return o << ev.u.lval;
  case AST_Expression::EV_ulong:
    return o << ev.u.ulval << 'u';
  case AST_Expression::EV_longlong:
    return o << ev.u.llval << "LL";
  case AST_Expression::EV_ulonglong:
    return o << ev.u.ullval << "ULL";
  case AST_Expression::EV_wchar:
    return o << "L'" << static_cast<char>(ev.u.wcval) << '\'';
  case AST_Expression::EV_char:
    return o << '\'' << ev.u.cval << '\'';
  case AST_Expression::EV_bool:
    return o << std::boolalpha << static_cast<bool>(ev.u.bval);
  case AST_Expression::EV_float:
    return o << ev.u.fval << 'f';
  case AST_Expression::EV_double:
    return o << ev.u.dval;
  case AST_Expression::EV_wstring:
    return o << "L\"" << ev.u.wstrval << '"';
  case AST_Expression::EV_string:
    return o << '"' << ev.u.strval->get_string() << '"';
#ifdef ACE_HAS_CDR_FIXED
  case AST_Expression::EV_fixed: {
    char buf[ACE_CDR::Fixed::MAX_STRING_SIZE];
    ev.u.fixedval.to_string(buf, sizeof buf);
    return o << "\"" << buf << "\"";
  }
#endif
  default:
    return o;
  }
}

inline
void generateBranchLabels(AST_UnionBranch* branch, AST_Type* discriminator,
                          size_t& n_labels, bool& has_default)
{
  for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
    ++n_labels;
    AST_UnionLabel* label = branch->label(j);
    if (label->label_kind() == AST_UnionLabel::UL_default) {
      be_global->impl_ << "  default:";
      has_default = true;
    } else if (discriminator->node_type() == AST_Decl::NT_enum) {
      be_global->impl_ << "  case "
        << getEnumLabel(label->label_val(), discriminator) << ':';
    } else {
      be_global->impl_ << "  case " << *label->label_val()->ev() << ':';
    }
    be_global->impl_<< ((j == branch->label_list_length() - 1) ? " {\n" : "\n");
  }
}

// see TAO_IDL_BE be_union::gen_empty_default_label()
inline bool needSyntheticDefault(AST_Type* disc, size_t n_labels)
{
  AST_Decl::NodeType nt = disc->node_type();
  if (nt == AST_Decl::NT_enum) return true;

  AST_PredefinedType* pdt = AST_PredefinedType::narrow_from_decl(disc);
  switch (pdt->pt()) {
  case AST_PredefinedType::PT_boolean:
    return n_labels < 2;
  case AST_PredefinedType::PT_char:
    return n_labels < ACE_OCTET_MAX;
  case AST_PredefinedType::PT_short:
  case AST_PredefinedType::PT_ushort:
    return n_labels < ACE_UINT16_MAX;
  case AST_PredefinedType::PT_long:
  case AST_PredefinedType::PT_ulong:
    return n_labels < ACE_UINT32_MAX;
  default:
    return true;
  }
}

typedef std::string (*CommonFn)(const std::string& name, AST_Type* type,
                                const std::string& prefix, std::string& intro,
                                const std::string&);

inline
void generateCaseBody(CommonFn commonFn, AST_UnionBranch* branch,
                      const char* statementPrefix, const char* namePrefix,
                      const char* uni, bool generateBreaks, bool parens)
{
  using namespace AstTypeClassification;
  const BE_GlobalData::LanguageMapping lmap = be_global->language_mapping();
  const bool use_cxx11 = lmap == BE_GlobalData::LANGMAP_CXX11;
  const std::string name = branch->local_name()->get_string();
  if (namePrefix == std::string(">> ")) {
    std::string brType = scoped(branch->field_type()->name()), forany;
    AST_Type* br = resolveActualType(branch->field_type());
    Classification br_cls = classify(br);
    if (!br->in_main_file()
        && br->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(br->file_name().c_str());
    }
    std::string rhs;
    if (br_cls & CL_STRING) {
      if (use_cxx11) {
        brType = std::string("std::") + ((br_cls & CL_WIDE) ? "w" : "")
          + "string";
        rhs = "tmp";
      } else {
        const std::string nmspace =
          lmap == BE_GlobalData::LANGMAP_FACE_CXX ? "FACE::" : "CORBA::";
        brType = nmspace + ((br_cls & CL_WIDE) ? "W" : "")
          + "String_var";
        rhs = "tmp.out()";
      }
    } else if (use_cxx11 && (br_cls & (CL_ARRAY | CL_SEQUENCE))) {
      rhs = "IDL::DistinctType<" + brType + ", " +
        dds_generator::scoped_helper(branch->field_type()->name(), "_")
        + "_tag>(tmp)";
    } else if (br_cls & CL_ARRAY) {
      forany = "    " + brType + "_forany fa = tmp;\n";
      rhs = getWrapper("fa", br, WD_INPUT);
    } else {
      rhs = getWrapper("tmp", br, WD_INPUT);
    }
    be_global->impl_ <<
      "    " << brType << " tmp;\n" << forany <<
      "    if (strm >> " << rhs << ") {\n"
      "      uni." << name << (use_cxx11 ? "(std::move(tmp));\n" : "(tmp);\n") <<
      "      uni._d(disc);\n"
      "      return true;\n"
      "    }\n"
      "    return false;\n";
  } else {
    const char* breakString = generateBreaks ? "    break;\n" : "";
    std::string intro;
    std::string expr = commonFn(name + (parens ? "()" : ""), branch->field_type(),
                                std::string(namePrefix) + "uni", intro, uni);
    be_global->impl_ <<
      (intro.empty() ? "" : "  ") << intro;
    if (*statementPrefix) {
      be_global->impl_ <<
        "    " << statementPrefix << " " << expr << ";\n" <<
        (statementPrefix == std::string("return") ? "" : breakString);
    } else {
      be_global->impl_ << expr << breakString;
    }
  }
}

inline
bool generateSwitchBody(CommonFn commonFn,
                        const std::vector<AST_UnionBranch*>& branches,
                        AST_Type* discriminator, const char* statementPrefix,
                        const char* namePrefix = "", const char* uni = "",
                        bool forceDisableDefault = false, bool parens = true,
                        bool breaks = true)
{
  size_t n_labels = 0;
  bool has_default = false;
  for (size_t i = 0; i < branches.size(); ++i) {
    AST_UnionBranch* branch = branches[i];
    if (forceDisableDefault) {
      bool foundDefault = false;
      for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
        if (branch->label(j)->label_kind() == AST_UnionLabel::UL_default) {
          foundDefault = true;
        }
      }
      if (foundDefault) {
        has_default = true;
        continue;
      }
    }
    generateBranchLabels(branch, discriminator, n_labels, has_default);
    generateCaseBody(commonFn, branch, statementPrefix, namePrefix, uni, breaks, parens);
    be_global->impl_ <<
      "  }\n";
  }
  if (!has_default && needSyntheticDefault(discriminator, n_labels)) {
    be_global->impl_ <<
      "  default:\n" <<
      ((namePrefix == std::string(">> ")) ? "    uni._d(disc);\n" : "") <<
      "    break;\n";
    return true;
  }
  return false;
}

/// returns true if a default: branch was generated (no default: label in IDL)
inline
bool generateSwitchForUnion(const char* switchExpr, CommonFn commonFn,
                            const std::vector<AST_UnionBranch*>& branches,
                            AST_Type* discriminator, const char* statementPrefix,
                            const char* namePrefix = "", const char* uni = "",
                            bool forceDisableDefault = false, bool parens = true,
                            bool breaks = true)
{
  using namespace AstTypeClassification;
  AST_Type* dt = resolveActualType(discriminator);
  AST_PredefinedType* bt = AST_PredefinedType::narrow_from_decl(dt);
  if (bt && bt->pt() == AST_PredefinedType::PT_boolean) {
    AST_UnionBranch* true_branch = 0;
    AST_UnionBranch* false_branch = 0;
    AST_UnionBranch* default_branch = 0;
    for (std::vector<AST_UnionBranch*>::const_iterator pos = branches.begin(),
         limit = branches.end(); pos != limit; ++pos) {
      AST_UnionBranch* branch = *pos;
      for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
        AST_UnionLabel* label = branch->label(j);
        if (label->label_kind() == AST_UnionLabel::UL_default) {
          default_branch = branch;
        } else if (label->label_val()->ev()->u.bval) {
          true_branch = branch;
        } else if (!label->label_val()->ev()->u.bval) {
          false_branch = branch;
        }
      }
    }

    if (true_branch || false_branch) {
      be_global->impl_ <<
        "  if (" << switchExpr << ") {\n";
    } else {
      be_global->impl_ <<
        "  {\n";
    }

    if (true_branch || default_branch) {
      generateCaseBody(commonFn, true_branch ? true_branch : default_branch,
                       statementPrefix, namePrefix, uni, false, parens);
    }

    if (false_branch || (default_branch && true_branch)) {
      be_global->impl_ <<
        "  } else {\n";
      generateCaseBody(commonFn, false_branch ? false_branch : default_branch,
                       statementPrefix, namePrefix, uni, false, parens);
    }

    be_global->impl_ <<
      "  }\n";

    return !default_branch && bool(true_branch) != bool(false_branch);

  } else {
    be_global->impl_ <<
      "  switch (" << switchExpr << ") {\n";
    bool b(generateSwitchBody(commonFn, branches, discriminator,
                              statementPrefix, namePrefix, uni,
                              forceDisableDefault, parens, breaks));
    be_global->impl_ <<
      "  }\n";
    return b;
  }
}

inline
std::string insert_cxx11_accessor_parens(
              const std::string& full_var_name_, bool is_union_member) {
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  if (!use_cxx11 || is_union_member) return full_var_name_;

  std::string full_var_name(full_var_name_);
  std::string::size_type n = 0;
  while ((n = full_var_name.find('.', n)) != std::string::npos) {
    if (full_var_name[n-1] != ']') {
      full_var_name.insert(n, "()");
      n += 3;
    } else {
      ++n;
    }
  }
  n = 0;
  while ((n = full_var_name.find('[', n)) != std::string::npos) {
    full_var_name.insert(n, "()");
    n += 3;
  }
  return full_var_name[full_var_name.size() - 1] == ']'
    ? full_var_name : full_var_name + "()";
}

/**
 * Wrapper for Iterating Over Structure Fields
 */
class Fields {
public:
  class Iterator {
  public:
    typedef AST_Field* value_type;
    typedef AST_Field** pointer;
    typedef AST_Field*& reference;
    typedef std::input_iterator_tag iterator_category;

    explicit Iterator(AST_Structure* node = 0, unsigned pos = 0)
    : node_(node)
    , pos_(pos)
    {
      check();
    }

    bool valid() const {
      return node_ && pos_ < node_->nfields();
    }

    void check()
    {
      if (!valid()) {
        node_ = 0;
        pos_ = 0;
      }
    }

    unsigned pos() const
    {
      return pos_;
    }

    Iterator& operator++() // Prefix
    {
      if (node_) {
        ++pos_;
        check();
      }
      return *this;
    }

    Iterator operator++(int) // Postfix
    {
      Iterator prev(*this);
      ++(*this);
      return prev;
    }

    AST_Field* operator*() const
    {
      if (node_) {
        AST_Field** field_ptrptr;
        node_->field(field_ptrptr, pos_);
        if (field_ptrptr) {
          return *field_ptrptr;
        }
      }
      return 0;
    }

    bool operator==(const Iterator& other) const
    {
      return node_ == other.node_ && pos_ == other.pos_;
    }

    bool operator!=(const Iterator& other) const
    {
      return !(*this == other);
    }

  private:
    AST_Structure* node_;
    unsigned pos_;
  };

  explicit Fields(AST_Structure* node = 0)
  : node_(node)
  {
  }

  AST_Structure* node() const
  {
    return node_;
  }

  Iterator begin() const
  {
    return Iterator(node_);
  }

  Iterator end() const
  {
    static Iterator end_value;
    return end_value;
  }

  Iterator operator[](unsigned position) const
  {
    return Iterator(node_, position);
  }

private:
  AST_Structure* node_;
};

#endif
