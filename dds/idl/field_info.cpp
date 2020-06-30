/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "field_info.h"
#include <sstream>

using namespace AstTypeClassification;

FieldInfo::EleLen::EleLen(FieldInfo& af) : ele_(af.as_base_), len_(af.n_elems_)
{
}

bool FieldInfo::EleLen::Cmp::operator()(const EleLen& a, const EleLen& b) const
{
  return a.ele_ != b.ele_ || a.len_ != b.len_;
}

const std::string FieldInfo::scope_op = "::";

std::string FieldInfo::get_type_name(AST_Type& field)
{
  std::string n = scoped(field.name());
  if (!field.anonymous()) {
    return n;
  }
  n = n.substr(0, n.rfind(scope_op) + 2) + "_" + field.local_name()->get_string();
  return (field.node_type() == AST_Decl::NT_sequence) ? (n + "_seq") : n;
}

// for anonymous types
FieldInfo::FieldInfo(AST_Field& field) :
  type_(field.field_type()),
  name_(field.local_name()->get_string())
{
  init();
}

FieldInfo::FieldInfo(UTL_ScopedName* sn, AST_Type* base) :
  type_(base),
  scoped_type_(scoped(sn))
{
  const bool use_cxx11 = (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11);
  underscored_ = use_cxx11 ? dds_generator::scoped_helper(sn, "_") : "";
  init();
}

void FieldInfo::init()
{
  act_ = resolveActualType(type_);
  cls_ = classify(act_);
  arr_ = AST_Array::narrow_from_decl(type_);
  seq_ = AST_Sequence::narrow_from_decl(type_);
  as_base_ = arr_ ? arr_->base_type() : (seq_ ? seq_->base_type() : 0);
  as_act_ = as_base_ ? resolveActualType(as_base_) : 0;
  as_cls_ = as_act_ ? classify(as_act_) : CL_UNKNOWN;

  if (type_->anonymous() && as_base_) {
    scoped_type_ = scoped(type_->name());
    std::size_t i = scoped_type_.rfind(scope_op);
    struct_name_ = scoped_type_.substr(0, i);
    if (!name_.empty()) {
      type_name_ = "_" + name_;
      if (seq_) { type_name_ += "_seq"; }
      scoped_type_ = struct_name_ + "::" + type_name_;
    } else {
      type_name_ = scoped_type_.substr(i + 2);
    }
  } else {
    if (scoped_type_.empty()) {
      scoped_type_ = scoped(type_->name());
    }
    //name_
    type_name_ = scoped_type_;
    //struct_name_
  }

  set_element();
  n_elems_ = 1;
  if (arr_) {
    for (size_t i = 0; i < arr_->n_dims(); ++i) {
      n_elems_ *= arr_->dims()[i]->ev()->u.ulval;
    }
    std::ostringstream os;
    os << n_elems_;
    length_ = os.str();
    arg_ = "arr";
  } else if (seq_) {
    n_elems_ = !seq_->unbounded() ? seq_->max_size()->ev()->u.ulval : 0;
    length_ = "length";
    arg_ = "seq";
  }

  if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
    be_global->header_ << "struct " << underscored_ << "_tag {};\n\n";
    unwrap_ = scoped_type_ + "& " + arg_ + " = wrap;\n  ACE_UNUSED_ARG(" + arg_ + ");\n";
    const_unwrap_ = "  const " + unwrap_;
    unwrap_ = "  " + unwrap_;
    arg_ = "wrap";
    ref_       = "IDL::DistinctType<"       + scoped_type_ + ", " + underscored_ + "_tag>";
    const_ref_ = "IDL::DistinctType<const " + scoped_type_ + ", " + underscored_ + "_tag>";
  } else {
    ref_ = scoped_type_ + (arr_ ? "_forany&" : "&");
    const_ref_ = "const " + ref_;
  }
  ptr_ = scoped_type_ + (arr_ ? "_forany*" : "*");
}

