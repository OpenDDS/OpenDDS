/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "anonymous.h"

using namespace AstTypeClassification;

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

std::string Field::get_anonymous_type_name(const std::string& scoped_name)
{
  std::size_t i = scoped_name.find("::") + 2;
  return scoped_name.substr(0, i) + "_" + scoped_name.substr(i);
}

std::string Field::get_type_name(AST_Type& field)
{
  std::string n = scoped(field.name());
  return is_anonymous_type(field) ? get_anonymous_type_name(n) : n;
}

// for anonymous types
Field::Field(AST_Field& field) :
  ast_type(field.field_type()),
  arr(AST_Array::narrow_from_decl(ast_type)),
  seq(AST_Sequence::narrow_from_decl(ast_type)),
  ast_elem(arr ? arr->base_type() : (seq ? seq->base_type() : nullptr)),
  name(field.local_name()->get_string()),
  cls(CL_UNKNOWN), elem_sz(0), n_elems(1)
{
  type = ast_elem ? ("_" + name) : name;
  if (seq) { type += "_seq"; }
  std::string ftn = scoped(ast_type->name());
  struct_name = ftn.substr(0, ftn.find("::"));
  scoped_type = struct_name + "::" + type;
  //const bool use_cxx11 = (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11);
  //underscores = use_cxx11 ? dds_generator::scoped_helper(sn, "_") : "";
  init();
}

Field::Field(UTL_ScopedName* sn, AST_Type* base) :
  ast_type(base),
  arr(AST_Array::narrow_from_decl(ast_type)),
  seq(AST_Sequence::narrow_from_decl(ast_type)),
  ast_elem(arr ? arr->base_type() : (seq ? seq->base_type() : nullptr)),
  scoped_type(scoped(sn)),
  cls(CL_UNKNOWN), elem_sz(0), n_elems(1)
{
  const bool use_cxx11 = (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11);
  underscores = use_cxx11 ? dds_generator::scoped_helper(sn, "_") : "";
  init();
}

void Field::init()
{
  set_element();
  if (arr) {
    for (size_t i = 0; i < arr->n_dims(); ++i) {
      n_elems *= arr->dims()[i]->ev()->u.ulval;
    }
    length = std::to_string(n_elems);
    arg = "arr";
  } else if (seq) {
    length = "length";
    arg = "seq";
  }
  if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
    be_global->header_ << "struct " << underscores << "_tag {};\n\n";
    unwrap = scoped_type + "& " + arg + " = wrap;\n  ACE_UNUSED_ARG(" + arg + ");\n";
    const_unwrap = "  const " + unwrap;
    unwrap = "  " + unwrap;
    arg = "wrap";
    ref       = "IDL::DistinctType<"       + scoped_type + ", " + underscores + "_tag>";
    const_ref = "IDL::DistinctType<const " + scoped_type + ", " + underscores + "_tag>";
  } else {
    ref = scoped_type + (arr ? "_forany&" : "&");
    const_ref = "const " + ref;
  }
}

void Field::set_element()
{
  if (ast_elem) {
    cls = classify(ast_elem);
    if (cls & CL_ENUM) {
      elem_sz = 4; elem = "ACE_CDR::ULong"; return;
    } else if (cls & CL_STRING) {
      elem_sz = 4; elem = string_type(cls); return; // encoding of str length is 4 bytes
    } else if (cls & CL_PRIMITIVE) {
      AST_Type* type = resolveActualType(ast_elem);
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
      switch (p->pt()) {
      case AST_PredefinedType::PT_long: elem_sz = 4; elem = "ACE_CDR::Long"; return;
      case AST_PredefinedType::PT_ulong: elem_sz = 4; elem = "ACE_CDR::ULong"; return;
      case AST_PredefinedType::PT_longlong: elem_sz = 8; elem = "ACE_CDR::LongLong"; return;
      case AST_PredefinedType::PT_ulonglong: elem_sz = 8; elem = "ACE_CDR::ULongLong"; return;
      case AST_PredefinedType::PT_short: elem_sz = 2; elem = "ACE_CDR::Short"; return;
      case AST_PredefinedType::PT_ushort: elem_sz = 2; elem = "ACE_CDR::UShort"; return;
      case AST_PredefinedType::PT_float: elem_sz = 4; elem = "ACE_CDR::Float"; return;
      case AST_PredefinedType::PT_double: elem_sz = 8; elem = "ACE_CDR::Double"; return;
      case AST_PredefinedType::PT_longdouble: elem_sz = 16; elem = "ACE_CDR::LongDouble"; return;
      case AST_PredefinedType::PT_char: elem_sz = 1; elem = "ACE_CDR::Char"; return;
      case AST_PredefinedType::PT_wchar: elem_sz = 1; elem = "ACE_CDR::WChar"; return; // encoding of wchar length is 1 byte
      case AST_PredefinedType::PT_boolean: elem_sz = 1; elem = "ACE_CDR::Boolean"; return;
      case AST_PredefinedType::PT_octet: elem_sz = 1; elem = "ACE_CDR::Octet"; return;
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
