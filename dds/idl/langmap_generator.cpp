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
    HLP_SEQ, HLP_SEQ_NS, HLP_SEQ_VAR_VAR, HLP_SEQ_FIX_VAR, HLP_SEQ_OUT,
    HLP_ARR_VAR_VAR, HLP_ARR_FIX_VAR, HLP_ARR_OUT, HLP_ARR_FORANY,
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

  std::string map_type(AST_Expression::ExprType type)
  {
    AST_PredefinedType::PredefinedType pt = AST_PredefinedType::PT_void;
    switch (type)
    {
    case AST_Expression::EV_short: pt = AST_PredefinedType::PT_short; break;
    case AST_Expression::EV_ushort: pt = AST_PredefinedType::PT_ushort; break;
    case AST_Expression::EV_long: pt = AST_PredefinedType::PT_long; break;
    case AST_Expression::EV_ulong: pt = AST_PredefinedType::PT_ulong; break;
    case AST_Expression::EV_longlong: pt = AST_PredefinedType::PT_longlong; break;
    case AST_Expression::EV_ulonglong: pt = AST_PredefinedType::PT_ulonglong; break;
    case AST_Expression::EV_float: pt = AST_PredefinedType::PT_float; break;
    case AST_Expression::EV_double: pt = AST_PredefinedType::PT_double; break;
    case AST_Expression::EV_longdouble: pt = AST_PredefinedType::PT_longdouble; break;
    case AST_Expression::EV_char: pt = AST_PredefinedType::PT_char; break;
    case AST_Expression::EV_wchar: pt = AST_PredefinedType::PT_wchar; break;
    case AST_Expression::EV_octet: pt = AST_PredefinedType::PT_octet; break;
    case AST_Expression::EV_bool: pt = AST_PredefinedType::PT_boolean; break;
    case AST_Expression::EV_string: pt = AST_PredefinedType::PT_char; break;
    case AST_Expression::EV_wstring: pt = AST_PredefinedType::PT_wchar; break;
    default: break;
    }
    if (type == AST_Expression::EV_string || type == AST_Expression::EV_wstring)
      return primtype_[pt] + "* const";
    return primtype_[pt];
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
    be_global->add_include("FACE/StringManager.h", BE_GlobalData::STREAM_LANG_H);
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
    helpers_[HLP_SEQ] = "::OpenDDS::FaceTypes::Sequence";
    helpers_[HLP_SEQ_NS] = "::OpenDDS::FaceTypes";
    helpers_[HLP_SEQ_VAR_VAR] = "::TAO_VarSeq_Var_T";
    helpers_[HLP_SEQ_FIX_VAR] = "::TAO_FixedSeq_Var_T";
    helpers_[HLP_SEQ_OUT] = "::TAO_Seq_Out_T";
    helpers_[HLP_ARR_VAR_VAR] = "::TAO_VarArray_Var_T";
    helpers_[HLP_ARR_FIX_VAR] = "::TAO_FixedArray_Var_T";
    helpers_[HLP_ARR_OUT] = "::TAO_Array_Out_T";
    helpers_[HLP_ARR_FORANY] = "::TAO_Array_Forany_T";
    break;
  default: break;
  }
}

bool langmap_generator::gen_const(UTL_ScopedName* name, bool /*nestedInInterface*/,
                                  AST_Constant* constant)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  const char* const nm = name->last_component()->get_string();

  const AST_Expression::ExprType type = constant->et();
  const bool is_enum = (type == AST_Expression::EV_enum);
  const std::string type_name = is_enum
    ? scoped(constant->enum_full_name()) : map_type(type);
  be_global->lang_header_ <<
    "const " << type_name << ' ' << nm << " = ";

  if (is_enum) {
    be_global->lang_header_ << scoped(constant->constant_value()->n()) << ";\n";
  } else {
    be_global->lang_header_ << *constant->constant_value()->ev() << ";\n";
  }
  return true;
}

