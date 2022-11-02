#include "face_language_mapping.h"
#include "opendds_idl_face_export.h"

#include <langmap_generator_helper.h>
#include <dds_generator.h>
#include <field_info.h>

using namespace AstTypeClassification;

extern "C" {
  opendds_idl_face_Export LanguageMapping* opendds_idl_face_allocator()
  {
    FaceLanguageMapping* ptr = 0;
    ACE_NEW_RETURN(ptr, FaceLanguageMapping, 0);
    return ptr;
  }
}

class FaceGenerator : public GeneratorBase
{
public:
  FaceGenerator()
    : emitTS_(false)
  {
  }

  virtual std::string string_ns()
  {
    return "::FACE";
  }

  virtual void add_fixed_include()
  {
    be_builtin_global->add_include("FACE/Fixed.h", BE_BuiltinGlobalData::STREAM_LANG_H);
  }

  virtual void enum_entry(AST_Enum* e, AST_Union* the_union, AST_Union::DefaultValue& dv, std::stringstream& first_label)
  {
    std::string prefix = scoped(e->name());
    const size_t pos = prefix.rfind("::");
    if (pos != std::string::npos) {
      first_label << prefix.substr(0, pos) + "::";
    }
    UTL_ScopedName* default_name;
    if (dv.u.enum_val < static_cast<ACE_CDR::ULong>(e->member_count())) {
      default_name = e->value_to_name(dv.u.enum_val);
    }
    else {
      const Fields fields(the_union);
      AST_UnionBranch* ub = dynamic_cast<AST_UnionBranch*>(*(fields.begin()));
      AST_Expression::AST_ExprValue* ev = ub->label(0)->label_val()->ev();
      default_name = e->value_to_name(ev->u.eval);
    }
    first_label << default_name->last_component()->get_string();
  }

