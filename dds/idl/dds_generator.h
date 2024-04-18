/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef dds_generator_H
#define dds_generator_H

#include "be_extern.h"
#include "be_util.h"

#include <dds/DCPS/ValueHelper.h>

#include <utl_scoped_name.h>
#include <utl_identifier.h>
#include <utl_string.h>
#include <ast.h>
#include <ast_component_fwd.h>
#include <ast_eventtype_fwd.h>
#include <ast_structure_fwd.h>
#include <ast_union_fwd.h>
#include <ast_valuetype_fwd.h>

#include <ace/CDR_Base.h>

#include <string>
#include <vector>
#include <cstring>
#include <set>
#include <stdexcept>
#include <iomanip>
#include <cctype>
#include <climits>

/// How to handle IDL underscore escaping. Depends on where the name is
/// going and where the name came from.
enum EscapeContext {
  /// This is for generated IDL. (Like *TypeSupport.idl)
  EscapeContext_ForGenIdl,
  /// This is for a name coming from generated IDL. (Like *TypeSupportC.h)
  EscapeContext_FromGenIdl,
  /// Strip any escapes
  EscapeContext_StripEscapes,
  /// This is for everything else.
  EscapeContext_Normal,
};

class dds_generator {
public:
  virtual ~dds_generator() = 0;

  static std::string get_tag_name(const std::string& base_name, const std::string& qualifier = "");

  static std::string get_xtag_name(UTL_ScopedName* name);

  static bool cxx_escaped(const std::string& s);

  static std::string valid_var_name(const std::string& str);

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

  static std::string to_string(
    Identifier* id, EscapeContext ec = EscapeContext_Normal);
  static std::string scoped_helper(
    UTL_ScopedName* sn, const char* sep, EscapeContext cxt = EscapeContext_Normal);
  static std::string module_scope_helper(
    UTL_ScopedName* sn, const char* sep, EscapeContext cxt = EscapeContext_Normal);

  static bool gen_enum_helper(AST_Enum* node, UTL_ScopedName* name,
    const std::vector<AST_EnumVal*>& contents, const char* repoid);
};

inline std::string canonical_name(UTL_ScopedName* sn)
{
  // NOTE: Names should not have leading "::" according to the XTypes spec.
  return dds_generator::scoped_helper(sn, "::", EscapeContext_StripEscapes);
}

inline std::string canonical_name(Identifier* id)
{
  return dds_generator::to_string(id, EscapeContext_StripEscapes);
}

inline std::string canonical_name(AST_Decl* node)
{
  return canonical_name(node->local_name());
}

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
  const bool enabled_;
  std::vector<std::string>* ns_;
  std::vector<std::string> default_ns_;

  NamespaceGuard(bool enabled = true, std::vector<std::string>* ns = 0)
    : enabled_(enabled)
    , ns_(ns)
  {
    if (!ns_) {
      default_ns_.push_back("OpenDDS");
      default_ns_.push_back("DCPS");
      ns_ = &default_ns_;
    }
    if (enabled_) {
      std::string start_ns = "OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL\n";
      for (size_t i = 0; i < ns_->size(); ++i) {
        if (i > 0) {
          start_ns += " ";
        }
        start_ns += "namespace " + (*ns_)[i] + " {";
      }
      start_ns += "\n\n";
      be_global->header_ << start_ns;
      be_global->impl_ << start_ns;
    }
  }

  ~NamespaceGuard()
  {
    if (enabled_) {
      std::string end_ns;
      for (size_t i = 0; i < ns_->size(); ++i) {
        if (i > 0) {
          end_ns += " ";
        }
        end_ns += "}";
      }
      end_ns += "\nOPENDDS_END_VERSIONED_NAMESPACE_DECL\n\n";
      be_global->header_ << end_ns;
      be_global->impl_ << end_ns;
    }
  }
};

struct ScopedNamespaceGuard  {
  ScopedNamespaceGuard(UTL_ScopedName* name, std::ostream& os,
                       const char* keyword = "namespace")
    : os_(os)
    , n_(0)
  {
    const bool idl = !std::strcmp(keyword, "module");
    const ACE_CString vn_begin = be_global->versioning_begin();
    if (!idl && !vn_begin.empty()) {
      os_ << vn_begin << '\n';
      suffix_ = (be_global->versioning_end() + '\n').c_str();
    }
    const EscapeContext ec = idl ? EscapeContext_ForGenIdl : EscapeContext_Normal;
    for (n_ = 0; name->tail();
         name = static_cast<UTL_ScopedName*>(name->tail())) {
      const char* str = name->head()->get_string();
      if (str && str[0]) {
        ++n_;
        os_ << keyword << ' ' << dds_generator::to_string(name->head(), ec) << " {\n";
      }
    }
    if (idl) semi_ = ";";
  }

  ~ScopedNamespaceGuard()
  {
    for (int i = 0; i < n_; ++i) {
      os_ << '}' << semi_ << '\n';
    }
    os_ << suffix_;
  }

  std::ostream& os_;
  std::string semi_, suffix_;
  int n_;
};

struct Function {
  bool has_arg_;
  std::string preamble_;
  bool extra_newline_;

  Function(const std::string& name, const std::string returntype,
           const char* template_args = 0)
    : has_arg_(false)
    , extra_newline_(true)
  {
    using std::string;
    if (template_args) {
      const string tmpl = string("template<") + template_args + "> ";
      be_global->header_ << tmpl;
      be_global->impl_ << tmpl;
    }
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
    be_global->impl_ << "}\n";
    if (extra_newline_) {
      be_global->impl_ << "\n";
    }
  }
};