namespace {
  void gen_typecode(UTL_ScopedName* name)
  {
    if (be_global->suppress_typecode()) {
      return;
    }
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

namespace {
  void struct_decls(UTL_ScopedName* name, AST_Type::SIZE_TYPE size)
  {
    be_global->add_include("<tao/VarOut_T.h>", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = name->last_component()->get_string();
    const Helper var = (size == AST_Type::VARIABLE) ? HLP_VAR_VAR : HLP_FIX_VAR;
    be_global->lang_header_ <<
      "struct " << nm << ";\n"
      "typedef " << helpers_[var] << '<' << nm << "> " << nm << "_var;\n"
      "typedef " << helpers_[HLP_OUT] << '<' << nm << "> " << nm << "_out;\n";
  }
}

bool langmap_generator::gen_struct_fwd(UTL_ScopedName* name,
                                       AST_Type::SIZE_TYPE size)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  struct_decls(name, size);
  return true;
}

bool langmap_generator::gen_struct(UTL_ScopedName* name,
                                   const std::vector<AST_Field*>& fields,
                                   AST_Type::SIZE_TYPE size,
                                   const char*)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  const char* const nm = name->last_component()->get_string();
  struct_decls(name, size);
  be_global->lang_header_ <<
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

  be_global->lang_header_ << "\n"
    "  bool operator==(const " << nm << "& rhs) const;\n"
    "};\n\n";

  if (size == AST_Type::VARIABLE) {
    be_global->lang_header_ <<
      exporter() << "void swap(" << nm << "& lhs, " << nm << "& rhs);\n\n";
  }

  {
    const ScopedNamespaceGuard guard(name, be_global->impl_);
    be_global->impl_ <<
      "bool " << nm << "::operator==(const " << nm << "& rhs) const\n"
      "{\n"
      "  return ";

    for (size_t i = 0; i < fields.size(); ++i) {
      const std::string field_name = fields[i]->local_name()->get_string();
      be_global->impl_ << field_name << " == rhs." << field_name;
      if (i < fields.size() - 1) {
        be_global->impl_ << "\n    && ";
      }
    }
    be_global->impl_ << ";\n}\n\n";

    if (size == AST_Type::VARIABLE) {
      be_global->impl_ <<
        "void swap(" << nm << "& lhs, " << nm << "& rhs)\n"
        "{\n"
        "  using std::swap;\n";
      for (size_t i = 0; i < fields.size(); ++i) {
        const std::string fn = fields[i]->local_name()->get_string();
        AST_Type* field_type = fields[i]->field_type();
        resolveActualType(field_type);
        const Classification cls = classify(field_type);
        if (cls & CL_ARRAY) {
          std::string flat_fn = fn;
          ACE_CDR::ULong elems = 1;
          AST_Array* arr = AST_Array::narrow_from_decl(field_type);
          for (ACE_CDR::ULong dim = 0; dim < arr->n_dims(); ++dim) {
            elems *= arr->dims()[dim]->ev()->u.ulval;
            if (dim) flat_fn += "[0]";
          }
          be_global->add_include("<algorithm>", BE_GlobalData::STREAM_CPP);
          be_global->impl_ <<
            "  std::swap_ranges(lhs." << flat_fn << ", lhs." << flat_fn
            << " + " << elems << ", rhs." << flat_fn << ");\n";
        } else {
          be_global->impl_ <<
            "  swap(lhs." << fn << ", rhs." << fn << ");\n";
        }
      }
      be_global->impl_ << "}\n\n";
    }
  }

  gen_typecode(name);
  return true;
}

namespace {
  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    be_global->add_include("<tao/Seq_Var_T.h>", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("<tao/Seq_Out_T.h>", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("FACE/Sequence.h", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = tdname->last_component()->get_string();
    AST_Type* elem = seq->base_type();
    const Classification elem_cls = classify(elem);
    const bool bounded = !seq->unbounded();
    const Helper var = (elem->size_type() == AST_Type::VARIABLE)
                        ? HLP_SEQ_VAR_VAR : HLP_SEQ_FIX_VAR,
      out = HLP_SEQ_OUT;

    std::string elem_type = map_type(elem), extra_tmp_args;
    if (elem_cls & CL_STRING) {
      const AST_Expression::ExprType char_type = (elem_cls & CL_WIDE)
        ? AST_Expression::EV_wchar : AST_Expression::EV_char;
      extra_tmp_args = ", " + helpers_[HLP_SEQ_NS] + "::StringEltPolicy< "
        + map_type(char_type) + ">";
    } else if (elem_cls & CL_ARRAY) {
      extra_tmp_args = ", " + helpers_[HLP_SEQ_NS] + "::ArrayEltPolicy<"
        + elem_type + "_forany>";
    } else if (elem->size_type() == AST_Type::VARIABLE) {
      extra_tmp_args = ", " +  helpers_[HLP_SEQ_NS] + "::VariEltPolicy<"
        + elem_type + ">";
    }

    std::string bound = helpers_[HLP_SEQ_NS] + "::Unbounded";
    if (bounded) {
      std::ostringstream oss;
      oss << helpers_[HLP_SEQ_NS] << "::Bounded<"
        << seq->max_size()->ev()->u.ulval << '>';
      bound = oss.str();
    }

    const std::string base = helpers_[HLP_SEQ] + "< " + elem_type + ", " + bound
                           + extra_tmp_args + " >",
      len_type = primtype_[AST_PredefinedType::PT_ulong],
      flag_type = primtype_[AST_PredefinedType::PT_boolean];

    be_global->lang_header_ <<
      "class " << nm << ";\n"
      "typedef " << helpers_[var] << '<' << nm << "> " << nm << "_var;\n"
      "typedef " << helpers_[out] << '<' << nm << "> " << nm << "_out;\n\n"
      "class " << exporter() << nm << " : public " << base << " {\n"
      "public:\n"
      "  typedef " << nm << "_var _var_type;\n"
      "  typedef " << nm << "_out _out_type;\n\n"
      "  " << nm << "() {}\n"
      "  " << nm << "(const " << nm << "& seq) : " << base << "(seq) {}\n"
      "  friend void swap(" << nm << "& a, " << nm << "& b) { a.swap(b); }\n"
      "  " << nm << "& operator=(const " << nm << "& rhs)\n"
      "  {\n"
      "    " << nm << " tmp(rhs);\n"
      "    swap(tmp);\n"
      "    return *this;\n"
      "  }\n";

    if (bounded) {
      be_global->lang_header_ <<
        "  " << nm << "(" << len_type << " length, " << elem_type << "* data, "
        << flag_type << " release = false)\n"
        "    : " << base << "(0u, length, data, release) {}\n";
    } else {
      be_global->lang_header_ <<
        "  " << nm << "(" << len_type << " maximum)\n"
        "    : " << base << "(maximum, 0u, 0, true) {}\n"
        "  " << nm << "(" << len_type << " maximum, " << len_type << " length, "
        << elem_type << "* data, " << flag_type << " release = false)\n"
        "    : " << base << "(maximum, length, data, release) {}\n";
    }
    be_global->lang_header_ <<
      "};\n\n";
  }

  void gen_array(UTL_ScopedName* tdname, AST_Array* arr)
  {
    be_global->add_include("<tao/Array_VarOut_T.h>", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("dds/DCPS/SafetyProfilePool.h", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = tdname->last_component()->get_string();
    AST_Type* elem = arr->base_type();
    const Classification elem_cls = classify(elem);
    const Helper var = (elem->size_type() == AST_Type::VARIABLE)
                        ? HLP_ARR_VAR_VAR : HLP_ARR_FIX_VAR,
      out = HLP_ARR_OUT,
      forany = HLP_ARR_FORANY;

    std::ostringstream bound, nofirst, total;
    std::string zeros;
    for (ACE_CDR::ULong dim = 0; dim < arr->n_dims(); ++dim) {
      const ACE_CDR::ULong extent = arr->dims()[dim]->ev()->u.ulval;
      bound << '[' << extent << ']';
      if (dim) {
        nofirst << '[' << extent << ']';
        zeros += "[0]";
        total << " * ";
      }
      total << extent;
    }

    std::string elem_type = map_type(elem);
    if (elem_cls & CL_STRING) {
      elem_type = helpers_[(elem_cls & CL_WIDE) ? HLP_WSTR_MGR : HLP_STR_MGR];
    }

    std::string out_type = nm;
    if (elem->size_type() == AST_Type::VARIABLE) {
      out_type = helpers_[out] + '<' + nm + ", " + nm + "_var, " + nm +
        "_slice, " + nm + "_tag>";
    }

    be_global->lang_header_ <<
      "typedef " << elem_type << ' ' << nm << bound.str() << ";\n"
      "typedef " << elem_type << ' ' << nm << "_slice" << nofirst.str() << ";\n"
      "struct " << nm << "_tag {};\n"
      "typedef " << helpers_[var] << '<' << nm << ", " << nm << "_slice, "
      << nm << "_tag> " << nm << "_var;\n"
      "typedef " << out_type << ' ' << nm << "_out;\n"
      "typedef " << helpers_[forany] << '<' << nm << ", " << nm << "_slice, "
      << nm << "_tag> " << nm << "_forany;\n\n" <<
      exporter() << nm << "_slice* " << nm << "_alloc();\n" <<
      exporter() << "void " << nm << "_init_i(" << elem_type << "* begin);\n" <<
      exporter() << "void " << nm << "_fini_i(" << elem_type << "* begin);\n" <<
      exporter() << "void " << nm << "_free(" << nm << "_slice* slice);\n" <<
      exporter() << nm << "_slice* " << nm << "_dup(const " << nm
      << "_slice* slice);\n" <<
      exporter() << "void " << nm << "_copy(" << nm << "_slice* dst, const "
      << nm << "_slice* src);\n\n";
    const ScopedNamespaceGuard namespaces(tdname, be_global->impl_);
    be_global->impl_ <<
      nm << "_slice* " << nm << "_alloc()\n"
      "{\n"
      "  void* const raw = OpenDDS::DCPS::SafetyProfilePool::instance()->malloc"
      "(sizeof(" << nm << "));\n"
      "  " << nm << "_slice* const slice = static_cast<" << nm << "_slice*"
      << ">(raw);\n"
      "  " << nm << "_init_i(slice" << zeros << ");\n"
      "  return slice;\n"
      "}\n\n"
      "void " << nm << "_init_i(" << elem_type << "* begin)\n"
      "{\n";

    if (elem_cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_global->impl_ << "  ACE_UNUSED_ARG(begin);\n";
    } else if (elem_cls & CL_ARRAY) {
      std::string indent = "  ";
      const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
      be_global->impl_ <<
        indent << elem_type << "_init_i(begin" << nfl.index_ << ");\n";
    } else {
      be_global->impl_ <<
        "  std::uninitialized_fill_n(begin, " << total.str() << ", "
        << elem_type << "());\n";
    }

    be_global->impl_ <<
      "}\n\n"
      "void " << nm << "_fini_i(" << elem_type << "* begin)\n"
      "{\n";

    if (elem_cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_global->impl_ << "  ACE_UNUSED_ARG(begin);\n";
    } else if (elem_cls & CL_ARRAY) {
      std::string indent = "  ";
      const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
      be_global->impl_ <<
        indent << elem_type << "_fini_i(begin" << nfl.index_ << ");\n";
    } else {
      const std::string::size_type idx_last = elem_type.rfind("::");
      const std::string elem_last = (elem_cls & CL_STRING) ? "StringManager"
        : ((idx_last == std::string::npos) ? elem_type
           : elem_type.substr(idx_last + 2));
      be_global->impl_ <<
        "  for (int i = 0; i < " << total.str() << "; ++i) {\n"
        "    begin[i]." << elem_type << "::~" << elem_last << "();\n"
        "  }\n";
    }

    be_global->impl_ <<
      "}\n\n"
      "void " << nm << "_free(" << nm << "_slice* slice)\n"
      "{\n"
      "  if (!slice) return;\n"
      "  " << nm << "_fini_i(slice" << zeros << ");\n"
      "  OpenDDS::DCPS::SafetyProfilePool::instance()->free(slice);\n"
      "}\n\n" <<
      nm << "_slice* " << nm << "_dup(const " << nm << "_slice* slice)\n"
      "{\n"
      "  " << nm << "_slice* const arr = " << nm << "_alloc();\n"
      "  if (arr) " << nm << "_copy(arr, slice);\n"
      "  return arr;\n"
      "}\n\n"
      "void " << nm << "_copy(" << nm << "_slice* dst, const " << nm
      << "_slice* src)\n"
      "{\n"
      "  if (!src || !dst) return;\n";

    {
      std::string indent = "  ";
      const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
      if (elem_cls & CL_ARRAY) {
        be_global->impl_ <<
          indent << elem_type << "_copy(dst" << nfl.index_ << ", src"
          << nfl.index_ << ");\n";
      } else {
        be_global->impl_ <<
          indent << "dst" << nfl.index_ << " = src" << nfl.index_ << ";\n";
      }
    }

    be_global->impl_ <<
      "}\n\n";
  }

  // Outside of user's namespace: add Traits for arrays so that they can be
  // used in Sequences and Array_var/_out/_forany.
  void gen_array_traits(UTL_ScopedName* tdname, AST_Array* arr)
  {
    const std::string nm = scoped(tdname);
    std::string zeros;
    for (ACE_CDR::ULong i = 1; i < arr->n_dims(); ++i) zeros += "[0]";
    be_global->lang_header_ <<
      "namespace TAO {\n"
      "template <>\n"
      "struct " << exporter() << "Array_Traits<" << nm << "_forany>\n"
      "{\n"
      "  static void free(" << nm << "_slice* slice)\n"
      "  {\n"
      "    " << nm << "_free(slice);\n"
      "  }\n\n"
      "  static " << nm << "_slice* dup(const " << nm << "_slice* slice)\n"
      "  {\n"
      "    return " << nm << "_dup(slice);\n"
      "  }\n\n"
      "  static void copy(" << nm << "_slice* dst, const " << nm
      << "_slice* src)\n"
      "  {\n"
      "    " << nm << "_copy(dst, src);\n"
      "  }\n\n"
      "  static " << nm << "_slice* alloc()\n"
      "  {\n"
      "    return " << nm << "_alloc();\n"
      "  }\n\n"
      "  static void zero(" << nm << "_slice* slice)\n"
      "  {\n"
      "    " << nm << "_fini_i(slice" << zeros << ");\n"
      "    " << nm << "_init_i(slice" << zeros << ");\n"
      "  }\n"
      "  static void construct(" << nm << "_slice* slice)\n"
      "  {\n"
      "    " << nm << "_init_i(slice" << zeros << ");\n"
      "  }\n"
      "  static void destroy(" << nm << "_slice* slice)\n"
      "  {\n"
      "    " << nm << "_fini_i(slice" << zeros << ");\n"
      "  }\n"
      "};\n}\n\n";
  }
}

bool langmap_generator::gen_typedef(UTL_ScopedName* name, AST_Type* base,
                                    const char*)
{
  AST_Array* arr = 0;
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const char* const nm = name->last_component()->get_string();

    switch (base->node_type()) {
    case AST_Decl::NT_sequence:
      gen_sequence(name, AST_Sequence::narrow_from_decl(base));
      break;
    case AST_Decl::NT_array:
      gen_array(name, arr = AST_Array::narrow_from_decl(base));
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

    gen_typecode(name);
  }
  if (arr) gen_array_traits(name, arr);
  return true;
}

bool langmap_generator::gen_union(UTL_ScopedName*,
                                  const std::vector<AST_UnionBranch*>&,
                                  AST_Type*, const char*)
{
  std::cerr << "ERROR: unions are not supported with -L*\n";
  return false;
}
