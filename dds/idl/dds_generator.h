/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef dds_generator_H
#define dds_generator_H

#include "be_extern.h"
#include "be_util.h"
#include "../DCPS/RestoreOutputStreamState.h"

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

  static std::string get_tag_name(const std::string& base_name, bool nested_key_only = false);

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

const char* const namespace_guard_start =
  "OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL\n"
  "namespace OpenDDS { namespace DCPS {\n\n";
const char* const namespace_guard_end =
  "} }\n"
  "OPENDDS_END_VERSIONED_NAMESPACE_DECL\n\n";

struct NamespaceGuard {
  const bool enabled_;

  NamespaceGuard(bool enabled = true)
  : enabled_(enabled)
  {
    if (enabled_) {
      be_global->header_ << namespace_guard_start;
      be_global->impl_ << namespace_guard_start;
    }
  }
  ~NamespaceGuard()
  {
    if (enabled_) {
      be_global->header_ << namespace_guard_end;
      be_global->impl_ << namespace_guard_end;
    }
  }
};

struct ScopedNamespaceGuard  {
  ScopedNamespaceGuard(UTL_ScopedName* name, std::ostream& os,
                       const char* keyword = "namespace")
    : os_(os)
    , semi_()
    , n_(0)
  {
    const bool idl = !std::strcmp(keyword, "module");
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
    for (int i = 0; i < n_; ++i) os_ << '}' << semi_ << '\n';
  }

  std::ostream& os_;
  std::string semi_;
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
  , extra_newline_(false)
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
    type = AstTypeClassification::resolveActualType(type);
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
    case AST_PredefinedType::PT_uint8:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_uint8(" : "ACE_InputCDR::to_uint8(";
    case AST_PredefinedType::PT_int8:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_int8(" : "ACE_InputCDR::to_int8(";
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
    return "ACE_CDR::ULong";
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
    case AST_PredefinedType::PT_int8:
      size = 1;
      return "ACE_CDR::Int8";
    case AST_PredefinedType::PT_uint8:
      size = 1;
      return "ACE_CDR::UInt8";
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

template <typename IntType>
std::ostream& signed_int_helper(std::ostream& o, IntType value, IntType min)
{
  /*
   * It seems that in C/C++ the minus sign and the bare number are parsed
   * separately for negative integer literals. This can cause compilers
   * to complain when using the minimum value of a signed integer because
   * the number without the minus sign is 1 past the max signed value.
   *
   * https://stackoverflow.com/questions/65007935
   *
   * Apparently the workaround is to write it as `VALUE_PLUS_ONE - 1`.
   */
  const bool min_value = value == min;
  if (min_value) ++value;
  o << value;
  if (min_value) o << " - 1";
  return o;
}

inline
std::ostream& hex_value(std::ostream& o, unsigned value, size_t bytes)
{
  OpenDDS::DCPS::RestoreOutputStreamState ross(o);
  o << std::hex << std::setw(bytes * 2) << std::setfill('0') << value;
  return o;
}

template <typename CharType>
unsigned char_value(CharType value)
{
  return value;
}

#if CHAR_MIN < 0
/*
 * If char is signed, then it needs to be reinterpreted as unsigned char or
 * else static casting '\xff' to a 32-bit unsigned int would result in
 * 0xffffffff because those are both the signed 2's complement forms of -1.
 */
template <>
inline unsigned char_value<char>(char value)
{
  return reinterpret_cast<unsigned char&>(value);
}
#endif

template <typename CharType>
std::ostream& char_helper(std::ostream& o, CharType value)
{
  switch (value) {
  case '\'':
  case '\"':
  case '\\':
  case '\?':
    return o << '\\' << static_cast<char>(value);
  case '\n':
    return o << "\\n";
  case '\t':
    return o << "\\t";
  case '\v':
    return o << "\\v";
  case '\b':
    return o << "\\b";
  case '\r':
    return o << "\\r";
  case '\f':
    return o << "\\f";
  case '\a':
    return o << "\\a";
  }
  if (isprint(char_value(value))) {
    return o << static_cast<char>(value);
  }
  return hex_value(o << "\\x", char_value<CharType>(value), sizeof(CharType) == 1 ? 1 : 2);
}

inline
std::ostream& operator<<(std::ostream& o,
                         const AST_Expression::AST_ExprValue& ev)
{
  OpenDDS::DCPS::RestoreOutputStreamState ross(o);
  switch (ev.et) {
  case AST_Expression::EV_octet:
    return hex_value(o << "0x", static_cast<int>(ev.u.oval), 1);
  case AST_Expression::EV_int8:
    return o << static_cast<short>(ev.u.int8val);
  case AST_Expression::EV_uint8:
    return o << static_cast<unsigned short>(ev.u.uint8val);
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
#ifdef ACE_HAS_CDR_FIXED
  case AST_Expression::EV_fixed: {
    char buf[ACE_CDR::Fixed::MAX_STRING_SIZE];
    ev.u.fixedval.to_string(buf, sizeof buf);
    return o << "\"" << buf << "\"";
  }
#endif
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
  const std::string& name, bool is_anonymous = false, bool is_union = false);

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
  case AST_PredefinedType::PT_int8:
  case AST_PredefinedType::PT_uint8:
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

typedef std::string (*CommonFn)(
  const std::string& indent,
  const std::string& name, AST_Type* type,
  const std::string& prefix, bool wrap_nested_key_only, Intro& intro,
  const std::string&, bool printing);

inline
void generateCaseBody(
  CommonFn commonFn, CommonFn commonFn2,
  AST_UnionBranch* branch,
  const char* statementPrefix, const char* namePrefix, const char* uni, bool generateBreaks, bool parens,
  bool printing = false)
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
        + dds_generator::get_tag_name(dds_generator::scoped_helper(branch->field_type()->name(), "::"))
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
          "          " << strtype << " s = tmp;\n"
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
        << commonFn2(indent, name + (parens ? "()" : ""), branch->field_type(), "uni", false, intro, "", false)
        << indent << "if (!strm.write_parameter_id(" << id << ", size)) {\n"
        << indent << "  return false;\n"
        << indent << "}\n";
    }
    const std::string expr = commonFn(indent,
      name + (parens ? "()" : ""), branch->field_type(),
      std::string(namePrefix) + "uni", false, intro, uni, printing);
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
                     uni, breaks, parens, false);
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

#endif