class PreprocessorIfGuard {
public:
  PreprocessorIfGuard(
    const std::string& what,
    bool impl = true, bool header = true,
    const std::string& indent = "")
    : what_(what)
    , impl_(impl)
    , header_(header)
    , indent_(indent)
    , extra_newline_(true)
  {
    output("#" + indent + "if" + what + "\n");
  }

  ~PreprocessorIfGuard()
  {
    output("#" + indent_ + "endif // if" + what_ + "\n");
    if (extra_newline_) {
      output("\n");
    }
  }

  void output(const std::string& str) const
  {
    if (impl_) {
      be_global->impl_ << str;
    }
    if (header_) {
      be_global->header_ << str;
    }
  }

  void extra_newline(bool value)
  {
    extra_newline_ = value;
  }

private:
  const std::string what_;
  const bool impl_;
  const bool header_;
  const std::string indent_;
  bool extra_newline_;
};

inline std::string scoped(UTL_ScopedName* sn, EscapeContext ec = EscapeContext_Normal)
{
  // Add the leading scope operator here to make type names "fully-qualified" and avoid
  // naming collisions with identifiers in OpenDDS::DCPS.
  // The leading space allows this string to be used directly in a <>-delimeted template
  // argument list while avoiding the <: digraph.
  return " ::" + dds_generator::scoped_helper(sn, "::", ec);
}

inline std::string module_scope(UTL_ScopedName* sn)
{
  return dds_generator::module_scope_helper(sn, "::");
}

