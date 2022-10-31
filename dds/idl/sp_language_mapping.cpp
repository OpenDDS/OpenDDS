#include "sp_language_mapping.h"
#include "dds_generator.h"
#include "field_info.h"

using namespace AstTypeClassification;

class SafetyProfileGenerator : public GeneratorBase
{
public:
  virtual void init()
  {
    be_builtin_global->add_include("tao/String_Manager_T.h", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("tao/CORBA_String.h", BE_BuiltinGlobalData::STREAM_LANG_H);
    primtype_[AST_PredefinedType::PT_long] = "CORBA::Long";
    primtype_[AST_PredefinedType::PT_ulong] = "CORBA::ULong";
    primtype_[AST_PredefinedType::PT_longlong] = "CORBA::LongLong";
    primtype_[AST_PredefinedType::PT_ulonglong] = "CORBA::ULongLong";
    primtype_[AST_PredefinedType::PT_short] = "CORBA::Short";
    primtype_[AST_PredefinedType::PT_ushort] = "CORBA::UShort";
#if OPENDDS_HAS_EXPLICIT_INTS
    primtype_[AST_PredefinedType::PT_int8] = "CORBA::Int8";
    primtype_[AST_PredefinedType::PT_uint8] = "CORBA::UInt8";
#endif
    primtype_[AST_PredefinedType::PT_float] = "CORBA::Float";
    primtype_[AST_PredefinedType::PT_double] = "CORBA::Double";
    primtype_[AST_PredefinedType::PT_longdouble] = "CORBA::LongDouble";
    primtype_[AST_PredefinedType::PT_char] = "CORBA::Char";
    primtype_[AST_PredefinedType::PT_wchar] = "CORBA::WChar";
    primtype_[AST_PredefinedType::PT_boolean] = "CORBA::Boolean";
    primtype_[AST_PredefinedType::PT_octet] = "CORBA::Octet";
    helpers_[HLP_STR_VAR] = "CORBA::String_var";
    helpers_[HLP_STR_OUT] = "CORBA::String_out";
    helpers_[HLP_WSTR_VAR] = "CORBA::WString_var";
    helpers_[HLP_WSTR_OUT] = "CORBA::WString_out";
    helpers_[HLP_STR_MGR] = "::TAO::String_Manager";
    helpers_[HLP_WSTR_MGR] = "CORBA::WString_mgr";
    helpers_[HLP_FIX_VAR] = "::TAO_Fixed_Var_T";
    helpers_[HLP_VAR_VAR] = "::TAO_Var_Var_T";
    helpers_[HLP_OUT] = "::TAO_Out_T";
    helpers_[HLP_SEQ] = "::OpenDDS::SafetyProfile::Sequence";
    helpers_[HLP_SEQ_NS] = "::OpenDDS::SafetyProfile";
    helpers_[HLP_SEQ_VAR_VAR] = "::OpenDDS::SafetyProfile::SequenceVar";
    helpers_[HLP_SEQ_FIX_VAR] = "::OpenDDS::SafetyProfile::SequenceVar";
    helpers_[HLP_SEQ_OUT] = "::TAO_Seq_Out_T";
    helpers_[HLP_ARR_VAR_VAR] = "::TAO_VarArray_Var_T";
    helpers_[HLP_ARR_FIX_VAR] = "::TAO_FixedArray_Var_T";
    helpers_[HLP_ARR_OUT] = "::TAO_Array_Out_T";
    helpers_[HLP_ARR_FORANY] = "::TAO_Array_Forany_T";
  }

  virtual void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    be_builtin_global->add_include("<tao/Seq_Out_T.h>", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("dds/DCPS/SafetyProfileSequence.h", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("dds/DCPS/SafetyProfileSequenceVar.h", BE_BuiltinGlobalData::STREAM_LANG_H);
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
    }
    else if (elem_cls & CL_ARRAY) {
      extra_tmp_args = ", " + helpers_[HLP_SEQ_NS] + "::ArrayEltPolicy<"
        + elem_type + "_forany>";
    }
    else if (elem->size_type() == AST_Type::VARIABLE) {
      extra_tmp_args = ", " + helpers_[HLP_SEQ_NS] + "::VariEltPolicy<"
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

    be_builtin_global->lang_header_ <<
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
      be_builtin_global->lang_header_ <<
        "  " << nm << "(" << len_type << " length, " << elem_type << "* data, "
        << flag_type << " release = false)\n"
        "    : " << base << "(0u, length, data, release) {}\n";
    }
    else {
      be_builtin_global->lang_header_ <<
        "  " << nm << "(" << len_type << " maximum)\n"
        "    : " << base << "(maximum, 0u, 0, true) {}\n"
        "  " << nm << "(" << len_type << " maximum, " << len_type << " length, "
        << elem_type << "* data, " << flag_type << " release = false)\n"
        "    : " << base << "(maximum, length, data, release) {}\n";
    }
    be_builtin_global->lang_header_ <<
      "};\n\n";

    be_builtin_global->lang_header_ <<
      "inline ACE_CDR::Boolean operator<< (ACE_OutputCDR&, const " << nm << "&) { return true; }\n\n";

    be_builtin_global->lang_header_ <<
      "inline ACE_CDR::Boolean operator>> (ACE_InputCDR&, " << nm << "&) { return true; }\n\n";
  }

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
    const std::vector<AST_Field*>& fields,
    AST_Type::SIZE_TYPE size,
    const char*)
  {
    const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
    const char* const nm = name->last_component()->get_string();
    struct_decls(name, size);
    be_builtin_global->lang_header_ <<
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
      be_builtin_global->lang_header_ <<
        "  " << type_name << ' ' << field_name << ";\n";
    }

