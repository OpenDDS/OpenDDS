/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "langmap_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include <map>
#include <iostream>

using namespace AstTypeClassification;

namespace {
  std::map<AST_PredefinedType::PredefinedType, std::string> primtype_;

  enum Helper {
    HLP_STR_VAR, HLP_STR_OUT, HLP_WSTR_VAR, HLP_WSTR_OUT,
    HLP_STR_MGR, HLP_WSTR_MGR,
    HLP_FIX_VAR, HLP_VAR_VAR, HLP_OUT,
    HLP_SEQ_VAR_VAR, HLP_SEQ_FIX_VAR, HLP_SEQ_OUT,
    HLP_SEQ_BOUND, HLP_SEQ_UNBOUND,
  };
  std::map<Helper, std::string> helpers_;

  std::string map_type(AST_Type* type)
  {
    AST_Type* actual = type;
    resolveActualType(actual);
    const Classification cls = classify(actual);
    if (cls & CL_PRIMITIVE) {
      return primtype_[AST_PredefinedType::narrow_from_decl(actual)->pt()];
    }
    if (cls & CL_STRING) {
      const AST_PredefinedType::PredefinedType chartype = (cls & CL_WIDE)
        ? AST_PredefinedType::PT_wchar : AST_PredefinedType::PT_char;
      return primtype_[chartype] + '*';
    }
    if (cls & (CL_STRUCTURE | CL_SEQUENCE | CL_ARRAY | CL_ENUM)) {
      return scoped(type->name());
    }
    return "<<unknown>>";
  }

  std::string exporter() {
    return be_global->export_macro().empty() ? ""
      : be_global->export_macro().c_str() + std::string(" ");
  }
}