namespace AstTypeClassification {
  inline AST_Type* resolveActualType(AST_Type* element)
  {
    if (element->node_type() == AST_Decl::NT_typedef) {
      AST_Typedef* td = dynamic_cast<AST_Typedef*>(element);
      return td->primitive_base_type();
    }

    switch(element->node_type()) {
    case AST_Decl::NT_interface_fwd:
    {
      AST_InterfaceFwd* td = dynamic_cast<AST_InterfaceFwd*>(element);
      return td->full_definition();
    }
    case AST_Decl::NT_valuetype_fwd:
    {
      AST_ValueTypeFwd* td = dynamic_cast<AST_ValueTypeFwd*>(element);
      return td->full_definition();
    }
    case AST_Decl::NT_union_fwd:
    {
      AST_UnionFwd* td = dynamic_cast<AST_UnionFwd*>(element);
      return td->full_definition();
    }
    case AST_Decl::NT_struct_fwd:
    {
      AST_StructureFwd* td = dynamic_cast<AST_StructureFwd*>(element);
      return td->full_definition();
    }
    case AST_Decl::NT_component_fwd:
    {
      AST_ComponentFwd* td = dynamic_cast<AST_ComponentFwd*>(element);
      return td->full_definition();
    }
    case AST_Decl::NT_eventtype_fwd:
    {
      AST_EventTypeFwd* td = dynamic_cast<AST_EventTypeFwd*>(element);
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
    type = resolveActualType(type);
    switch (type->node_type()) {
    case AST_Decl::NT_pre_defined: {
      AST_PredefinedType* p = dynamic_cast<AST_PredefinedType*>(type);
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
        ((dynamic_cast<AST_String*>(type)->max_size()->ev()->u.ulval == 0)
        ? 0 : CL_BOUNDED) |
        ((type->node_type() == AST_Decl::NT_wstring) ? CL_WIDE : 0);
    case AST_Decl::NT_sequence:
      return CL_SEQUENCE |
        ((dynamic_cast<AST_Sequence*>(type)->unbounded()) ? 0 : CL_BOUNDED);
    case AST_Decl::NT_struct:
      return CL_STRUCTURE;
    case AST_Decl::NT_enum:
      return CL_SCALAR | CL_ENUM;
    case AST_Decl::NT_interface:
      return CL_INTERFACE;
    case AST_Decl::NT_fixed:
      return CL_FIXED;
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
    AST_PredefinedType* const p = dynamic_cast<AST_PredefinedType*>(type);
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
#if OPENDDS_HAS_EXPLICIT_INTS
    case AST_PredefinedType::PT_uint8:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_uint8(" : "ACE_InputCDR::to_uint8(";
    case AST_PredefinedType::PT_int8:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_int8(" : "ACE_InputCDR::to_int8(";
#endif
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

inline std::string string_type(AstTypeClassification::Classification cls)
{
  return be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11 ?
    ((cls & AstTypeClassification::CL_WIDE) ? "std::wstring" : "std::string") :
    (cls & AstTypeClassification::CL_WIDE) ? "TAO::WString_Manager" : "TAO::String_Manager";
}

inline std::string to_cxx_type(AST_Type* type, std::size_t& size)
{
  const AstTypeClassification::Classification cls = AstTypeClassification::classify(type);
  if (cls & AstTypeClassification::CL_ENUM) {
    size = 4;
    // Using the XTypes definition of Enums, this type is signed.
    // It contradicts the OMG standard CDR definition.
    return "ACE_CDR::Long";
  }
  if (cls & AstTypeClassification::CL_STRING) {
    return string_type(cls);
  }
  if (cls & AstTypeClassification::CL_PRIMITIVE) {
    AST_Type* t = AstTypeClassification::resolveActualType(type);
    AST_PredefinedType* p = dynamic_cast<AST_PredefinedType*>(t);
    switch (p->pt()) {
    case AST_PredefinedType::PT_long:
      size = 4;
      return "ACE_CDR::Long";
    case AST_PredefinedType::PT_ulong:
      size = 4;
      return "ACE_CDR::ULong";
    case AST_PredefinedType::PT_longlong:
      size = 8;
      return "ACE_CDR::LongLong";
    case AST_PredefinedType::PT_ulonglong:
      size = 8;
      return "ACE_CDR::ULongLong";
    case AST_PredefinedType::PT_short:
      size = 2;
      return "ACE_CDR::Short";
    case AST_PredefinedType::PT_ushort:
      size = 2;
      return "ACE_CDR::UShort";
#if OPENDDS_HAS_EXPLICIT_INTS
    case AST_PredefinedType::PT_int8:
      size = 1;
      return "ACE_CDR::Int8";
    case AST_PredefinedType::PT_uint8:
      size = 1;
      return "ACE_CDR::UInt8";
#endif
    case AST_PredefinedType::PT_float:
      size = 4;
      return "ACE_CDR::Float";
    case AST_PredefinedType::PT_double:
      size = 8;
      return "ACE_CDR::Double";
    case AST_PredefinedType::PT_longdouble:
      size = 16;
      return "ACE_CDR::LongDouble";
    case AST_PredefinedType::PT_char:
      size = 1;
      return "ACE_CDR::Char";
    case AST_PredefinedType::PT_wchar:
      size = 2;
      return "ACE_CDR::WChar";
    case AST_PredefinedType::PT_boolean:
      size = 1;
      return "ACE_CDR::Boolean";
    case AST_PredefinedType::PT_octet:
      size = 1;
      return "ACE_CDR::Octet";
    case AST_PredefinedType::PT_any:
    case AST_PredefinedType::PT_object:
    case AST_PredefinedType::PT_value:
    case AST_PredefinedType::PT_abstract:
    case AST_PredefinedType::PT_void:
    case AST_PredefinedType::PT_pseudo:
      be_util::misc_error_and_abort("Unsupported predefined type in to_cxx_type");
    }
    be_util::misc_error_and_abort("Unhandled predefined type in to_cxx_type");
  }
  return scoped(type->name());
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
  using namespace OpenDDS::DCPS;
  RestoreOutputStreamState ross(o);
  switch (ev.et) {
  case AST_Expression::EV_octet:
    return hex_value(o << "0x", static_cast<int>(ev.u.oval), 1);
#if OPENDDS_HAS_EXPLICIT_INTS
  case AST_Expression::EV_int8:
    return o << static_cast<short>(ev.u.int8val);
  case AST_Expression::EV_uint8:
    return o << static_cast<unsigned short>(ev.u.uint8val);
#endif
  case AST_Expression::EV_short:
    return o << ev.u.sval;
  case AST_Expression::EV_ushort:
    return o << ev.u.usval << 'u';
  case AST_Expression::EV_long:
    return signed_int_helper<ACE_CDR::Long>(o, ev.u.lval, ACE_INT32_MIN);
  case AST_Expression::EV_ulong:
    return o << ev.u.ulval << 'u';
  case AST_Expression::EV_longlong:
    return signed_int_helper<ACE_CDR::LongLong>(o, ev.u.llval, ACE_INT64_MIN) << "LL";
  case AST_Expression::EV_ulonglong:
    return o << ev.u.ullval << "ULL";
  case AST_Expression::EV_wchar:
    return char_helper<ACE_CDR::WChar>(o << "L'", ev.u.wcval) << '\'';
  case AST_Expression::EV_char:
    return char_helper<ACE_CDR::Char>(o << '\'', ev.u.cval) << '\'';
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
  case AST_Expression::EV_fixed: {
    char buf[ACE_CDR::Fixed::MAX_STRING_SIZE];
    ev.u.fixedval.to_string(buf, sizeof buf);
    return o << "\"" << buf << "\"";
  }
  case AST_Expression::EV_enum:
  case AST_Expression::EV_longdouble:
  case AST_Expression::EV_any:
  case AST_Expression::EV_object:
  case AST_Expression::EV_void:
  case AST_Expression::EV_none:
    be_util::misc_error_and_abort(
      "Unsupported ExprType value in operator<<(std::ostream, AST_ExprValue)");
  }
  be_util::misc_error_and_abort(
    "Unhandled ExprType value in operator<<(std::ostream, AST_ExprValue)");
  return o;
}

inline std::string bounded_arg(AST_Type* type)
{
  using namespace AstTypeClassification;
  std::ostringstream arg;
  const Classification cls = classify(type);
  if (cls & CL_STRING) {
    AST_String* const str = dynamic_cast<AST_String*>(type);
    arg << str->max_size()->ev()->u.ulval;
  } else if (cls & CL_SEQUENCE) {
    AST_Sequence* const seq = dynamic_cast<AST_Sequence*>(type);
    arg << seq->max_size()->ev()->u.ulval;
  }
  return arg.str();
}

std::string type_to_default(const std::string& indent, AST_Type* type,
  const std::string& name, bool is_anonymous = false, bool is_union = false, bool is_optional = false);

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

  AST_PredefinedType* pdt = dynamic_cast<AST_PredefinedType*>(disc);
  switch (pdt->pt()) {
  case AST_PredefinedType::PT_boolean:
    return n_labels < 2;
#if OPENDDS_HAS_EXPLICIT_INTS
  case AST_PredefinedType::PT_int8:
  case AST_PredefinedType::PT_uint8:
#endif
  case AST_PredefinedType::PT_char:
  case AST_PredefinedType::PT_octet:
    return n_labels < ACE_OCTET_MAX;
  case AST_PredefinedType::PT_short:
  case AST_PredefinedType::PT_ushort:
  case AST_PredefinedType::PT_wchar:
    return n_labels < ACE_UINT16_MAX;
  case AST_PredefinedType::PT_long:
  case AST_PredefinedType::PT_ulong:
    return n_labels < ACE_UINT32_MAX;
  default:
    return true;
  }
}

struct Intro {
  typedef std::set<std::string> LineSet;
  LineSet line_set;
  typedef std::vector<std::string> LineVec;
  LineVec line_vec;

  void join(std::ostream& os, const std::string& indent)
  {
    for (LineVec::iterator i = line_vec.begin(); i != line_vec.end(); ++i) {
      os << indent << *i << '\n';
    }
  }

  void insert(const std::string& line)
  {
    if (line_set.insert(line).second) {
      line_vec.push_back(line);
    }
  }

  void insert(const Intro& other)
  {
    for (LineVec::const_iterator i = other.line_vec.begin(); i != other.line_vec.end(); ++i) {
      insert(*i);
    }
  }
};

std::string field_type_name(AST_Field* field, AST_Type* field_type = 0);

/**
 * For the some situations, like a tag name, the type name we need is the
 * deepest named type, not the actual type. This will be the name of the
 * deepest typedef if it's an array or sequence, otherwise the name of the
 * type.
 */
AST_Type* deepest_named_type(AST_Type* type);

typedef std::string (*CommonFn)(
  const std::string& indent, AST_Decl* node,
  const std::string& name, AST_Type* type,
  const std::string& prefix, bool wrap_nested_key_only, Intro& intro,
  const std::string&);

inline
void generateCaseBody(
  CommonFn commonFn, CommonFn commonFn2,
  AST_UnionBranch* branch,
  const char* statementPrefix, const char* namePrefix, const char* uni, bool generateBreaks, bool parens)
{
  using namespace AstTypeClassification;
  const BE_GlobalData::LanguageMapping lmap = be_global->language_mapping();
  const bool use_cxx11 = lmap == BE_GlobalData::LANGMAP_CXX11;
  const std::string name = branch->local_name()->get_string();
  if (namePrefix == std::string(">> ")) {
    std::string brType = field_type_name(branch, branch->field_type());
    std::string forany;
    AST_Type* br = resolveActualType(branch->field_type());
    Classification br_cls = classify(br);
    if (!br->in_main_file()
        && br->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(br->file_name().c_str());
    }

    std::string rhs;
    const bool is_face = lmap == BE_GlobalData::LANGMAP_FACE_CXX;
    const bool is_wide = br_cls & CL_WIDE;
    const bool is_bound_string = (br_cls & (CL_STRING | CL_BOUNDED)) == (CL_STRING | CL_BOUNDED);
    const std::string bound_string_suffix = (is_bound_string && !is_face) ? ".c_str()" : "";

    if (is_bound_string) {
      const std::string to_type = is_face ? is_wide ? "ACE_InputCDR::to_wstring" : "ACE_InputCDR::to_string"
        : is_wide ? "Serializer::ToBoundedString<wchar_t>" : "Serializer::ToBoundedString<char>";
      const std::string face_suffix = is_face ? ".out()" : "";
      brType = is_face ? is_wide ? "FACE::WString_var" : "FACE::String_var"
        : is_wide ? "OPENDDS_WSTRING" : "OPENDDS_STRING";
      rhs = to_type + "(tmp" + face_suffix + ", " + bounded_arg(br) + ")";
    } else if (br_cls & CL_STRING) {
      const std::string nmspace = is_face ? "FACE::" : "CORBA::";
      brType = use_cxx11 ? std::string("std::") + (is_wide ? "w" : "") + "string"
        : nmspace + (is_wide ? "W" : "") + "String_var";
      rhs = use_cxx11 ? "tmp" : "tmp.out()";
    } else if (use_cxx11 && (br_cls & (CL_ARRAY | CL_SEQUENCE))) {  //array or seq C++11
      rhs = "IDL::DistinctType<" + brType + ", "
        + dds_generator::get_tag_name(brType)
        + ">(tmp)";
    } else if (br_cls & CL_ARRAY) { //array classic
      forany = "    " + brType + "_forany fa = tmp;\n";
      rhs = getWrapper("fa", br, WD_INPUT);
    } else { // anything else
      rhs = getWrapper("tmp", br, WD_INPUT);
    }

    if (*statementPrefix) {
      be_global->impl_ << statementPrefix;
    }
    be_global->impl_ <<
      "    " << brType << " tmp;\n" << forany <<
      "    if (strm >> " << rhs << ") {\n"
      "      uni." << name << (use_cxx11 ? "(std::move(tmp));\n" : "(tmp" + bound_string_suffix + ");\n") <<
      "      uni._d(disc);\n"
      "      return true;\n"
      "    }\n";

    if (be_global->try_construct(branch) == tryconstructfailaction_use_default) {
      be_global->impl_ <<
        type_to_default("        ", br, "uni." + name, branch->anonymous(), true) <<
        "        strm.set_construction_status(Serializer::ConstructionSuccessful);\n"
        "        return true;\n";
    } else if ((be_global->try_construct(branch) == tryconstructfailaction_trim) && (br_cls & CL_BOUNDED) &&
                (br_cls & (CL_STRING | CL_SEQUENCE))) {
      if (is_bound_string) {
        const std::string check_not_empty = "!tmp.empty()";
        const std::string get_length = use_cxx11 ? "tmp.length()" : "ACE_OS::strlen(tmp.c_str())";
        const std::string inout = use_cxx11 ? "" : ".inout()";
        const std::string strtype = br_cls & CL_WIDE ? "std::wstring" : "std::string";
        be_global->impl_ <<
          "        if (strm.get_construction_status() == Serializer::BoundConstructionFailure && " << check_not_empty << " && ("
                    << bounded_arg(br) << " < " << get_length << ")) {\n"
          "          " << strtype << " s = tmp.c_str();\n"
          "          s.resize(" << bounded_arg(br) << ");\n"
          "          uni." << name << "(s.c_str());\n"
          "          strm.set_construction_status(Serializer::ConstructionSuccessful);\n"
          "          return true;\n"
          "        } else {\n"
          "          strm.set_construction_status(Serializer::ElementConstructionFailure);\n"
          "          return false;\n"
          "        }\n";
      } else if (br_cls & CL_SEQUENCE) {
        be_global->impl_ <<
          "        if(strm.get_construction_status() == Serializer::ElementConstructionFailure) {\n"
          "          return false;\n"
          "        }\n"
          "        uni." << name << (use_cxx11 ? "(std::move(tmp));\n" : "(tmp);\n") <<
          "        uni._d(disc);\n"
          "        strm.set_construction_status(Serializer::ConstructionSuccessful);\n"
          "        return true;\n";
      }
    } else {
      //discard/default
      be_global->impl_ <<
        "        strm.set_construction_status(Serializer::ElementConstructionFailure);\n"
        "        return false;\n  ";
    }
  } else {
    const char* breakString = generateBreaks ? "    break;\n" : "";
    const std::string indent = "    ";
    Intro intro;
    std::ostringstream contents;
    if (commonFn2) {
      const OpenDDS::XTypes::MemberId id = be_global->get_id(branch);
      contents
        << commonFn2(indent, branch, name + (parens ? "()" : ""), branch->field_type(), "uni", false, intro, "")
        << indent << "if (!strm.write_parameter_id(" << id << ", size)) {\n"
        << indent << "  return false;\n"
        << indent << "}\n";
    }
    const std::string expr = commonFn(indent, branch,
      name + (parens ? "()" : ""), branch->field_type(),
      std::string(namePrefix) + "uni", false, intro, uni);
    if (*statementPrefix) {
      contents <<
        indent << statementPrefix << " " << expr << ";\n" <<
        (statementPrefix == std::string("return") ? "" : breakString);
    } else {
      contents << expr << breakString;
    }
    intro.join(be_global->impl_, indent);
    be_global->impl_ << contents.str();
  }
}

inline
bool generateSwitchBody(AST_Union*, CommonFn commonFn,
                        const std::vector<AST_UnionBranch*>& branches,
                        AST_Type* discriminator, const char* statementPrefix,
                        const char* namePrefix = "", const char* uni = "",
                        bool forceDisableDefault = false, bool parens = true,
                        bool breaks = true, CommonFn commonFn2 = 0)
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
    generateCaseBody(commonFn, commonFn2, branch, statementPrefix, namePrefix,
                     uni, breaks, parens);
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
bool generateSwitchForUnion(AST_Union* u, const char* switchExpr, CommonFn commonFn,
                            const std::vector<AST_UnionBranch*>& branches,
                            AST_Type* discriminator, const char* statementPrefix,
                            const char* namePrefix = "", const char* uni = "",
                            bool forceDisableDefault = false, bool parens = true,
                            bool breaks = true, CommonFn commonFn2 = 0)
{
  using namespace AstTypeClassification;
  AST_Type* dt = resolveActualType(discriminator);
  AST_PredefinedType* bt = dynamic_cast<AST_PredefinedType*>(dt);
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
      generateCaseBody(commonFn, commonFn2, true_branch ? true_branch : default_branch,
                       statementPrefix, namePrefix, uni, false, parens);
    }

    if (false_branch || (default_branch && true_branch)) {
      be_global->impl_ <<
        "  } else {\n";
      generateCaseBody(commonFn, commonFn2, false_branch ? false_branch : default_branch,
                       statementPrefix, namePrefix, uni, false, parens);
    }

    be_global->impl_ <<
      "  }\n";

    return !default_branch && bool(true_branch) != bool(false_branch);

  } else {
    be_global->impl_ <<
      "  switch (" << switchExpr << ") {\n";
    bool b(generateSwitchBody(u, commonFn, branches, discriminator,
                              statementPrefix, namePrefix, uni,
                              forceDisableDefault, parens, breaks,
                              commonFn2));
    be_global->impl_ <<
      "  }\n";
    return b;
  }
}

inline
std::string insert_cxx11_accessor_parens(
  const std::string& full_var_name_, bool is_union_member = false)
{
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  if (!use_cxx11 || is_union_member || full_var_name_.empty()) {
    return full_var_name_;
  }

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

enum FieldFilter {
  FieldFilter_All,
  FieldFilter_NestedKeyOnly,
  FieldFilter_KeyOnly
};

inline
AST_Field* get_struct_field(AST_Structure* struct_node, unsigned index)
{
  if (!struct_node || index >= struct_node->nfields()) {
    return 0;
  }
  AST_Field** field_ptrptr;
  struct_node->field(field_ptrptr, index);
  return field_ptrptr ? *field_ptrptr : 0;
}

inline
bool struct_has_explicit_keys(AST_Structure* node)
{
  for (unsigned i = 0; i < node->nfields(); ++i) {
    if (be_global->is_key(get_struct_field(node, i))) {
      return true;
    }
  }
  return false;
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

    explicit Iterator(AST_Structure* node = 0, unsigned pos = 0, bool explicit_keys_only = false)
    : node_(node)
    , pos_(pos)
    , explicit_keys_only_(explicit_keys_only)
    {
      validate_pos();
    }

    bool valid() const
    {
      return node_ && pos_ < node_->nfields();
    }

    bool check()
    {
      if (!valid()) {
        if (node_) {
          *this = Iterator();
        }
        return false;
      }
      return true;
    }

    void validate_pos()
    {
      for (; check() && explicit_keys_only_ && !be_global->is_key(**this); ++pos_) {
      }
    }

    unsigned pos() const
    {
      return pos_;
    }

    Iterator& operator++() // Prefix
    {
      ++pos_;
      validate_pos();
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
      return get_struct_field(node_, pos_);
    }

    bool operator==(const Iterator& other) const
    {
      return node_ == other.node_
        && pos_ == other.pos_
        && explicit_keys_only_ == other.explicit_keys_only_;
    }

    bool operator!=(const Iterator& other) const
    {
      return !(*this == other);
    }

  private:
    AST_Structure* node_;
    unsigned pos_;
    bool explicit_keys_only_;
  };

  explicit Fields(AST_Structure* node = 0, FieldFilter filter = FieldFilter_All)
  : node_(node)
  , explicit_keys_only_(explicit_keys_only(node, filter))
  {
  }

  static bool explicit_keys_only(AST_Structure* node, FieldFilter filter)
  {
    return filter == FieldFilter_KeyOnly ||
      (filter == FieldFilter_NestedKeyOnly && node && struct_has_explicit_keys(node));
  }

  AST_Structure* node() const
  {
    return node_;
  }

  Iterator begin() const
  {
    return Iterator(node_, 0, explicit_keys_only_);
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
  AST_Structure* const node_;
  const bool explicit_keys_only_;
};

inline
ACE_CDR::ULong array_element_count(AST_Array* arr)
{
  ACE_CDR::ULong count = 1;
  for (ACE_CDR::ULong i = 0; i < arr->n_dims(); ++i) {
    count *= arr->dims()[i]->ev()->u.ulval;
  }
  return count;
}

inline
ACE_CDR::ULong container_element_limit(AST_Type* type)
{
  AST_Type* const act = AstTypeClassification::resolveActualType(type);
  AST_Sequence* const seq = dynamic_cast<AST_Sequence*>(act);
  AST_Array* const arr = dynamic_cast<AST_Array*>(act);
  if (seq && !seq->unbounded()) {
    return seq->max_size()->ev()->u.ulval;
  } else if (arr) {
    return array_element_count(arr);
  }
  return 0;
}

inline
AST_Type* container_base_type(AST_Type* type)
{
  AST_Type* const act = AstTypeClassification::resolveActualType(type);
  AST_Sequence* const seq = dynamic_cast<AST_Sequence*>(act);
  AST_Array* const arr = dynamic_cast<AST_Array*>(act);
  if (seq) {
    return seq->base_type();
  } else if (arr) {
    return arr->base_type();
  }
  return 0;
}

/**
 * Returns true for a type if nested key serialization is different from
 * normal serialization.
 */
inline bool needs_nested_key_only(AST_Type* type)
{
  AST_Type* const non_aliased_type = type;

  using namespace AstTypeClassification;

  static std::vector<AST_Type*> type_stack;
  type = resolveActualType(type);
  // Check if we have encountered the same type recursively
  for (size_t i = 0; i < type_stack.size(); ++i) {
    if (type == type_stack[i]) {
      return true;
    }
  }
  type_stack.push_back(type);

  bool result = false;
  const std::string name = scoped(type->name());

  std::string template_name;
  if (be_global->special_serialization(non_aliased_type, template_name)) {
    result = false;
  } else {
    const Classification type_class = classify(type);
    if (type_class & CL_ARRAY) {
      result = needs_nested_key_only(dynamic_cast<AST_Array*>(type)->base_type());
    } else if (type_class & CL_SEQUENCE) {
      result = needs_nested_key_only(dynamic_cast<AST_Sequence*>(type)->base_type());
    } else if (type_class & CL_STRUCTURE) {
      AST_Structure* const struct_node = dynamic_cast<AST_Structure*>(type);
      // TODO(iguessthislldo): Possible optimization: If everything in a struct
      // was a key recursively, then we could return false.
      if (struct_has_explicit_keys(struct_node)) {
        result = true;
      } else {
        const Fields fields(struct_node);
        const Fields::Iterator fields_end = fields.end();
        for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
          if (needs_nested_key_only((*i)->field_type())) {
            result = true;
            break;
          }
        }
      }
    } else if (type_class & CL_UNION) {
      // A union will always be different as a key because it's just the
      // discriminator.
      result = true;
    }
  }
  type_stack.pop_back();
  return result;
}

inline bool needs_forany(AST_Type* type)
{
  using namespace AstTypeClassification;
  const Classification type_class = classify(resolveActualType(type));
  return be_global->language_mapping() != BE_GlobalData::LANGMAP_CXX11 &&
    type_class & CL_ARRAY;
}

inline bool needs_distinct_type(AST_Type* type)
{
  using namespace AstTypeClassification;
  const Classification type_class = classify(resolveActualType(type));
  return be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11 &&
    type_class & (CL_SEQUENCE | CL_ARRAY);
}

const char* const shift_out = "<< ";
const char* const shift_in = ">> ";

inline std::string strip_shift_op(const std::string& s)
{
  const size_t shift_len = 3;
  std::string rv = s;
  if (rv.size() > shift_len) {
    const std::string first3 = rv.substr(0, shift_len);
    if (first3 == shift_out || first3 == shift_in) {
      rv.erase(0, 3);
    }
  }
  return rv;
}

inline const char* get_shift_op(const std::string& s)
{
  const size_t shift_len = 3;
  if (s.size() > shift_len) {
    const std::string first3 = s.substr(0, shift_len);
    if (first3 == shift_in) {
      return shift_in;
    }
    if (first3 == shift_out) {
      return shift_out;
    }
  }
  return "";
}

inline std::string extensibility_kind(ExtensibilityKind ek)
{
  switch (ek) {
  case extensibilitykind_final:
    return "OpenDDS::DCPS::FINAL";
  case extensibilitykind_appendable:
    return "OpenDDS::DCPS::APPENDABLE";
  case extensibilitykind_mutable:
    return "OpenDDS::DCPS::MUTABLE";
  default:
    return "invalid";
  }
}

inline std::string type_kind(AST_Type* type)
{
  type = AstTypeClassification::resolveActualType(type);
  switch (type->node_type()) {
  case AST_Decl::NT_pre_defined: {
    AST_PredefinedType* pt_type = dynamic_cast<AST_PredefinedType*>(type);
    if (!pt_type) {
      return "XTypes::TK_NONE";
    }
    switch (pt_type->pt()) {
    case AST_PredefinedType::PT_long:
      return "XTypes::TK_INT32";
    case AST_PredefinedType::PT_ulong:
      return "XTypes::TK_UINT32";
    case AST_PredefinedType::PT_longlong:
      return "XTypes::TK_INT64";
    case AST_PredefinedType::PT_ulonglong:
      return "XTypes::TK_UINT64";
    case AST_PredefinedType::PT_short:
      return "XTypes::TK_INT16";
    case AST_PredefinedType::PT_ushort:
      return "XTypes::TK_UINT16";
    case AST_PredefinedType::PT_float:
      return "XTypes::TK_FLOAT32";
    case AST_PredefinedType::PT_double:
      return "XTypes::TK_FLOAT64";
    case AST_PredefinedType::PT_longdouble:
      return "XTypes::TK_FLOAT128";
    case AST_PredefinedType::PT_char:
      return "XTypes::TK_CHAR8";
    case AST_PredefinedType::PT_wchar:
      return "XTypes::TK_CHAR16";
    case AST_PredefinedType::PT_boolean:
      return "XTypes::TK_BOOLEAN";
    case AST_PredefinedType::PT_octet:
      return "XTypes::TK_BYTE";
    case AST_PredefinedType::PT_int8:
      return "XTypes::TK_INT8";
    case AST_PredefinedType::PT_uint8:
      return "XTypes::TK_UINT8";
    default:
      return "XTypes::TK_NONE";
    }
  }
  case AST_Decl::NT_string:
    return "XTypes::TK_STRING8";
  case AST_Decl::NT_wstring:
    return "XTypes::TK_STRING16";
  case AST_Decl::NT_array:
    return "XTypes::TK_ARRAY";
  case AST_Decl::NT_sequence:
    return "XTypes::TK_SEQUENCE";
  case AST_Decl::NT_union:
    return "XTypes::TK_UNION";
  case AST_Decl::NT_struct:
    return "XTypes::TK_STRUCTURE";
  case AST_Decl::NT_enum:
    return "XTypes::TK_ENUM";
  default:
    return "XTypes::TK_NONE";
  }
}

inline
FieldFilter nested(FieldFilter filter_kind)
{
  return filter_kind == FieldFilter_KeyOnly ? FieldFilter_NestedKeyOnly : filter_kind;
}

inline
bool has_discriminator(AST_Union* u, FieldFilter filter_kind)
{
  return be_global->union_discriminator_is_key(u)
    || filter_kind == FieldFilter_NestedKeyOnly
    || filter_kind == FieldFilter_All;
}

// TODO: Add more fine-grained control of "const" string for the wrapper type and wrapped type.
// Currently, there is a single bool to control both; that is, either both are "const" or
// none is "const". But sometimes, we want something like "const KeyOnly<SampleType>&", and
// not "const KeyOnly<const SampleType>&" or "KeyOnly<SampleType>&".

/// Handling wrapping and unwrapping references in the wrapper types:
/// NestedKeyOnly, KeyOnly, IDL::DistinctType, and *_forany.
struct RefWrapper {
  const bool cpp11_;
  AST_Type* const type_;
  const std::string type_name_;
  const std::string to_wrap_;
  const char* const shift_op_;
  const std::string fieldref_;
  const std::string local_;
  bool is_const_;
  FieldFilter field_filter_;
  bool nested_key_only_;
  bool classic_array_copy_;
  bool dynamic_data_adapter_;
  std::string classic_array_copy_var_;
  AST_Typedef* typedef_node_;

  RefWrapper(AST_Type* type, const std::string& type_name,
    const std::string& to_wrap, bool is_const = true)
    : cpp11_(be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11)
    , type_(type)
    , type_name_(type_name)
    , to_wrap_(strip_shift_op(to_wrap))
    , shift_op_(get_shift_op(to_wrap))
    , is_const_(is_const)
    , field_filter_(FieldFilter_All)
    , nested_key_only_(false)
    , classic_array_copy_(false)
    , dynamic_data_adapter_(false)
    , typedef_node_(0)
    , done_(false)
    , needs_dda_tag_(false)
  {
  }

  RefWrapper(AST_Type* type, const std::string& type_name,
    const std::string& fieldref, const std::string& local, bool is_const = true)
    : cpp11_(be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11)
    , type_(type)
    , type_name_(type_name)
    , shift_op_("")
    , fieldref_(strip_shift_op(fieldref))
    , local_(local)
    , is_const_(is_const)
    , field_filter_(FieldFilter_All)
    , nested_key_only_(false)
    , classic_array_copy_(false)
    , dynamic_data_adapter_(false)
    , typedef_node_(0)
    , done_(false)
    , needs_dda_tag_(false)
  {
  }

  RefWrapper& done(Intro* intro = 0)
  {
    ACE_ASSERT(!done_);

    if (is_const_ && !std::strcmp(shift_op_, shift_in)) {
      is_const_ = false;
    }
    const std::string const_str = is_const_ ? "const " : "";
    const bool forany = classic_array_copy_ || needs_forany(type_);
    const bool distinct_type = needs_distinct_type(type_);
    needs_dda_tag_ = dynamic_data_adapter_ && (forany || distinct_type);
    // If field_filter_ is set, this object is being used for vwrite or vread generator.
    nested_key_only_ = field_filter_ == FieldFilter_NestedKeyOnly ||
      (nested_key_only_ && needs_nested_key_only(typedef_node_ ? typedef_node_ : type_));
    wrapped_type_name_ = type_name_;
    bool by_ref = true;

    if (to_wrap_.size()) {
      ref_ = to_wrap_;
    } else {
      ref_ = fieldref_;
      if (local_.size()) {
        ref_ += '.' + local_;
      }
    }

    if (forany && !dynamic_data_adapter_) {
      const std::string forany_type = type_name_ + "_forany";
      if (classic_array_copy_) {
        const std::string var_name = dds_generator::valid_var_name(ref_) + "_tmp_var";
        classic_array_copy_var_ = var_name;
        if (intro) {
          intro->insert(type_name_ + "_var " + var_name + "= " + type_name_ + "_alloc();");
        }
        ref_ = var_name;
      }
      const std::string var_name = dds_generator::valid_var_name(ref_) + "_forany";
      wrapped_type_name_ = forany_type;
      if (intro) {
        std::string line = forany_type + " " + var_name;
        if (classic_array_copy_) {
          line += " = " + ref_ + ".inout();";
        } else {
          line += "(const_cast<" + type_name_ + "_slice*>(" + ref_ + "));";
        }
        intro->insert(line);
      }
      ref_ = var_name;
    }

    if (field_filter_ == FieldFilter_KeyOnly) {
      wrapped_type_name_ = std::string("KeyOnly<") + const_str + wrapped_type_name_ + ">";
    }

    if (nested_key_only_) {
      wrapped_type_name_ = std::string("NestedKeyOnly<") + const_str + wrapped_type_name_ + ">";
      value_access_post_ = ".value" + value_access_post_;
      const std::string nko_arg = "(" + ref_ + ")";
      if (is_const_) {
        ref_ = wrapped_type_name_ + nko_arg;
      } else {
        ref_ = dds_generator::valid_var_name(ref_) + "_nested_key_only";
        if (intro) {
          intro->insert(wrapped_type_name_ + " " + ref_ + nko_arg + ";");
        }
      }
    }

    if (distinct_type && !dynamic_data_adapter_) {
      wrapped_type_name_ =
        std::string("IDL::DistinctType<") + const_str + wrapped_type_name_ +
        ", " + get_tag_name_i() + ">";
      value_access_pre_ += "(*";
      value_access_post_ = ".val_)" + value_access_post_;
      const std::string idt_arg = "(" + ref_ + ")";
      if (is_const_) {
        ref_ = wrapped_type_name_ + idt_arg;
      } else {
        ref_ = dds_generator::valid_var_name(ref_) + "_distinct_type";
        if (intro) {
          intro->insert(wrapped_type_name_ + " " + ref_ + idt_arg + ";");
        }
      }
      by_ref = false;
    }

    wrapped_type_name_ = const_str + wrapped_type_name_ + (by_ref ? "&" : "");
    done_ = true;
    return *this;
  }

  std::string ref() const
  {
    ACE_ASSERT(done_);
    return ref_;
  }

  std::string wrapped_type_name() const
  {
    ACE_ASSERT(done_);
    return wrapped_type_name_;
  }

  bool needs_dda_tag() const
  {
    ACE_ASSERT(done_);
    return needs_dda_tag_;
  }

  void generate_tag() const
  {
    if (cpp11_ || needs_dda_tag_) {
      be_global->header_ << "struct " << get_tag_name() << " {};\n\n";
    }
  }

  std::string get_tag_name() const
  {
    ACE_ASSERT(done_);
    return get_tag_name_i();
  }

  std::string get_var_name(const std::string& var_name) const
  {
    return var_name.size() ? var_name : to_wrap_;
  }

  std::string value_access(const std::string& var_name = "") const
  {
    return value_access_pre_ + get_var_name(var_name) + value_access_post_;
  }

  std::string seq_check_empty() const
  {
    return value_access() + (cpp11_ ? ".empty()" : ".length() == 0");
  }

  std::string seq_get_length() const
  {
    const std::string value = value_access();
    return cpp11_ ? "static_cast<uint32_t>(" + value + ".size())" : value + ".length()";
  }

  std::string seq_resize(const std::string& new_size) const
  {
    const std::string value = value_access();
    return value + (cpp11_ ? ".resize" : ".length") + "(" + new_size + ");\n";
  }

  std::string seq_get_buffer() const
  {
    return value_access() + (cpp11_ ? ".data()" : ".get_buffer()");
  }

  std::string flat_collection_access(std::string index) const
  {
    AST_Array* const array_node = dynamic_cast<AST_Array*>(type_);
    std::string ref;
    if (array_node) {
      ref = "(&" + value_access();
      for (ACE_CDR::ULong dim = array_node->n_dims(); dim; --dim) {
        ref += "[0]";
      }
      ref += ")";
    } else {
      ref = value_access();
    }
    return ref += "[" + index + "]";
  }

  std::string stream() const
  {
    return shift_op_ + ref();
  }

  std::string classic_array_copy() const
  {
    return type_name_ + "_copy(" + to_wrap_ + ", " + classic_array_copy_var_ + ".in());";
  }

private:
  bool done_;
  std::string wrapped_type_name_;
  std::string ref_;
  std::string value_access_pre_;
  std::string value_access_post_;
  bool needs_dda_tag_;

  std::string get_tag_name_i() const
  {
    std::string qualifier;
    if (nested_key_only_) {
      qualifier = "_nested_key_only";
    } else if (needs_dda_tag_) {
      qualifier = "_dda";
    }
    return dds_generator::get_tag_name(type_name_, qualifier);
  }
};

inline
std::string key_only_type_name(AST_Type* type, const std::string& type_name,
                               FieldFilter field_filter, bool writing)
{
  RefWrapper wrapper(type, type_name, "", writing ? true : false);
  wrapper.field_filter_ = field_filter;
  const bool has_wrapper = field_filter != FieldFilter_All;
  return (has_wrapper && !writing ? "const " : "") + wrapper.done().wrapped_type_name();
}

#endif
