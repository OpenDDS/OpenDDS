/*
 * $Id$
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

#include "ast.h"
#include "ast_component_fwd.h"
#include "ast_eventtype_fwd.h"
#include "ast_structure_fwd.h"
#include "ast_union_fwd.h"
#include "ast_valuetype_fwd.h"

#include <string>
#include <vector>

class dds_generator {
public:
  virtual ~dds_generator() = 0;

  virtual bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                         AST_Expression::ExprType type,
                         AST_Expression::AST_ExprValue* value) = 0;

  virtual bool gen_enum(UTL_ScopedName* name,
                        const std::vector<AST_EnumVal*>& contents,
                        const char* repoid) = 0;

  virtual bool gen_struct(UTL_ScopedName* name,
                          const std::vector<AST_Field*>& fields,
                          const char* repoid) = 0;

  virtual bool gen_typedef(UTL_ScopedName* name, AST_Type* base,
                           const char* repoid) = 0;

  virtual bool gen_interf(UTL_ScopedName* name, bool local,
                          const std::vector<AST_Interface*>& inherits,
                          const std::vector<AST_Interface*>& inherits_flat,
                          const std::vector<AST_Attribute*>& attrs,
                          const std::vector<AST_Operation*>& ops,
                          const char* repoid) = 0;

  virtual bool gen_interf_fwd(UTL_ScopedName* name) = 0;

  virtual bool gen_native(UTL_ScopedName* name, const char* repoid) = 0;

  virtual bool gen_union(UTL_ScopedName* name,
                         const std::vector<AST_UnionBranch*>& branches,
                         AST_Type* discriminator,
                         const char* repoid) = 0;

  static std::string scoped_helper(UTL_ScopedName* sn, const char* sep);
};

class composite_generator : public dds_generator {
public:
  bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                 AST_Expression::ExprType type,
                 AST_Expression::AST_ExprValue* value);

  bool gen_enum(UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields, const char* repoid);

  bool gen_typedef(UTL_ScopedName* name, AST_Type* base, const char* repoid);

  bool gen_interf(UTL_ScopedName* name, bool local,
                  const std::vector<AST_Interface*>& inherits,
                  const std::vector<AST_Interface*>& inherits_flat,
                  const std::vector<AST_Attribute*>& attrs,
                  const std::vector<AST_Operation*>& ops, const char* repoid);

  bool gen_interf_fwd(UTL_ScopedName* name);

  bool gen_native(UTL_ScopedName* name, const char* repoid);

  bool gen_union(UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* discriminator,
                 const char* repoid);

  template <typename InputIterator>
  composite_generator(InputIterator begin, InputIterator end)
  : components_(begin, end) {}

private:
  std::vector<dds_generator*> components_;
};

// common utilities for all "generator" derived classes

struct NamespaceGuard {
  NamespaceGuard()
  {
    be_global->header_ << "namespace OpenDDS { namespace DCPS {\n\n";
    be_global->impl_ << "namespace OpenDDS { namespace DCPS {\n\n";
  }
  ~NamespaceGuard()
  {
    be_global->header_ << "}  }\n\n";
    be_global->impl_ << "}  }\n\n";
  }
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

namespace AstTypeClassification {
  inline void resolveActualType(AST_Type*& element)
  {
    if (element->node_type() == AST_Decl::NT_typedef) {
      AST_Typedef* td = AST_Typedef::narrow_from_decl(element);
      element = td->primitive_base_type();
    }

    switch(element->node_type()) {
    case AST_Decl::NT_interface_fwd:
    {
      AST_InterfaceFwd* td = AST_InterfaceFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_valuetype_fwd:
    {
      AST_ValueTypeFwd* td = AST_ValueTypeFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_union_fwd:
    {
      AST_UnionFwd* td = AST_UnionFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_struct_fwd:
    {
      AST_StructureFwd* td = AST_StructureFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_component_fwd:
    {
      AST_ComponentFwd* td = AST_ComponentFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_eventtype_fwd:
    {
      AST_EventTypeFwd* td = AST_EventTypeFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    default :
      break;
    }
  }

  typedef size_t Classification;
  const Classification CL_UNKNOWN = 0, CL_SCALAR = 1, CL_PRIMITIVE = 2,
    CL_STRUCTURE = 4, CL_STRING = 8, CL_ENUM = 16, CL_UNION = 32, CL_ARRAY = 64,
    CL_SEQUENCE = 128, CL_WIDE = 256, CL_BOUNDED = 512, CL_INTERFACE = 1024;

  inline Classification classify(AST_Type* type)
  {
    resolveActualType(type);
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
    default:
      return CL_UNKNOWN;
    }
  }
}

struct NestedForLoops {
  NestedForLoops(const char* type, const char* prefix, AST_Array* arr,
                 std::string& indent)
    : n_(arr->n_dims()), indent_(indent)
  {
    std::ostringstream index_oss;
    for (size_t i = 0; i < n_; ++i) {
      be_global->impl_ <<
        indent << "for (" << type << ' ' << prefix << i << " = 0; " <<
        prefix << i << " < " << arr->dims()[i]->ev()->u.ulval << "; ++" <<
        prefix << i << ") {\n";
      indent += "  ";
      index_oss << "[i" << i << "]";
    }
    index_ = index_oss.str();
  }

  ~NestedForLoops()
  {
    for (size_t i = 0; i < n_; ++i) {
      indent_.resize(indent_.size() - 2);
      be_global->impl_ << indent_ << "}\n";
    }
  }

  size_t n_;
  std::string& indent_;
  std::string index_;
};

enum WrapDirection {WD_OUTPUT, WD_INPUT};

inline
std::string wrapPrefix(AST_Type* type, WrapDirection wd)
{
  switch (type->node_type()) {
  case AST_Decl::NT_pre_defined:
    {
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
  std::string pre = wrapPrefix(type, wd);
  return (pre.empty()) ? name : (pre + name + ')');
}

inline
std::string getEnumLabel(AST_Expression* label_val, AST_Type* disc)
{
  std::string e = scoped(disc->name()),
    label = label_val->n()->last_component()->get_string();
  const size_t colon = e.rfind("::");
  if (colon == std::string::npos) {
    return label;
  }
  return e.replace(colon + 2, std::string::npos, label);
}

struct RestoreOutputStreamState {
  explicit RestoreOutputStreamState(std::ostream& o)
    : os_(o), state_(o.flags()) {}
  ~RestoreOutputStreamState() {
    os_.flags(state_);
  }
  std::ostream& os_;
  std::ios_base::fmtflags state_;
};

inline
std::ostream& operator<<(std::ostream& o, const AST_Expression& e)
{
  // TAO_IDL_FE interfaces are not const-correct
  AST_Expression& e_nonconst = const_cast<AST_Expression&>(e);
  const AST_Expression::AST_ExprValue& ev = *e_nonconst.ev();
  RestoreOutputStreamState ross(o);
  switch (ev.et) {
  case AST_Expression::EV_short:
    return o << ev.u.sval;
  case AST_Expression::EV_ushort:
    return o << ev.u.usval;
  case AST_Expression::EV_long:
    return o << ev.u.lval;
  case AST_Expression::EV_ulong:
    return o << ev.u.ulval;
  case AST_Expression::EV_longlong:
    return o << ev.u.llval << "LL";
  case AST_Expression::EV_ulonglong:
    return o << ev.u.ullval << "ULL";
  case AST_Expression::EV_char:
    return o << '\'' << ev.u.cval << '\'';
  case AST_Expression::EV_bool:
    return o << std::boolalpha << static_cast<bool>(ev.u.bval);
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
      be_global->impl_ << "  default:\n";
      has_default = true;
    } else if (discriminator->node_type() == AST_Decl::NT_enum) {
      be_global->impl_ << "  case "
        << getEnumLabel(label->label_val(), discriminator) << ":\n";
    } else {
      be_global->impl_ << "  case " << *label->label_val() << ":\n";
    }
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

#endif