void FieldInfo::set_element()
{
  elem_sz_ = 0;
  if (as_base_) {
    if (as_cls_ & CL_ENUM) {
      elem_sz_ = 4; elem_ = "ACE_CDR::ULong"; return;
    } else if (as_cls_ & CL_STRING) {
      elem_sz_ = 4; elem_ = string_type(as_cls_); return; // encoding of str length is 4 bytes
    } else if (as_cls_ & CL_PRIMITIVE) {
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(as_act_);
      switch (p->pt()) {
      case AST_PredefinedType::PT_long: elem_sz_ = 4; elem_ = "ACE_CDR::Long"; return;
      case AST_PredefinedType::PT_ulong: elem_sz_ = 4; elem_ = "ACE_CDR::ULong"; return;
      case AST_PredefinedType::PT_longlong: elem_sz_ = 8; elem_ = "ACE_CDR::LongLong"; return;
      case AST_PredefinedType::PT_ulonglong: elem_sz_ = 8; elem_ = "ACE_CDR::ULongLong"; return;
      case AST_PredefinedType::PT_short: elem_sz_ = 2; elem_ = "ACE_CDR::Short"; return;
      case AST_PredefinedType::PT_ushort: elem_sz_ = 2; elem_ = "ACE_CDR::UShort"; return;
      case AST_PredefinedType::PT_float: elem_sz_ = 4; elem_ = "ACE_CDR::Float"; return;
      case AST_PredefinedType::PT_double: elem_sz_ = 8; elem_ = "ACE_CDR::Double"; return;
      case AST_PredefinedType::PT_longdouble: elem_sz_ = 16; elem_ = "ACE_CDR::LongDouble"; return;
      case AST_PredefinedType::PT_char: elem_sz_ = 1; elem_ = "ACE_CDR::Char"; return;
      case AST_PredefinedType::PT_wchar: elem_sz_ = 1; elem_ = "ACE_CDR::WChar"; return; // encoding of wchar length is 1 byte
      case AST_PredefinedType::PT_boolean: elem_sz_ = 1; elem_ = "ACE_CDR::Boolean"; return;
      case AST_PredefinedType::PT_octet: elem_sz_ = 1; elem_ = "ACE_CDR::Octet"; return;
      default: break;
      }
    }
    elem_ = scoped(as_act_->name());
  }
}

std::string FieldInfo::string_type(Classification c)
{
  return be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11 ?
    ((c & CL_WIDE) ? "std::wstring" : "std::string") :
    (c & CL_WIDE) ? "TAO::WString_Manager" : "TAO::String_Manager";
}

std::string FieldInfo::to_cxx_type(AST_Type* type, std::size_t& size)
{
  const Classification cls = classify(type);
  if (cls & CL_ENUM) { size = 4; return "ACE_CDR::ULong"; }
  if (cls & CL_STRING) { size = 4; return string_type(cls); } // encoding of str length is 4 bytes
  if (cls & CL_PRIMITIVE) {
    type = resolveActualType(type);
    AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
    switch (p->pt()) {
    case AST_PredefinedType::PT_long: size = 4; return "ACE_CDR::Long";
    case AST_PredefinedType::PT_ulong: size = 4; return "ACE_CDR::ULong";
    case AST_PredefinedType::PT_longlong: size = 8; return "ACE_CDR::LongLong";
    case AST_PredefinedType::PT_ulonglong: size = 8; return "ACE_CDR::ULongLong";
    case AST_PredefinedType::PT_short: size = 2; return "ACE_CDR::Short";
    case AST_PredefinedType::PT_ushort: size = 2; return "ACE_CDR::UShort";
    case AST_PredefinedType::PT_float: size = 4; return "ACE_CDR::Float";
    case AST_PredefinedType::PT_double: size = 8; return "ACE_CDR::Double";
    case AST_PredefinedType::PT_longdouble: size = 16; return "ACE_CDR::LongDouble";
    case AST_PredefinedType::PT_char: size = 1; return "ACE_CDR::Char";
    case AST_PredefinedType::PT_wchar: size = 1; return "ACE_CDR::WChar"; // encoding of wchar length is 1 byte
    case AST_PredefinedType::PT_boolean: size = 1; return "ACE_CDR::Boolean";
    case AST_PredefinedType::PT_octet: size = 1; return "ACE_CDR::Octet";
    default: break;
    }
  }
  return scoped(type->name());
}
