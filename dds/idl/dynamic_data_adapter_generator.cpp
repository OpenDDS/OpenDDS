/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dynamic_data_adapter_generator.h"

#include "field_info.h"
#include "be_extern.h"

#include <utl_identifier.h>

#include <string>

using namespace AstTypeClassification;

namespace {
  class DynamicDataAdapterGuard : public PreprocessorIfGuard {
  public:
    DynamicDataAdapterGuard()
      : PreprocessorIfGuard(" OPENDDS_HAS_DYNAMIC_DATA_ADAPTER")
    {
    }
  };

  class NoSafetyProfileGuard : public PreprocessorIfGuard {
  public:
    NoSafetyProfileGuard()
      : PreprocessorIfGuard("ndef OPENDDS_SAFETY_PROFILE")
    {
    }
  };

  const char* dynamic_data_adapter_suffix(AST_Type* type, bool set, std::string& extra)
  {
    const bool cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    AST_Type* const t = resolveActualType(type);
    switch (t->node_type()) {
    case AST_Decl::NT_pre_defined:
      switch (dynamic_cast<AST_PredefinedType*>(t)->pt()) {
      case AST_PredefinedType::PT_octet:
        return "byte";
      case AST_PredefinedType::PT_boolean:
        return "bool";
      case AST_PredefinedType::PT_int8:
        return "i8";
      case AST_PredefinedType::PT_uint8:
        return "u8";
      case AST_PredefinedType::PT_short:
      case AST_PredefinedType::PT_long:
      case AST_PredefinedType::PT_longlong:
      case AST_PredefinedType::PT_ushort:
      case AST_PredefinedType::PT_ulong:
      case AST_PredefinedType::PT_ulonglong:
      case AST_PredefinedType::PT_float:
      case AST_PredefinedType::PT_double:
      case AST_PredefinedType::PT_longdouble:
        return "simple";
      case AST_PredefinedType::PT_char:
        return "c8";
      case AST_PredefinedType::PT_wchar:
        return "c16";
      case AST_PredefinedType::PT_any:
      case AST_PredefinedType::PT_object:
      case AST_PredefinedType::PT_value:
      case AST_PredefinedType::PT_abstract:
      case AST_PredefinedType::PT_void:
      case AST_PredefinedType::PT_pseudo:
        return 0;
      }
      break;
    case AST_Decl::NT_enum:
      return "enum";
    case AST_Decl::NT_string:
      if (cxx11) {
        return "cpp11_s8";
      }
      if (set) {
        extra = ".inout()";
      }
      return "s8";
    case AST_Decl::NT_wstring:
      if (cxx11) {
        return "cpp11_s16";
      }
      if (set) {
        extra = ".inout()";
      }
      return "s16";
    case AST_Decl::NT_struct:
    case AST_Decl::NT_sequence:
    case AST_Decl::NT_union:
      return set ? "direct_complex" : "complex";
    case AST_Decl::NT_array:
      if (set) {
        return cxx11 ? "direct_complex" : "indirect_complex";
      }
      return "complex";
    case AST_Decl::NT_module:
    case AST_Decl::NT_root:
    case AST_Decl::NT_interface:
    case AST_Decl::NT_interface_fwd:
    case AST_Decl::NT_valuetype:
    case AST_Decl::NT_valuetype_fwd:
    case AST_Decl::NT_const:
    case AST_Decl::NT_except:
    case AST_Decl::NT_attr:
    case AST_Decl::NT_op:
    case AST_Decl::NT_argument:
    case AST_Decl::NT_union_fwd:
    case AST_Decl::NT_union_branch:
    case AST_Decl::NT_struct_fwd:
    case AST_Decl::NT_field:
    case AST_Decl::NT_enum_val:
    case AST_Decl::NT_typedef:
    case AST_Decl::NT_native:
    case AST_Decl::NT_factory:
    case AST_Decl::NT_finder:
    case AST_Decl::NT_component:
    case AST_Decl::NT_component_fwd:
    case AST_Decl::NT_home:
    case AST_Decl::NT_eventtype:
    case AST_Decl::NT_eventtype_fwd:
    case AST_Decl::NT_valuebox:
    case AST_Decl::NT_type:
    case AST_Decl::NT_fixed:
    case AST_Decl::NT_porttype:
    case AST_Decl::NT_provides:
    case AST_Decl::NT_uses:
    case AST_Decl::NT_publishes:
    case AST_Decl::NT_emits:
    case AST_Decl::NT_consumes:
    case AST_Decl::NT_ext_port:
    case AST_Decl::NT_mirror_port:
    case AST_Decl::NT_connector:
    case AST_Decl::NT_param_holder:
    case AST_Decl::NT_annotation_decl:
    case AST_Decl::NT_annotation_appl:
    case AST_Decl::NT_annotation_member:
      return 0;
    }
    return 0;
  }