    be_builtin_global->lang_header_ << "\n"
      "  bool operator==(const " << nm << "& rhs) const;\n"
      "  bool operator!=(const " << nm << "& rhs) const { return !(*this == rhs); }\n"
      "  OPENDDS_POOL_ALLOCATION_HOOKS\n"
      "};\n\n";

    be_builtin_global->add_include("dds/DCPS/PoolAllocationBase.h", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("<ace/CDR_Stream.h>", BE_BuiltinGlobalData::STREAM_LANG_H);

    if (size == AST_Type::VARIABLE) {
      be_builtin_global->lang_header_ <<
        exporter() << "void swap(" << nm << "& lhs, " << nm << "& rhs);\n\n";
    }

    be_builtin_global->lang_header_ <<
      exporter() << "ACE_CDR::Boolean operator<< (ACE_OutputCDR& os, const " << nm << "& x);\n\n";
    be_builtin_global->lang_header_ <<
      exporter() << "ACE_CDR::Boolean operator>> (ACE_InputCDR& os, " << nm << "& x);\n\n";

    {
      const ScopedNamespaceGuard guard(name, be_builtin_global->impl_);
      be_builtin_global->impl_ <<
        "bool " << nm << "::operator==(const " << nm << "& rhs) const\n"
        "{\n";
      for (size_t i = 0; i < fields.size(); ++i) {
        const std::string field_name = fields[i]->local_name()->get_string();
        AST_Type* field_type = resolveActualType(fields[i]->field_type());
        const Classification cls = classify(field_type);
        if (cls & CL_ARRAY) {
          std::string indent("  ");
          NestedForLoops nfl("int", "i",
            dynamic_cast<AST_Array*>(field_type), indent, true);
          be_builtin_global->impl_ <<
            indent << "if (" << field_name << nfl.index_ << " != rhs."
            << field_name << nfl.index_ << ") {\n" <<
            indent << "  return false;\n" <<
            indent << "}\n";
        }
        else {
          be_builtin_global->impl_ <<
            "  if (" << field_name << " != rhs." << field_name << ") {\n"
            "    return false;\n"
            "  }\n";
        }
      }
      be_builtin_global->impl_ << "  return true;\n}\n\n";

      if (size == AST_Type::VARIABLE) {
        be_builtin_global->impl_ <<
          "void swap(" << nm << "& lhs, " << nm << "& rhs)\n"
          "{\n"
          "  using std::swap;\n";
        for (size_t i = 0; i < fields.size(); ++i) {
          const std::string fn = fields[i]->local_name()->get_string();
          AST_Type* field_type = resolveActualType(fields[i]->field_type());
          const Classification cls = classify(field_type);
          if (cls & CL_ARRAY) {
            ACE_CDR::ULong elems = 1;
            const std::string flat_fn = fn + array_dims(field_type, elems);
            be_builtin_global->add_include("<algorithm>", BE_BuiltinGlobalData::STREAM_CPP);
            be_builtin_global->impl_ <<
              "  std::swap_ranges(lhs." << flat_fn << ", lhs." << flat_fn
              << " + " << elems << ", rhs." << flat_fn << ");\n";
          }
          else {
            be_builtin_global->impl_ <<
              "  swap(lhs." << fn << ", rhs." << fn << ");\n";
          }
        }
        be_builtin_global->impl_ << "}\n\n";
      }

      be_builtin_global->impl_ <<
        "ACE_CDR::Boolean operator<< (ACE_OutputCDR &, const " << nm << "&) { return true; }\n\n";
      be_builtin_global->impl_ <<
        "ACE_CDR::Boolean operator>> (ACE_InputCDR &, " << nm << "&) { return true; }\n\n";
    }

    gen_typecode(name);
    return true;
  }
};

SPLanguageMapping::~SPLanguageMapping()
{
}

bool SPLanguageMapping::cxx11() const
{
  return false;
}

std::string SPLanguageMapping::getMinimalHeaders() const
{
  return "#include <tao/orbconf.h>\n"
         "#include <tao/Basic_Types.h>\n";
}

bool SPLanguageMapping::needSequenceTypeSupportImplHeader() const
{
  return false;
}

bool SPLanguageMapping::skipTAOSequences() const
{
  return true;
}

GeneratorBase* SPLanguageMapping::getGeneratorHelper() const
{
  static SafetyProfileGenerator generator;
  return &generator;
}