void langmap_generator::init()
{
  switch (be_global->language_mapping()) {
  case BE_GlobalData::LANGMAP_FACE_CXX:
    be_global->add_include("FACE/types.hpp", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("FACE/String_Manager.h", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("<tao/Seq_Var_T.h>", BE_GlobalData::STREAM_LANG_H); //TODO
    be_global->add_include("<tao/Seq_Out_T.h>", BE_GlobalData::STREAM_LANG_H); //TODO
    be_global->add_include("<tao/Sequence_T.h>", BE_GlobalData::STREAM_LANG_H); //TODO
    primtype_[AST_PredefinedType::PT_long] = "::FACE::Long";
    primtype_[AST_PredefinedType::PT_ulong] = "::FACE::UnsignedLong";
    primtype_[AST_PredefinedType::PT_longlong] = "::FACE::LongLong";
    primtype_[AST_PredefinedType::PT_ulonglong] = "::FACE::UnsignedLongLong";
    primtype_[AST_PredefinedType::PT_short] = "::FACE::Short";
    primtype_[AST_PredefinedType::PT_ushort] = "::FACE::UnsignedShort";
    primtype_[AST_PredefinedType::PT_float] = "::FACE::Float";
    primtype_[AST_PredefinedType::PT_double] = "::FACE::Double";
    primtype_[AST_PredefinedType::PT_longdouble] = "::FACE::LongDouble";
    primtype_[AST_PredefinedType::PT_char] = "::FACE::Char";
    primtype_[AST_PredefinedType::PT_wchar] = "::FACE::WChar";
    primtype_[AST_PredefinedType::PT_boolean] = "::FACE::Boolean";
    primtype_[AST_PredefinedType::PT_octet] = "::FACE::Octet";
    helpers_[HLP_STR_VAR] = "::FACE::String_var";
    helpers_[HLP_STR_OUT] = "::FACE::String_out";
    helpers_[HLP_WSTR_VAR] = "::FACE::WString_var";
    helpers_[HLP_WSTR_OUT] = "::FACE::WString_out";
    helpers_[HLP_STR_MGR] = "::OpenDDS::FaceTypes::String_mgr";
    helpers_[HLP_WSTR_MGR] = "::OpenDDS::FaceTypes::WString_mgr";
    helpers_[HLP_FIX_VAR] = "::TAO_Fixed_Var_T";
    helpers_[HLP_VAR_VAR] = "::TAO_Var_Var_T";
    helpers_[HLP_OUT] = "::TAO_Out_T";
    helpers_[HLP_SEQ_VAR_VAR] = "::TAO_VarSeq_Var_T"; //TODO: use FACE-enabled replacements
    helpers_[HLP_SEQ_FIX_VAR] = "::TAO_FixedSeq_Var_T";
    helpers_[HLP_SEQ_OUT] = "::TAO_Seq_Out_T";
    helpers_[HLP_SEQ_BOUND] = "::TAO::bounded_value_sequence"; //TODO: not always "value"
    helpers_[HLP_SEQ_UNBOUND] = "::TAO::unbounded_value_sequence";
    break;
  }
}

bool langmap_generator::gen_const(UTL_ScopedName* name, bool nestedInInterface,
                                  AST_Expression::ExprType type,
                                  AST_Expression::AST_ExprValue* value)
{
  return true;
}

namespace {
  void gen_typecode(UTL_ScopedName* name)
  {
    const char* const nm = name->last_component()->get_string();
    be_global->lang_header_ <<
      "extern " << exporter() << "const ::CORBA::TypeCode_ptr _tc_" << nm
      << ";\n";
    const ScopedNamespaceGuard cppNs(name, be_global->impl_);
    be_global->impl_ <<
      "const ::CORBA::TypeCode_ptr _tc_" << nm << " = 0;\n";
  }
}

bool langmap_generator::gen_enum(UTL_ScopedName* name,
                                 const std::vector<AST_EnumVal*>& contents,
                                 const char*)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  const char* const nm = name->last_component()->get_string();
  be_global->lang_header_ <<
    "enum " << nm << " {\n";
  for (size_t i = 0; i < contents.size(); ++i) {
    be_global->lang_header_ <<
      "  " << contents[i]->local_name()->get_string()
      << ((i < contents.size() - 1) ? ",\n" : "\n");
  }
  be_global->lang_header_ <<
    "};\n\n"
    "typedef " << nm << "& " << nm << "_out;\n";
  gen_typecode(name);
  return true;
}

bool langmap_generator::gen_struct(UTL_ScopedName* name,
                                   const std::vector<AST_Field*>& fields,
                                   AST_Type::SIZE_TYPE size,
                                   const char*)
{
  be_global->add_include("<tao/VarOut_T.h>", BE_GlobalData::STREAM_LANG_H);
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  const char* const nm = name->last_component()->get_string();
  const Helper var = (size == AST_Type::VARIABLE) ? HLP_VAR_VAR : HLP_FIX_VAR;

  be_global->lang_header_ <<
    "struct " << nm << ";\n"
    "typedef " << helpers_[var] << '<' << nm << "> " << nm << "_var;\n"
    "typedef " << helpers_[HLP_OUT] << '<' << nm << "> " << nm << "_out;\n"
    "\n"
    "struct " << exporter() << nm << "\n"
    "{\n"
    "  typedef " << nm << "_var _var_type;\n"
    "  typedef " << nm << "_out _out_type;\n\n";

  for (size_t i = 0; i < fields.size(); ++i) {
    AST_Type* field_type = fields[i]->field_type();
    const std::string field_name = fields[i]->local_name()->get_string();
    std::string type_name = map_type(field_type);
    const Classification cls = classify(field_type);
    if (cls & CL_STRING) {
      type_name = helpers_[(cls & CL_WIDE) ? HLP_WSTR_MGR : HLP_STR_MGR];
    }
    be_global->lang_header_ <<
      "  " << type_name << ' ' << field_name << ";\n";
  }

  be_global->lang_header_ <<
    "};\n\n";
  gen_typecode(name);
  return true;
}

namespace {
  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    const char* const nm = tdname->last_component()->get_string();
    AST_Type* elem = seq->base_type();
    const Classification elem_cls = classify(elem);
    const Helper var = (elem->size_type() == AST_Type::VARIABLE)
                        ? HLP_SEQ_VAR_VAR : HLP_SEQ_FIX_VAR,
      out = HLP_SEQ_OUT,
      base = seq->unbounded() ? HLP_SEQ_UNBOUND : HLP_SEQ_BOUND;

    std::string elem_type = map_type(elem);
    if (elem_cls & CL_STRING) {
      elem_type = helpers_[(elem_cls & CL_WIDE) ? HLP_WSTR_MGR : HLP_STR_MGR];
    }

    std::ostringstream bound;
    if (!seq->unbounded()) {
      bound << ", " << seq->max_size()->ev()->u.ulval;
    }

    const std::string len_type = primtype_[AST_PredefinedType::PT_ulong],
      flag_type = primtype_[AST_PredefinedType::PT_boolean];

    be_global->lang_header_ <<
      "class " << nm << ";\n"
      "typedef " << helpers_[var] << '<' << nm << "> " << nm << "_var;\n"
      "typedef " << helpers_[out] << '<' << nm << "> " << nm << "_out;\n\n"
      "class " << exporter() << nm << " : public " << helpers_[base] << "< "
      << elem_type << bound.str() << "> {\n"
      "public:\n"
      "  typedef " << nm << "_var _var_type;\n"
      "  typedef " << nm << "_out _out_type;\n\n"
      "  " << nm << "() {}\n";

    if (seq->unbounded()) {
      be_global->lang_header_ <<
        "  " << nm << "(" << len_type << " maximum)\n"
        "    : " << helpers_[base] << "< " << elem_type << ">(maximum) {}\n"
        "  " << nm << "(" << len_type << " maximum, " << len_type << " length, "
        << elem_type << "* data, " << flag_type << " release = false)\n"
        "    : " << helpers_[base] << "< " << elem_type
        << ">(maximum, length, data, release) {}\n";
    } else {
      be_global->lang_header_ <<
        "  " << nm << "(" << len_type << " length, " << elem_type << "* data, "
        << flag_type << " release = false)\n"
        "    : " << helpers_[base] << "< " << elem_type << bound.str()
        << ">(length, data, release) {}\n";
    }
    be_global->lang_header_ <<
      "};\n\n";
  }

  void gen_array(UTL_ScopedName* tdname, AST_Array* arr)
  {
  }
}