  void generate_dynamic_data_adapter_access_field(
    AST_Union* union_node, bool set, const std::string& op,
    OpenDDS::XTypes::MemberId field_id, AST_Type* field_type, AST_Field* field = 0)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const bool disc = union_node && field_id == OpenDDS::XTypes::DISCRIMINATOR_ID;
    const std::string cpp_field_name = disc ? "_d" : field->local_name()->get_string();

    be_global->impl_ <<
      "    case ";
    if (disc) {
      be_global->impl_ << "OpenDDS::XTypes::DISCRIMINATOR_ID";
    } else {
      be_global->impl_ << field_id;
    }
    be_global->impl_ << ": // " << (disc ? "discriminator" : canonical_name(field)) << "\n"
      "      {\n";

    std::string extra;
    const char* const suffix = dynamic_data_adapter_suffix(field_type, set, extra);
    if (!suffix) {
      be_util::misc_error_and_abort("Unsupported field type in dynamic_data_adapter_suffix", field);
    }

    if (field && (!be_global->dynamic_data_adapter(field) ||
        !be_global->dynamic_data_adapter(field->field_type()))) {
      be_global->impl_ <<
        "        ACE_UNUSED_ARG(" << (set ? "source" : "dest") << ");\n"
        "        ACE_UNUSED_ARG(tk);\n"
        "        return missing_dda(method, id);\n";

    } else {
      std::string args = "value_.";
      const char* rc_dest = "return";
      if (union_node) {
        if (set) {
          // Setting a union branch or discriminator requires a temporary
          const Classification cls = classify(field_type);
          std::string temp_type =
            (cls & CL_STRING) ? string_type(cls) : scoped(field_type->name());
          if (field) {
            FieldInfo af(*field);
            if (af.as_base_ && af.type_->anonymous()) {
              temp_type = af.scoped_type_;
            }
          }
          be_global->impl_ <<
            "        " << temp_type << " temp;\n";
          rc_dest = "rc =";
          args = "temp" + extra;
        } else {
          args += cpp_field_name + "()" + extra;
        }
      } else {
        args += (use_cxx11 ? "_" : "") + cpp_field_name + extra;
      }
      args += ", id";
      be_global->impl_ <<
        "        " << rc_dest << " " << op << "_" << suffix << "_raw_value(method, ";
      if (set) {
        be_global->impl_ << args << ", source, tk";
      } else {
        be_global->impl_ << "dest, tk, " << args;
      }
      be_global->impl_ << ");\n";
      if (union_node && set) {
        be_global->impl_ <<
          "        if (rc == DDS::RETCODE_OK) {\n"
          "          value_." << cpp_field_name << "(temp);\n"
          "        }\n"
          "        return rc;\n";
      }
    }

