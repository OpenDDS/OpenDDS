/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "anonymous.h"

using namespace AstTypeClassification;

Field::SeqLen::SeqLen(Field& af) : seq_(af.ast_elem_), len_(0)
{
  if (!af.seq_->unbounded()) {
    len_ = af.seq_->max_size()->ev()->u.ulval;
  }
}

bool Field::SeqLen::Cmp::operator()(const SeqLen& a, const SeqLen& b) const
{
  return a.seq_ != b.seq_ || a.len_ != b.len_;
}

bool Field::is_anonymous_array(AST_Type& field)
{
  return field.node_type() == AST_Decl::NT_array;
}

bool Field::is_anonymous_sequence(AST_Type& field)
{
  return field.node_type() == AST_Decl::NT_sequence;
}

bool Field::is_anonymous_type(AST_Type& field)
{
  return is_anonymous_array(field) || is_anonymous_sequence(field);
}

std::string Field::get_type_name(AST_Type& field)
{
  std::string n = scoped(field.name());
  if (!is_anonymous_type(field)) {
    return n;
  }
  const std::string scope = n.substr(0, n.find("::") + 2);
  const std::string name = field.local_name()->get_string();
  n = scope + "_" + name;
  if (is_anonymous_sequence(field)) {
    return n + "_seq";
  }
  return n;
}

// for anonymous types
Field::Field(AST_Field& field) :
  ast_type_(field.field_type()),
  arr_(AST_Array::narrow_from_decl(ast_type_)),
  seq_(AST_Sequence::narrow_from_decl(ast_type_)),
  ast_elem_(arr_ ? arr_->base_type() : (seq_ ? seq_->base_type() : nullptr)),
  name_(field.local_name()->get_string()),
  cls_(CL_UNKNOWN), elem_sz_(0), n_elems_(1)
{
  type_ = ast_elem_ ? ("_" + name_) : name_;
  if (seq_) { type_ += "_seq"; }

  const std::string ftn = scoped(ast_type_->name());
  struct_name_ = ftn.substr(0, ftn.find("::"));
  scoped_type_ = struct_name_ + "::" + type_;
  //const bool use_cxx11 = (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11);
  //underscores_ = use_cxx11 ? dds_generator::scoped_helper(sn, "_") : "";
  init();
}

Field::Field(UTL_ScopedName* sn, AST_Type* base) :
  ast_type_(base),
  arr_(AST_Array::narrow_from_decl(ast_type_)),
  seq_(AST_Sequence::narrow_from_decl(ast_type_)),
  ast_elem_(arr_ ? arr_->base_type() : (seq_ ? seq_->base_type() : nullptr)),
  scoped_type_(scoped(sn)),
  cls_(CL_UNKNOWN), elem_sz_(0), n_elems_(1)
{
  const bool use_cxx11 = (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11);
  underscores_ = use_cxx11 ? dds_generator::scoped_helper(sn, "_") : "";
  init();
}

void Field::init()
{
  set_element();
  if (arr_) {
    for (size_t i = 0; i < arr_->n_dims(); ++i) {
      n_elems_ *= arr_->dims()[i]->ev()->u.ulval;
    }
    length_ = std::to_string(n_elems_);
    arg_ = "arr";
  } else if (seq_) {
    length_ = "length";
    arg_ = "seq";
  }
  if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
    be_global->header_ << "struct " << underscores_ << "_tag {};\n\n";
    unwrap_ = scoped_type_ + "& " + arg_ + " = wrap;\n  ACE_UNUSED_ARG(" + arg_ + ");\n";
    const_unwrap_ = "  const " + unwrap_;
    unwrap_ = "  " + unwrap_;
    arg_ = "wrap";
    ref_       = "IDL::DistinctType<"       + scoped_type_ + ", " + underscores_ + "_tag>";
    const_ref_ = "IDL::DistinctType<const " + scoped_type_ + ", " + underscores_ + "_tag>";
  } else {
    ref_ = scoped_type_ + (arr_ ? "_forany&" : "&");
    const_ref_ = "const " + ref_;
  }
}

void Field::set_element()
{
  if (ast_elem_) {
    cls_ = classify(ast_elem_);
    if (cls_ & CL_ENUM) {
      elem_sz_ = 4; elem_ = "ACE_CDR::ULong"; return;
    } else if (cls_ & CL_STRING) {
      elem_sz_ = 4; elem_ = string_type(cls_); return; // encoding of str length is 4 bytes
    } else if (cls_ & CL_PRIMITIVE) {
      AST_Type* type = resolveActualType(ast_elem_);
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
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
  }
}

std::string Field::string_type(Classification c)
{
  return be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11 ?
    ((c & CL_WIDE) ? "std::wstring" : "std::string") :
    (c & CL_WIDE) ? "TAO::WString_Manager" : "TAO::String_Manager";
}