bool langmap_generator::gen_typedef(UTL_ScopedName* name, AST_Type* base,
                                    const char*)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  const char* const nm = name->last_component()->get_string();
  
  switch (base->node_type()) {
  case AST_Decl::NT_sequence:
    gen_sequence(name, AST_Sequence::narrow_from_decl(base));
    break;
  case AST_Decl::NT_array:
    gen_array(name, AST_Array::narrow_from_decl(base));
    break;
  default:
    be_global->lang_header_ <<
      "typedef " << map_type(base) << ' ' << nm << ";\n";
    break;
  }

  const Classification cls = classify(base);
  if (cls & CL_STRING) {
    const Helper var = (cls & CL_WIDE) ? HLP_WSTR_VAR : HLP_STR_VAR,
      out = (cls & CL_WIDE) ? HLP_WSTR_OUT : HLP_STR_OUT;
    be_global->lang_header_ <<
      "typedef " << helpers_[var] << ' ' << nm << "_var;\n"
      "typedef " << helpers_[out] << ' ' << nm << "_out;\n";
  }

  if (!be_global->suppress_typecode()) {
    gen_typecode(name);
  }

  return true;
}

bool langmap_generator::gen_union(UTL_ScopedName* name,
                                  const std::vector<AST_UnionBranch*>& branches,
                                  AST_Type* type, const char*)
{
  std::cerr << "ERROR: unions are not supported with -L*\n";
  return false;
}