    be_global->impl_ <<
      "      }\n";
  }

  bool generate_dynamic_data_adapter_access(AST_Decl* node, bool set)
  {
    AST_Structure* struct_node = 0;
    AST_Union* union_node = 0;
    AST_Sequence* seq_node = 0;
    AST_Array* array_node = 0;
    if (!node) {
      return false;
    }
    switch (node->node_type()) {
    case AST_Decl::NT_struct:
      struct_node = dynamic_cast<AST_Structure*>(node);
      break;
    case AST_Decl::NT_union:
      union_node = dynamic_cast<AST_Union*>(node);
      break;
    case AST_Decl::NT_sequence:
      seq_node = dynamic_cast<AST_Sequence*>(node);
      break;
    case AST_Decl::NT_array:
      array_node = dynamic_cast<AST_Array*>(node);
      break;
    default:
      return false;
    }

    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

    const std::string op = set ? "set" : "get";
    be_global->impl_ << "  DDS::ReturnCode_t " << op << "_raw_value(const char* method, ";
    if (set) {
      be_global->impl_ << "DDS::MemberId id, const void* source, DDS::TypeKind tk";
    } else {
      be_global->impl_ << "void* dest, DDS::TypeKind tk, DDS::MemberId id";
    }
    be_global->impl_ << ")\n"
      "  {\n";

    if (struct_node || union_node) {
      if (set) {
        be_global->impl_ <<
          "    DDS::ReturnCode_t rc = assert_mutable(method);\n"
          "    if (rc != DDS::RETCODE_OK) {\n"
          "      return rc;\n"
          "    }\n";
      }
      be_global->impl_ <<
        "    switch (id) {\n";

      if (union_node) {
        generate_dynamic_data_adapter_access_field(
          union_node, set, op, OpenDDS::XTypes::DISCRIMINATOR_ID, union_node->disc_type());
      }

      const Fields fields(union_node ? union_node : struct_node);
      for (Fields::Iterator i = fields.begin(); i != fields.end(); ++i) {
        AST_Field* const field = *i;
        generate_dynamic_data_adapter_access_field(
          union_node, set, op,
          be_global->get_id(field), field->field_type(), field);
      }

      be_global->impl_ <<
        "    default:\n"
        "      return invalid_id(method, id);\n"
        "    }\n";
    } else if (seq_node || array_node) {
      be_global->impl_ <<
        "    const DDS::ReturnCode_t rc = check_index(method, id, ";
      if (seq_node || use_cxx11) {
        be_global->impl_ << "value_." << (use_cxx11 ? "size" : "length") << "()";
      } else {
        be_global->impl_ << array_element_count(array_node);
      }
      be_global->impl_ << ");\n"
        "    if (rc != DDS::RETCODE_OK) {\n"
        "      return rc;\n"
        "    }\n";

      AST_Type* const base_type = seq_node ? seq_node->base_type() : array_node->base_type();
      std::string extra;
      const char* const suffix = dynamic_data_adapter_suffix(base_type, set, extra);
      if (!suffix) {
        be_util::misc_error_and_abort(
          "Unsupported array or sequence base type in dynamic_data_adapter_suffix", node);
      }
      std::string args;
      if (seq_node) {
        args = "value_[id]";
      } else {
        args = "(&value_";
        for (ACE_CDR::ULong dim = array_node->n_dims(); dim; --dim) {
          args += "[0]";
        }
        args += ")[id]";
      }
      args += extra + ", id";
      be_global->impl_ <<
        "    return " << op << "_" << suffix << "_raw_value(method, ";
      if (set) {
        be_global->impl_ << args << ", source, tk";
      } else {
        be_global->impl_ << "dest, tk, " << args;
      }
      be_global->impl_ << ");\n";
    }

    be_global->impl_ <<
      "  }\n";

    return true;
  }

  void generate_get_dynamic_data_adapter(
    const std::string& cpp_name, const std::string& export_macro,
    bool is_const, bool dda_generated)
  {
    be_global->header_ <<
      "template <>\n" <<
      export_macro << "DDS::DynamicData_ptr get_dynamic_data_adapter<" << cpp_name <<
        ">(DDS::DynamicType_ptr type, " << (is_const ? "const " : "") << cpp_name << "& value);\n"
      "\n";

    be_global->impl_ <<
      "template <>\n"
      "DDS::DynamicData_ptr get_dynamic_data_adapter<" << cpp_name <<
        ">(DDS::DynamicType_ptr type, " << (is_const ? "const " : "") << cpp_name << "& value)\n"
      "{\n";
    if (dda_generated) {
      be_global->impl_ <<
        "#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER\n"
        "  if (type) {\n"
        "    return new DynamicDataAdapterImpl<" << cpp_name << ">(type, value);\n"
        "  }\n"
        "#  else\n";
    }
    be_global->impl_ <<
      "  ACE_UNUSED_ARG(type);\n"
      "  ACE_UNUSED_ARG(value);\n";
    if (dda_generated) {
      be_global->impl_ <<
        "#  endif\n";
    }
    be_global->impl_ <<
      "  return 0;\n"
      "}\n"
      "\n";
  }

  bool generate_dynamic_data_adapter(AST_Decl* node, const std::string* use_scoped_name = 0)
  {
    AST_Structure* struct_node = 0;
    AST_Union* union_node = 0;
    AST_Sequence* seq_node = 0;
    AST_Array* array_node = 0;
    if (!node) {
      return false;
    }
    switch (node->node_type()) {
    case AST_Decl::NT_struct:
      struct_node = dynamic_cast<AST_Structure*>(node);
      break;
    case AST_Decl::NT_union:
      union_node = dynamic_cast<AST_Union*>(node);
      break;
    case AST_Decl::NT_sequence:
      seq_node = dynamic_cast<AST_Sequence*>(node);
      break;
    case AST_Decl::NT_array:
      array_node = dynamic_cast<AST_Array*>(node);
      break;
    default:
      return true;
    }
    const std::string cpp_name = use_scoped_name ? *use_scoped_name : scoped(node->name());
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const bool generate = be_global->dynamic_data_adapter(node);

    be_global->add_include("dds/DCPS/XTypes/DynamicDataAdapter.h", BE_GlobalData::STREAM_H);

    if (struct_node) {
      const Fields fields(struct_node);
      FieldInfo::EleLenSet anonymous_seq_generated;
      for (Fields::Iterator i = fields.begin(); i != fields.end(); ++i) {
        AST_Field* const field = *i;
        if (field->field_type()->anonymous()) {
          FieldInfo af(*field);
          if (af.arr_ || (af.seq_ && af.is_new(anonymous_seq_generated))) {
            if (!generate_dynamic_data_adapter(af.type_, &af.scoped_type_)) {
              be_util::misc_error_and_abort(
                "Failed to generate adapter for anonymous type of field", field);
            }
          }
        }
      }
    }

    std::vector<std::string> ns;
    ns.push_back("OpenDDS");
    ns.push_back("XTypes");
    NamespaceGuard ng(true, &ns);

    if (generate) {
      DynamicDataAdapterGuard ddag;

      be_global->impl_ <<
        "template <>\n"
        "class DynamicDataAdapterImpl<" << cpp_name << "> : public DynamicDataAdapter_T<"
          << cpp_name << "> {\n"
        "public:\n"
        "  DynamicDataAdapterImpl(DDS::DynamicType_ptr type, " << cpp_name << "& value)\n"
        "    : DynamicDataAdapter_T(type, value)\n"
        "  {\n"
        "  }\n"
        "\n"
        "  DynamicDataAdapterImpl(DDS::DynamicType_ptr type, const " << cpp_name << "& value)\n"
        "    : DynamicDataAdapter_T(type, value)\n"
        "  {\n"
        "  }\n"
        "\n"
        "  DDS::UInt32 get_item_count()\n"
        "  {\n"
        "    return ";
      if (struct_node) {
        be_global->impl_ << struct_node->nfields();
      } else if (union_node) {
        be_global->impl_ << union_node->nfields() + 1;
      } else if (seq_node) {
        be_global->impl_ << "value_." << (use_cxx11 ? "size" : "length") << "()";
      } else if (array_node) {
        be_global->impl_ << array_element_count(array_node);
      }
      be_global->impl_ << ";\n"
        "  }\n"
        "\n";

      if (seq_node) {
        // TODO: Set new elements to default values?
        be_global->impl_ <<
          "  DDS::MemberId get_member_id_at_index_impl(DDS::UInt32 index)\n"
          "  {\n"
          "    const DDS::UInt32 count = value_." << (use_cxx11 ? "size" : "length") << "();\n"
          "    if (!read_only_ && index >= count) {\n"
          "      value_." << (use_cxx11 ? "resize" : "length") << "(index + 1);\n"
          "      return index;\n"
          "    }\n"
          "    return check_index(\"get_member_id_at_index\", index, count)"
            " == DDS::RETCODE_OK ? index : MEMBER_ID_INVALID;\n"
          "  }\n"
          "\n";
      }

      be_global->impl_ <<
          "protected:\n";

      if (!generate_dynamic_data_adapter_access(node, false)) {
        return false;
      }
      be_global->impl_ << "\n";
      if (!generate_dynamic_data_adapter_access(node, true)) {
        return false;
      }

      be_global->impl_ <<
        "};\n"
        "\n";
    }

    {
      std::string export_macro = be_global->export_macro().c_str();
      if (export_macro.size()) {
        export_macro += " ";
      }

      NoSafetyProfileGuard nspg;
      generate_get_dynamic_data_adapter(cpp_name, export_macro, true, generate);
      generate_get_dynamic_data_adapter(cpp_name, export_macro, false, generate);

      be_global->header_ <<
        "template <>\n" <<
        export_macro << "const " << cpp_name << "* get_dynamic_data_adapter_value<" <<
          cpp_name << ">(DDS::DynamicData_ptr dd);\n"
        "\n";

      be_global->impl_ <<
        "template <>\n"
        "const " << cpp_name << "* get_dynamic_data_adapter_value<" <<
          cpp_name << ">(DDS::DynamicData_ptr dd)\n"
        "{\n"
        "  ACE_UNUSED_ARG(dd);\n";
      if (generate) {
        be_global->impl_ <<
          "#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER\n"
          "  typedef DynamicDataAdapterImpl<" << cpp_name << "> Dda;\n"
          "  const Dda* const dda = dynamic_cast<Dda*>(dd);\n"
          "  if (dda) {\n"
          "    return &dda->wrapped();\n"
          "  }\n"
          "#  endif\n";
      }
      be_global->impl_ <<
        "  return 0;\n";
      be_global->impl_ <<
        "}\n";
    }

    return true;
  }
}

bool dynamic_data_adapter_generator::gen_struct(AST_Structure* node, UTL_ScopedName*,
  const std::vector<AST_Field*>&, AST_Type::SIZE_TYPE, const char*)
{
  return generate_dynamic_data_adapter(node);
}

bool dynamic_data_adapter_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name,
  AST_Type* type, const char*)
{
  const AST_Decl::NodeType nt = type->node_type();
  if (nt != AST_Decl::NT_sequence && nt != AST_Decl::NT_array) {
    return true;
  }
  const std::string cpp_name = scoped(name);
  return generate_dynamic_data_adapter(type, &cpp_name);
}

bool dynamic_data_adapter_generator::gen_union(AST_Union* node, UTL_ScopedName*,
  const std::vector<AST_UnionBranch*>&, AST_Type*, const char*)
{
  return generate_dynamic_data_adapter(node);
}