  virtual void init()
  {
    be_builtin_global->add_include("FACE/types.hpp", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("FACE/StringManager.h", BE_BuiltinGlobalData::STREAM_LANG_H);
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
    helpers_[HLP_SEQ_VAR_VAR] = "::OpenDDS::FaceTypes::SequenceVar";
    helpers_[HLP_SEQ_FIX_VAR] = "::OpenDDS::FaceTypes::SequenceVar";
    helpers_[HLP_SEQ_OUT] = "::TAO_Seq_Out_T";
    helpers_[HLP_ARR_VAR_VAR] = "::TAO_VarArray_Var_T";
    helpers_[HLP_ARR_FIX_VAR] = "::TAO_FixedArray_Var_T";
    helpers_[HLP_ARR_OUT] = "::TAO_Array_Out_T";
    helpers_[HLP_ARR_FORANY] = "::TAO_Array_Forany_T";
    helpers_[HLP_FIXED] = "::OpenDDS::FaceTypes::Fixed_T";
    helpers_[HLP_FIXED_CONSTANT] = "::OpenDDS::FaceTypes::Fixed";
  }

  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    be_builtin_global->add_include("<tao/Seq_Out_T.h>", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("FACE/Sequence.h", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("FACE/SequenceVar.h", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("<ace/CDR_Stream.h>", BE_BuiltinGlobalData::STREAM_LANG_H);
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

  virtual const char* generate_ts(AST_Decl* decl, UTL_ScopedName* name) {
    if (emitTS_) {
      if (decl->node_type() == AST_Decl::NT_struct) {
        const std::string cxx_name = scoped(name),
          name_underscores = dds_generator::scoped_helper(name, "_"),
          exportMacro = be_builtin_global->export_macro().c_str(),
          exporter = exportMacro.empty() ? "" : ("    " + exportMacro + '\n');
        be_builtin_global->add_include("FACE/TS.hpp", FaceLanguageMapping::STREAM_FACETS_H);
        facets_header_ <<
          "namespace FACE\n"
          "{\n"
          "  namespace Read_Callback\n"
          "  {\n"
          "    typedef void (*send_event_" << name_underscores << "_Ptr) (\n"
          "      /* in */ TRANSACTION_ID_TYPE transaction_id,\n"
          "      /* inout */ " << cxx_name << "& message,\n"
          "      /* in */ MESSAGE_TYPE_GUID message_type_id,\n"
          "      /* in */ MESSAGE_SIZE_TYPE message_size,\n"
          "      /* in */ const WAITSET_TYPE waitset,\n"
          "      /* out */ RETURN_CODE_TYPE& return_code);\n"
          "  }\n\n"
          "  namespace TS\n"
          "  {\n" << exporter <<
          "    void Receive_Message(\n"
          "      /* in */ CONNECTION_ID_TYPE connection_id,\n"
          "      /* in */ TIMEOUT_TYPE timeout,\n"
          "      /* inout */ TRANSACTION_ID_TYPE& transaction_id,\n"
          "      /* out */ " << cxx_name << "& message,\n"
          "      /* in */ MESSAGE_SIZE_TYPE message_size,\n"
          "      /* out */ RETURN_CODE_TYPE& return_code);\n\n" << exporter <<
          "    void Send_Message(\n"
          "      /* in */ CONNECTION_ID_TYPE connection_id,\n"
          "      /* in */ TIMEOUT_TYPE timeout,\n"
          "      /* inout */ TRANSACTION_ID_TYPE& transaction_id,\n"
          "      /* inout */ " << cxx_name << "& message,\n"
          "      /* inout */ MESSAGE_SIZE_TYPE& message_size,\n"
          "      /* out */ RETURN_CODE_TYPE& return_code);\n\n" << exporter <<
          "    void Register_Callback(\n"
          "      /* in */ CONNECTION_ID_TYPE connection_id,\n"
          "      /* in */ const WAITSET_TYPE waitset,\n"
          "      /* in */ Read_Callback::send_event_" << name_underscores
          << "_Ptr data_callback,\n"
          "      /* in */ MESSAGE_SIZE_TYPE max_message_size,\n"
          "      /* out */ RETURN_CODE_TYPE& return_code);\n\n"
          "  }\n"
          "}\n\n";
        facets_impl_ <<
          "void Receive_Message(CONNECTION_ID_TYPE connection_id,\n"
          "                     TIMEOUT_TYPE timeout,\n"
          "                     TRANSACTION_ID_TYPE& transaction_id,\n"
          "                     " << cxx_name << "& message,\n"
          "                     MESSAGE_SIZE_TYPE message_size,\n"
          "                     RETURN_CODE_TYPE& return_code) {\n"
          "  OpenDDS::FaceTSS::receive_message(connection_id, timeout,\n"
          "                                    transaction_id, message,\n"
          "                                    message_size, return_code);\n"
          "}\n\n"
          "void Send_Message(CONNECTION_ID_TYPE connection_id,\n"
          "                  TIMEOUT_TYPE timeout,\n"
          "                  TRANSACTION_ID_TYPE& transaction_id,\n"
          "                  " << cxx_name << "& message,\n"
          "                  MESSAGE_SIZE_TYPE& message_size,\n"
          "                  RETURN_CODE_TYPE& return_code) {\n"
          "  OpenDDS::FaceTSS::send_message(connection_id, timeout,\n"
          "                                 transaction_id, message,\n"
          "                                 message_size, return_code);\n"
          "}\n\n"
          "void Register_Callback(CONNECTION_ID_TYPE connection_id,\n"
          "                       const WAITSET_TYPE waitset,\n"
          "                       Read_Callback::send_event_" << name_underscores
          << "_Ptr data_callback,\n"
          "                       MESSAGE_SIZE_TYPE max_message_size,\n"
          "                       RETURN_CODE_TYPE& return_code) {\n"
          "  OpenDDS::FaceTSS::register_callback(connection_id, waitset,\n"
          "                                      data_callback,\n"
          "                                      max_message_size, return_code);\n"
          "}\n\n";
      }
      else {
        return "Generating FACE type support for Union topic types is not supported";
      }
    }
    return 0;
  }

  std::ostringstream facets_header_, facets_impl_;
  bool emitTS_;
};

FaceLanguageMapping::FaceLanguageMapping()
  : emitTS_(false)
{
}

FaceLanguageMapping::~FaceLanguageMapping()
{
}

bool FaceLanguageMapping::cxx11() const
{
  return false;
}

bool FaceLanguageMapping::none() const
{
  return false;
}

std::string FaceLanguageMapping::getMinimalHeaders() const
{
  return "#include <tao/orbconf.h>\n"
         "#include <tao/Basic_Types.h>\n";
}

std::string FaceLanguageMapping::getInputCDRToString(bool wide) const
{
  return wide ? "ACE_InputCDR::to_wstring" : "ACE_InputCDR::to_string";
}

std::string FaceLanguageMapping::getBranchStringType(bool wide) const
{
  return wide ? "FACE::WString_var" : "FACE::String_var";
}

std::string FaceLanguageMapping::getBranchStringSuffix() const
{
  return ".out()";
}

std::string FaceLanguageMapping::getBoundStringSuffix() const
{
  return "";
}

std::string FaceLanguageMapping::getBranchStringPrefix() const
{
  return "FACE::";
}

bool FaceLanguageMapping::needSequenceTypeSupportImplHeader() const
{
  return false;
}

void FaceLanguageMapping::setTS(bool setting)
{
  emitTS_ = setting;
  dynamic_cast<FaceGenerator*>(getGeneratorHelper())->emitTS_ = setting;
}

void FaceLanguageMapping::produceTS() const
{
  if (emitTS_) {
    FaceGenerator* generator = dynamic_cast<FaceGenerator*>(getGeneratorHelper());
    postprocess(facets_header_name_.c_str(),
                generator->facets_header_, STREAM_FACETS_H);
    postprocess(facets_impl_name_.c_str(),
                generator->facets_impl_, STREAM_FACETS_CPP);
  }
}

GeneratorBase* FaceLanguageMapping::getGeneratorHelper() const
{
  static FaceGenerator generator;
  return &generator;
}

LanguageMapping::Includes_t* FaceLanguageMapping::additional_includes(int which)
{
  return (which == STREAM_FACETS_H ? &additional_h_ : 0);
}

void FaceLanguageMapping::reset_includes()
{
  additional_h_.clear();
}

void FaceLanguageMapping::set_additional_names(const std::string& filebase)
{
  facets_header_name_ = (filebase + "_TS.hpp").c_str();
  facets_impl_name_ = (filebase + "_TS.cpp").c_str();
}

void FaceLanguageMapping::postprocess_guard_begin(const std::string& macrofied, std::ostringstream& out, int which) const
{
  switch (which) {
    case STREAM_FACETS_H: {
      out << "#ifndef " + macrofied << "\n#define " << macrofied << '\n';
      emit_tao_header(out);
      break;
    }
    case STREAM_FACETS_CPP: {
      const ACE_CString pch = be_builtin_global->pch_include();
      if (pch.length() != 0) {
        out << "#include \"" << pch << "\"\n";
      }
      out << "#include \"" << facets_header_name_.c_str() << "\"\n"
             "#include \"" << be_builtin_global->header_name_.c_str() << "\"\n"
             "#include \"dds/FACE/FaceTSS.h\"\n\n"
             "namespace FACE { namespace TS {\n\n";
      break;
    }
  }

}

void FaceLanguageMapping::postprocess_guard_end(const std::string& macrofied, std::ostringstream& out, int which) const
{
  switch (which) {
  case STREAM_FACETS_H:
    out << "#endif /* " << macrofied << " */\n";
    break;
  case STREAM_FACETS_CPP:
    out << "}}\n";
    break;
  }
}
