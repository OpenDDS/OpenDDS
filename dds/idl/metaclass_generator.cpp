/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "metaclass_generator.h"

#include "marshal_generator.h"
#include "field_info.h"
#include "be_extern.h"
#include "topic_keys.h"

#include <utl_identifier.h>

#include <cstddef>
#include <stdexcept>

using namespace AstTypeClassification;

namespace {
  struct ContentSubscriptionGuard {
    explicit ContentSubscriptionGuard(bool activate = true)
      : activate_(activate)
    {
      if (activate) {
        be_global->header_ <<
          "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n";
        be_global->impl_ <<
          "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n";
      }
    }
    ~ContentSubscriptionGuard()
    {
      if (activate_) {
        be_global->header_ << "#endif\n";
        be_global->impl_ << "#endif\n";
      }
    }
    bool activate_;
  };

  const char* raw_adapter_suffix(AST_Type* type, bool set, std::string& extra)
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

  size_t array_total_count(AST_Array* array_node)
  {
    ACE_CDR::ULong total = 1;
    for (ACE_CDR::ULong dim = array_node->n_dims(); dim; --dim) {
      total *= array_node->dims()[dim - 1]->ev()->u.ulval;
    }
    return total;
  }

  void generate_raw_adapter_access_field(
    AST_Union* union_node, bool set, const std::string& op,
    OpenDDS::XTypes::MemberId field_id, AST_Type* field_type, const std::string field_name,
    AST_Field* field = 0)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

    be_global->impl_ <<
      "    case ";
    if (union_node && field_id == OpenDDS::XTypes::DISCRIMINATOR_ID) {
      be_global->impl_ << "OpenDDS::XTypes::DISCRIMINATOR_ID";
    } else {
      be_global->impl_ << field_id;
    }
    be_global->impl_ << ":\n"
      "      {\n";

    std::string extra;
    const char* const suffix = raw_adapter_suffix(field_type, set, extra);
    if (!suffix) {
      be_util::misc_error_and_abort("Unsupported field type in raw_adapter_suffix", field);
    }

    if (field && (!be_global->dynamic_data_adapter(field) ||
        !be_global->dynamic_data_adapter(field->field_type()))) {
      // TODO: Call method that logs that @OpenDDS::no_dynamic_data_adapter
      // was used.
      be_global->impl_ <<
        "        return DDS::RETCODE_UNSUPPORTED;\n";

    } else {
      std::string args = "typed.";
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
          args += field_name + "()" + extra;
        }
      } else {
        args += (use_cxx11 ? "_" : "") + field_name + extra;
      }
      args += ", type, id";
      be_global->impl_ <<
        "        " << rc_dest << " " << op << "_" << suffix << "_value(";
      if (set) {
        be_global->impl_ << args << ", source, tk";
      } else {
        be_global->impl_ << "dest, tk, " << args;
      }
      be_global->impl_ << ");\n";
      if (union_node && set) {
        be_global->impl_ <<
          "        if (rc == DDS::RETCODE_OK) {\n"
          "          typed." << field_name << "(temp);\n"
          "        }\n"
          "        return rc;\n";
      }
    }

    be_global->impl_ <<
      "      }\n";
  }

  bool generate_raw_adapter_access(
    AST_Decl* node, bool set, const std::string* use_scoped_name = 0)
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

    const std::string name = use_scoped_name ? *use_scoped_name : scoped(node->name());
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

    const std::string wrapped_et_al = "void* wrapped, DDS::DynamicType* type, DDS::MemberId id";
    const std::string op = set ? "set" : "get";
    be_global->impl_ << "  DDS::ReturnCode_t " << op << "_value(";
    if (set) {
      be_global->impl_ << wrapped_et_al << ", const void* source, DDS::TypeKind tk";
    } else {
      be_global->impl_ << "void* dest, DDS::TypeKind tk, " << wrapped_et_al;
    }
    be_global->impl_ << ") const\n"
      "  {\n"
      "    " << name << "& typed = *static_cast<" << name << "*>(wrapped);\n";

    if (struct_node || union_node) {
      if (union_node && set) {
        be_global->impl_ <<
          "    DDS::ReturnCode_t rc;\n";
      }
      be_global->impl_ <<
        "    ACE_UNUSED_ARG(typed);\n"
        "    switch (id) {\n";

      // Union Discriminator
      if (union_node) {
        generate_raw_adapter_access_field(
          union_node, set, op, OpenDDS::XTypes::DISCRIMINATOR_ID, union_node->disc_type(), "_d");
      }

      const Fields fields(union_node ? union_node : struct_node);
      for (Fields::Iterator i = fields.begin(); i != fields.end(); ++i) {
        AST_Field* const field = *i;
        generate_raw_adapter_access_field(
          union_node, set, op,
          be_global->get_id(field), field->field_type(), field->local_name()->get_string(),
          field);
      }

      be_global->impl_ <<
        "    default:\n"
        "      return invalid_id(\"" << name << "\", " << (set ? "true" : "false") << ", id);\n"
        "    }\n";
    } else if (seq_node || array_node) {
      be_global->impl_ <<
        "    const DDS::UInt32 limit = ";
      if (seq_node) {
        be_global->impl_ << "typed." << (use_cxx11 ? "size" : "length") << "()";
      } else {
        be_global->impl_ << array_total_count(array_node);
      }
      be_global->impl_ << ";\n"
        "    if (id >= limit) {\n"
        "      if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Notice) {\n"
        "        ACE_ERROR((LM_NOTICE, \"(%P|%t) NOTICE: RawAdapterImpl<" <<
          name << ">::" << op << "_value: \"\n"
        "          \"index %u is out of bounds %u\\n\", id, limit));\n"
        "      }\n"
        "      return DDS::RETCODE_BAD_PARAMETER;\n"
        "    }\n";

      AST_Type* const base_type = seq_node ? seq_node->base_type() : array_node->base_type();
      std::string extra;
      const char* const suffix = raw_adapter_suffix(base_type, set, extra);
      if (!suffix) {
        be_util::misc_error_and_abort(
          "Unsupported array orseq base type in raw_adapter_suffix", node);
      }
      std::string args;
      if (seq_node) {
        args = "typed[id]";
      } else {
        args = "(&typed";
        for (ACE_CDR::ULong dim = array_node->n_dims(); dim; --dim) {
          args += "[0]";
        }
        args += ")[id]";
      }
      args += extra + ", type, id";
      be_global->impl_ <<
        "    return " << op << "_" << suffix << "_value(";
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

  bool generate_raw_adapter(AST_Decl* node, const std::string* use_scoped_name = 0)
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
    const std::string name = use_scoped_name ? *use_scoped_name : scoped(node->name());
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
            if (!generate_raw_adapter(af.type_, &af.scoped_type_)) {
              be_util::misc_error_and_abort(
                "Failed to generate raw adapter for anonymous type of field", field);
            }
          }
        }
      }
    }

    std::vector<std::string> ns;
    ns.push_back("OpenDDS");
    ns.push_back("XTypes");
    NamespaceGuard ng(true, &ns);

    const std::string ra = "OpenDDS::XTypes::RawAdapter";

    std::string export_macro = be_global->export_macro().c_str();
    if (export_macro.size()) {
      export_macro = " " + export_macro;
    }

    const std::string get_ra_decl_first = "template <>";
    const std::string get_ra_decl_second =
      std::string("\nconst ") + ra + "& get_raw_adapter<" + name + ">()";
    be_global->header_ << get_ra_decl_first << export_macro << get_ra_decl_second << ";\n\n";

    if (generate) {
      be_global->impl_ <<
        "template <>\n"
        "class RawAdapterImpl<" << name << "> : public " << ra << " {\n"
        "public:\n"
        "  DDS::UInt32 get_item_count(const void* wrapped) const\n"
        "  {\n"
        "    const " << name << "& typed = *static_cast<const " << name << "*>(wrapped);\n"
        "    ACE_UNUSED_ARG(typed);\n"
        "    return ";
      if (struct_node) {
        be_global->impl_ << struct_node->nfields();
      } else if (union_node) {
        be_global->impl_ << union_node->nfields() + 1;
      } else if (seq_node) {
        be_global->impl_ << "typed." << (use_cxx11 ? "size" : "length") << "()";
      } else if (array_node) {
        be_global->impl_ << array_total_count(array_node);
      }
      be_global->impl_ << ";\n"
        "  }\n"
        "\n";

      if (seq_node || array_node) {
        be_global->impl_ <<
          "  DDS::MemberId get_member_id_at_index(void* wrapped, DDS::UInt32 index) const\n"
          "  {\n";
        if (seq_node) {
          be_global->impl_ <<
            "    " << name << "& typed = *static_cast<" << name << "*>(wrapped);\n"
            "    if (index >= typed." << (use_cxx11 ? "size" : "length") << "()) {\n"
            "      typed." << (use_cxx11 ? "resize" : "length") << "(index + 1);\n"
            "    }\n"
            "    return index;\n";
        } else if (array_node) {
          be_global->impl_ <<
            "    ACE_UNUSED_ARG(wrapped);\n"
            "    return index >= " << array_total_count(array_node)
              << " ? MEMBER_ID_INVALID : index;\n";
        }
        be_global->impl_ <<
          "  }\n"
          "\n";
      }

      if (!generate_raw_adapter_access(node, false, use_scoped_name)) {
        return false;
      }
      be_global->impl_ << "\n";
      if (!generate_raw_adapter_access(node, true, use_scoped_name)) {
        return false;
      }

      be_global->impl_ <<
        "};\n"
        "\n";

      be_global->impl_ <<
        get_ra_decl_first << export_macro << get_ra_decl_second << "\n"
        "{\n"
        "  static " << ra << "Impl<" << name << "> adapter;\n"
        "  return adapter;\n"
        "}\n"
        "\n";
    }

    return true;
  }
}

bool
metaclass_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char*)
{
  NamespaceGuard ng;
  std::string array_decl = "const char* gen_" + scoped_helper(name, "_") + "_names[]";
  std::string size_decl = "const size_t gen_" + scoped_helper(name, "_") + "_names_size";
  std::string decl_prefix = ((be_global->export_macro() == "") ? std::string("extern ") : (std::string(be_global->export_macro().c_str()) + " extern "));
  be_global->header_ << decl_prefix << array_decl << ";\n";
  be_global->header_ << decl_prefix << size_decl << ";\n";
  be_global->impl_ << array_decl << " = {\n";
  for (size_t i = 0; i < contents.size(); ++i) {
    be_global->impl_ << "  \"" << canonical_name(contents[i])
      << ((i < contents.size() - 1) ? "\",\n" : "\"\n");
  }
  be_global->impl_ << "};\n";
  be_global->impl_ << size_decl << " = " << contents.size() << ";\n";
  return true;
}

namespace {

  void
  delegateToNested(const std::string& fieldName, AST_Field* field,
    const std::string& firstArg)
  {
    const size_t n = fieldName.size() + 1 /* 1 for the dot */;
    const std::string fieldType = scoped(field->field_type()->name());
    be_global->impl_ <<
      "    if (std::strncmp(field, \"" << fieldName << ".\", " << n
      << ") == 0) {\n"
      "      return getMetaStruct<" << fieldType << ">().getValue("
      << firstArg << ", field + " << n << ");\n"
      "    }\n";
  }

  void
  gen_field_getValueByMemberId(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const Classification cls = classify(field->field_type());
    const OpenDDS::XTypes::MemberId id = be_global->get_id(field);
    const std::string fieldName = field->local_name()->get_string();
    if (cls & CL_SCALAR) {
      std::string prefix, suffix;
      if (cls & CL_ENUM) {
        AST_Type* enum_type = resolveActualType(field->field_type());
        prefix = "gen_" +
          dds_generator::scoped_helper(enum_type->name(), "_")
          + "_names[";
        if (use_cxx11) {
          prefix += "static_cast<int>(";
        }
        suffix = use_cxx11 ? "())]" : "]";
      } else if (use_cxx11) {
        suffix += "()";
      }
      const std::string string_to_ptr = use_cxx11 ? "" : ".in()";
      be_global->impl_ <<
        "    if (" << id << " == memberId) {\n"
        "      return " + prefix + "typed." + fieldName
        + (cls & CL_STRING ? string_to_ptr : "") + suffix + ";\n"
        "    }\n";
    }
  }

  void
  gen_field_getValue(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const Classification cls = classify(field->field_type());
    const std::string fieldName = field->local_name()->get_string();
    const std::string idl_name = canonical_name(field);
    if (cls & CL_SCALAR) {
      std::string prefix, suffix;
      if (cls & CL_ENUM) {
        AST_Type* enum_type = resolveActualType(field->field_type());
        prefix = "gen_" +
          dds_generator::scoped_helper(enum_type->name(), "_")
          + "_names[";
        if (use_cxx11) {
          prefix += "static_cast<int>(";
        }
        suffix = use_cxx11 ? "())]" : "]";
      } else if (use_cxx11) {
        suffix += "()";
      }
      const std::string string_to_ptr = use_cxx11 ? "" : ".in()";
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << idl_name << "\") == 0) {\n"
        "      return " + prefix + "typed." + fieldName
        + (cls & CL_STRING ? string_to_ptr : "") + suffix + ";\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_STRUCTURE) {
      delegateToNested(idl_name, field,
                       "&typed." + std::string(use_cxx11 ? "_" : "") + fieldName);
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    }
  }

  void
  gen_field_createQC(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    Classification cls = classify(field->field_type());
    const std::string fieldName = field->local_name()->get_string();
    const std::string idl_name = canonical_name(field);
    if (cls & CL_SCALAR) {
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << idl_name << "\") == 0) {\n"
        "      return make_field_cmp(&T::" << (use_cxx11 ? "_" : "")
        << fieldName << ", next);\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_STRUCTURE) {
      const size_t n = idl_name.size() + 1 /* 1 for the dot */;
      std::string fieldType = scoped(field->field_type()->name());
      be_global->impl_ <<
        "    if (std::strncmp(field, \"" << idl_name << ".\", " << n <<
        ") == 0) {\n"
        "      return make_struct_cmp(&T::" << (use_cxx11 ? "_" : "")
        << fieldName <<
        ", getMetaStruct<" << fieldType << ">().create_qc_comparator("
        "field + " << n << "), next);\n"
        "    }\n";
    }
  }

  void
  print_field_name(AST_Field* field)
  {
    be_global->impl_ << '"' << canonical_name(field) << '"' << ", ";
  }

  void
  get_raw_field(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const char* fieldName = field->local_name()->get_string();
    const std::string idl_name = canonical_name(field);
    be_global->impl_ <<
      "    if (std::strcmp(field, \"" << idl_name << "\") == 0) {\n"
      "      return &static_cast<const T*>(stru)->" << (use_cxx11 ? "_" : "")
      << fieldName << ";\n"
      "    }\n";
    be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
  }

  void
  assign_field(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    Classification cls = classify(field->field_type());
    if (!cls) return; // skip CL_UNKNOWN types
    std::string fieldType = (cls & CL_STRING) ?
      string_type(cls) : scoped(field->field_type()->name());
    FieldInfo af(*field);
    if (af.as_base_ && af.type_->anonymous()) {
      fieldType = af.scoped_type_;
    }
    const std::string idl_name = canonical_name(field);
    if ((cls & (CL_SCALAR | CL_STRUCTURE | CL_SEQUENCE | CL_UNION))
        || (use_cxx11 && (cls & CL_ARRAY))) {
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << idl_name << "\") == 0) {\n"
        "      static_cast<T*>(lhs)->" << (use_cxx11 ? "_" : "") << af.name_ <<
        " = *static_cast<const " << fieldType <<
        "*>(rhsMeta.getRawField(rhs, rhsFieldSpec));\n"
        "      return;\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_ARRAY) {
      AST_Type* unTD = resolveActualType(field->field_type());
      AST_Array* arr = dynamic_cast<AST_Array*>(unTD);
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << idl_name << "\") == 0) {\n"
        "      " << fieldType << "* lhsArr = &static_cast<T*>(lhs)->" << af.name_ << ";\n"
        "      const " << fieldType << "* rhsArr = static_cast<const " <<
        fieldType << "*>(rhsMeta.getRawField(rhs, rhsFieldSpec));\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
      AST_Type* elem = arr->base_type();
      AST_Type* elemUnTD = resolveActualType(elem);
      if (classify(elemUnTD) & CL_ARRAY) {
        // array-of-array case, fall back on the Serializer
        be_global->impl_ <<
          "      " << fieldType << "_forany rhsForany(const_cast<" <<
          fieldType << "_slice*>(*rhsArr));\n"
          // TODO(iguessthislldo) I'm not 100% certain this will always work
          "      const Encoding encoding(Encoding::KIND_UNALIGNED_CDR);\n"
          "      ACE_Message_Block mb(serialized_size(encoding, rhsForany));\n"
          "      Serializer ser_out(&mb, encoding);\n"
          "      ser_out << rhsForany;\n"
          "      " << fieldType << "_forany lhsForany(*lhsArr);\n"
          "      Serializer ser_in(&mb, encoding);\n"
          "      ser_in >> lhsForany;\n";
      } else {
        std::string indent = "      ";
        NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
        be_global->impl_ <<
          indent << "(*lhsArr)" << nfl.index_ << " = (*rhsArr)" <<
          nfl.index_ << ";\n";
      }
      be_global->impl_ <<
        "      return;\n"
        "    }\n";
    }
  }

  void
  compare_field(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    Classification cls = classify(field->field_type());
    if (!(cls & CL_SCALAR)) return;
    const char* fieldName = field->local_name()->get_string();
    const std::string idl_name = canonical_name(field);
    be_global->impl_ <<
      "    if (std::strcmp(field, \"" << idl_name << "\") == 0) {\n";
    be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    if (!use_cxx11 && (cls & CL_STRING)) {
      be_global->impl_ << // ACE_OS::strcmp has overloads for narrow & wide
        "      return 0 == ACE_OS::strcmp(static_cast<const T*>(lhs)->"
        << fieldName << ".in(), static_cast<const T*>(rhs)->" << fieldName
        << ".in());\n";
    } else {
      be_global->impl_ <<
        "      return static_cast<const T*>(lhs)->" << (use_cxx11 ? "_" : "")
        << fieldName << " == static_cast<const T*>(rhs)->" << (use_cxx11 ? "_" : "")
        << fieldName << ";\n";
    }
    be_global->impl_ << "    }\n";
  }

  bool
  generate_metaclass(AST_Decl* node, UTL_ScopedName* name,
    const std::vector<AST_Field*>& fields, bool& first_struct_,
    const std::string& clazz)
  {
    AST_Structure* struct_node = 0;
    AST_Union* union_node = 0;
    if (!node || !name) {
      return false;
    }
    if (node->node_type() == AST_Decl::NT_struct) {
      struct_node = dynamic_cast<AST_Structure*>(node);
    } else if (node->node_type() == AST_Decl::NT_union) {
      union_node = dynamic_cast<AST_Union*>(node);
    } else {
      return false;
    }
    const std::string type_idl_name = canonical_name(name);

    if (first_struct_) {
      be_global->header_ <<
        "class MetaStruct;\n\n"
        "template<typename T>\n"
        "const MetaStruct& getMetaStruct();\n\n";
      first_struct_ = false;
    }

    be_global->add_include("dds/DCPS/FilterEvaluator.h",
      BE_GlobalData::STREAM_CPP);

    std::string decl = "const MetaStruct& getMetaStruct<" + clazz + ">()",
      exp = be_global->export_macro().c_str();
    be_global->header_ << "template<>\n" << exp << (exp.length() ? "\n" : "")
      << decl << ";\n";

    size_t key_count = 0;
    IDL_GlobalData::DCPS_Data_Type_Info* info = 0;
    const bool is_topic_type = be_global->is_topic_type(node);
    if (struct_node) {
      info = idl_global->is_dcps_type(name);
      if (is_topic_type) {
        key_count = TopicKeys(struct_node).count();
      } else if (info) {
        key_count = info->key_list_.size();
      }
    } else { // Union
      key_count = be_global->union_discriminator_is_key(union_node) ? 1 : 0;
    }

    const std::string exception =
      "throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \" not "
      "found or its type is not supported (in struct" + clazz + ")\");\n";

    be_global->impl_ <<
      "template<>\n"
      "struct MetaStructImpl<" << clazz << "> : MetaStruct {\n"
      "  typedef " << clazz << " T;\n\n"
      "#ifndef OPENDDS_NO_MULTI_TOPIC\n"
      "  void* allocate() const { return new T; }\n\n"
      "  void deallocate(void* stru) const { delete static_cast<T*>(stru); }\n\n"
      "  size_t numDcpsKeys() const { return " << key_count << "; }\n\n"
      "#endif /* OPENDDS_NO_MULTI_TOPIC */\n\n";
    if (struct_node && be_global->extensibility(struct_node) == extensibilitykind_mutable) {
      be_global->impl_ <<
        "  ACE_CDR::ULong map_name_to_id(const char* field) const\n"
        "  {\n"
        "    static const std::pair<std::string, ACE_CDR::ULong> name_to_id_pairs[] = {\n";
      for (size_t i = 0; i < fields.size(); ++i) {
        be_global->impl_ << "      std::make_pair(\"" << canonical_name(fields[i]) << "\", " <<
          be_global->get_id(fields[i]) << "),\n";
      }
      be_global->impl_ <<
        "    };\n"
        "    static const std::map<std::string, ACE_CDR::ULong> name_to_id_map(name_to_id_pairs,"
        " name_to_id_pairs + " << fields.size() << ");\n"
        "    std::map<std::string, ACE_CDR::ULong>::const_iterator it = name_to_id_map.find(field);\n"
        "    if (it == name_to_id_map.end()) {\n"
        "      " << exception <<
        "    } else {\n"
        "      return it->second;\n"
        "    }\n"
        "  }\n\n";
    }
    be_global->impl_ <<
      "  Value getValue(const void* stru, DDS::MemberId memberId) const\n"
      "  {\n"
      "    ACE_UNUSED_ARG(memberId);\n"
      "    const" << clazz << "& typed = *static_cast<const" << clazz << "*>(stru);\n"
      "    ACE_UNUSED_ARG(typed);\n";
    std::for_each(fields.begin(), fields.end(), gen_field_getValueByMemberId);
    be_global->impl_ <<
      "    " << "throw std::runtime_error(\"Member not found or its type is not supported "
        "(in type" << type_idl_name << ")\");\n"
      "  }\n\n"
      "  Value getValue(const void* stru, const char* field) const\n"
      "  {\n"
      "    const" << clazz << "& typed = *static_cast<const" << clazz << "*>(stru);\n"
      "    ACE_UNUSED_ARG(typed);\n";
    std::for_each(fields.begin(), fields.end(), gen_field_getValue);
    be_global->impl_ <<
      "    " << exception <<
      "  }\n\n";
    if (struct_node) {
      marshal_generator::gen_field_getValueFromSerialized(struct_node, clazz);
    } else {
      be_global->impl_ <<
        "  Value getValue(Serializer& ser, const char* field, const TypeSupportImpl* = 0) const\n"
        "  {\n"
        "    ACE_UNUSED_ARG(ser);\n"
        "    if (!field[0]) {\n"   // if 'field' is the empty string...
        "      return 0;\n"        // ...we've skipped the entire struct
        "    }\n"                  //    and the return value is ignored
        "    throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \" not "
        "valid for union" << type_idl_name << "\");\n"
        "  }\n\n";
    }
    be_global->impl_ <<
      "  ComparatorBase::Ptr create_qc_comparator(const char* field, "
      "ComparatorBase::Ptr next) const\n"
      "  {\n"
      "    ACE_UNUSED_ARG(next);\n";
    be_global->add_include("<stdexcept>", BE_GlobalData::STREAM_CPP);
    if (struct_node) {
      std::for_each(fields.begin(), fields.end(), gen_field_createQC);
    }
    be_global->impl_ <<
      "    " << exception <<
      "  }\n\n"
      "#ifndef OPENDDS_NO_MULTI_TOPIC\n"
      "  const char** getFieldNames() const\n"
      "  {\n"
      "    static const char* names[] = {";
    if (struct_node) {
      std::for_each(fields.begin(), fields.end(), print_field_name);
    }
    be_global->impl_ <<
      "0};\n"
      "    return names;\n"
      "  }\n\n"
      "  const void* getRawField(const void* stru, const char* field) const\n"
      "  {\n";
    if (struct_node && fields.size()) {
      std::for_each(fields.begin(), fields.end(), get_raw_field);
    } else {
      be_global->impl_ << "    ACE_UNUSED_ARG(stru);\n";
    }
    be_global->impl_ <<
      "    " << exception <<
      "  }\n\n"
      "  void assign(void* lhs, const char* field, const void* rhs,\n"
      "    const char* rhsFieldSpec, const MetaStruct& rhsMeta) const\n"
      "  {\n"
      "    ACE_UNUSED_ARG(lhs);\n"
      "    ACE_UNUSED_ARG(field);\n"
      "    ACE_UNUSED_ARG(rhs);\n"
      "    ACE_UNUSED_ARG(rhsFieldSpec);\n"
      "    ACE_UNUSED_ARG(rhsMeta);\n";
    if (struct_node) {
      std::for_each(fields.begin(), fields.end(), assign_field);
    }
    be_global->impl_ <<
      "    " << exception <<
      "  }\n"
      "\n"
      "  bool compare(const void* lhs, const void* rhs, const char* field) "
      "const\n"
      "  {\n"
      "    ACE_UNUSED_ARG(lhs);\n"
      "    ACE_UNUSED_ARG(field);\n"
      "    ACE_UNUSED_ARG(rhs);\n";
    if (struct_node) {
      std::for_each(fields.begin(), fields.end(), compare_field);
    }
    be_global->impl_ <<
      "    " << exception <<
      "  }\n"
      "#endif /* OPENDDS_NO_MULTI_TOPIC */\n\n"
      "};\n\n"
      "template<>\n"
      << decl << "\n"
      "{\n"
      "  static MetaStructImpl<" << clazz << "> msi;\n"
      "  return msi;\n"
      "}\n\n";
    return true;
  }
}

bool
metaclass_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE, const char*)
{
  const std::string clazz = scoped(name);
  ContentSubscriptionGuard csg;
  be_global->add_include("dds/DCPS/PoolAllocator.h",
    BE_GlobalData::STREAM_CPP);

  if (!generate_raw_adapter(node)) {
    return false;
  }

  NamespaceGuard ng;

  if (!generate_metaclass(node, name, fields, first_struct_, clazz)) {
    return false;
  }

  FieldInfo::EleLenSet anonymous_seq_generated;
  for (size_t i = 0; i < fields.size(); ++i) {
    if (fields[i]->field_type()->anonymous()) {
      FieldInfo af(*fields[i]);
      if (af.arr_ || (af.seq_ && af.is_new(anonymous_seq_generated))) {
        Function f("gen_skip_over", "bool");
        f.addArg("ser", "Serializer&");
        f.addArg("", af.ptr_);
        f.endArgs();

        AST_Type* elem;
        if (af.seq_ != 0) {
          elem = af.seq_->base_type();
        } else {
          elem = af.arr_->base_type();
        }
        be_global->impl_ <<
          "  const Encoding& encoding = ser.encoding();\n"
          "  ACE_UNUSED_ARG(encoding);\n";
        Classification elem_cls = classify(elem);
        const bool primitive = elem_cls & CL_PRIMITIVE;
        marshal_generator::generate_dheader_code(
          "    if (!ser.read_delimiter(total_size)) {\n"
          "      return false;\n"
          "    }\n", !primitive, true);

        std::string len;
        if (af.arr_) {
          std::ostringstream strstream;
          strstream << array_element_count(af.arr_);
          len = strstream.str();
        } else { // Sequence
          be_global->impl_ <<
            "  ACE_CDR::ULong length;\n"
            "  if (!(ser >> length)) return false;\n";
          len = "length";
        }
        const std::string cxx_elem = scoped(elem->name());
        AST_Type* elem_orig = elem;
        elem = resolveActualType(elem);
        elem_cls = classify(elem);

        if ((elem_cls & (CL_PRIMITIVE | CL_ENUM))) {
          // fixed-length sequence/array element -> skip all elements at once
          size_t sz = 0;
          to_cxx_type(af.as_act_, sz);
          be_global->impl_ <<
            "  return ser.skip(" << af.length_ << ", " << sz << ");\n";
        } else {
          be_global->impl_ <<
            "  for (ACE_CDR::ULong i = 0; i < " << len << "; ++i) {\n";
          if (elem_cls & CL_STRING) {
            be_global->impl_ <<
              "    ACE_CDR::ULong strlength;\n"
              "    if (!(ser >> strlength && ser.skip(strlength))) return false;\n";
          } else if (elem_cls & (CL_ARRAY | CL_SEQUENCE | CL_STRUCTURE)) {
            std::string pre, post;
            const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
            if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
              post = "_forany";
            } else if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
              pre = "IDL::DistinctType<";
              post = ", " + dds_generator::get_tag_name(dds_generator::scoped_helper(elem_orig->name(), "_")) + ">";
            }
            be_global->impl_ <<
              "    if (!gen_skip_over(ser, static_cast<" << pre << cxx_elem << post
              << "*>(0))) return false;\n";
          }
          be_global->impl_ <<
            "  }\n"
            "  return true;\n";
        }

      }
    }
  }

  {
    Function f("gen_skip_over", "bool");
    f.addArg("ser", "Serializer&");
    f.addArg("", clazz + "*");
    f.endArgs();
    be_global->impl_ <<
      "  MetaStructImpl<" << clazz << ">().getValue(ser, \"\");\n"
      "  return true;\n";
  }
  return true;
}

bool
metaclass_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name,
  AST_Type* type, const char*)
{
  AST_Array* arr = dynamic_cast<AST_Array*>(type);
  AST_Sequence* seq = 0;
  if (!arr && !(seq = dynamic_cast<AST_Sequence*>(type))) {
    return true;
  }
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  const std::string clazz = scoped(name);

  ContentSubscriptionGuard csg;

  if (!generate_raw_adapter(type, &clazz)) {
    return false;
  }

  NamespaceGuard ng;
  Function f("gen_skip_over", "bool");
  f.addArg("ser", "Serializer&");
  if (use_cxx11) {
    f.addArg("", "IDL::DistinctType<" + clazz + ", " + dds_generator::get_tag_name(clazz) +">*");
  } else {
    f.addArg("", clazz + (arr ? "_forany*" : "*"));
  }
  f.endArgs();

  AST_Type* elem;
  if (seq != 0) {
    elem = seq->base_type();
  } else {
    elem = arr->base_type();
  }
  be_global->impl_ <<
    "  const Encoding& encoding = ser.encoding();\n"
    "  ACE_UNUSED_ARG(encoding);\n";
  Classification elem_cls = classify(elem);
  const bool primitive = elem_cls & CL_PRIMITIVE;
  marshal_generator::generate_dheader_code(
    "    if (!ser.read_delimiter(total_size)) {\n"
    "      return false;\n"
    "    }\n", !primitive, true);

  std::string len;

  if (arr) {
    std::ostringstream strstream;
    strstream << array_element_count(arr);
    len = strstream.str();
  } else { // Sequence
    be_global->impl_ <<
      "  ACE_CDR::ULong length;\n"
      "  if (!(ser >> length)) return false;\n";
    len = "length";
  }

  const std::string cxx_elem = scoped(elem->name());
  AST_Type* elem_orig = elem;
  elem = resolveActualType(elem);
  elem_cls = classify(elem);

  if ((elem_cls & (CL_PRIMITIVE | CL_ENUM))) {
    // fixed-length sequence/array element -> skip all elements at once
    size_t sz = 0;
    to_cxx_type(elem, sz);
    be_global->impl_ <<
      "  return ser.skip(" << len << ", " << sz << ");\n";
  } else {
    be_global->impl_ <<
      "  for (ACE_CDR::ULong i = 0; i < " << len << "; ++i) {\n";
    if (elem_cls & CL_STRING) {
      be_global->impl_ <<
        "    ACE_CDR::ULong strlength;\n"
        "    if (!(ser >> strlength && ser.skip(strlength))) return false;\n";
    } else if (elem_cls & (CL_ARRAY | CL_SEQUENCE | CL_STRUCTURE)) {
      std::string pre, post;
      if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
        post = "_forany";
      } else if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
        pre = "IDL::DistinctType<";
        post = ", " + dds_generator::get_tag_name(scoped_helper(elem_orig->name(), "::")) + ">";
      }
      be_global->impl_ <<
        "    if (!gen_skip_over(ser, static_cast<" << pre << cxx_elem << post
        << "*>(0))) return false;\n";
    }
    be_global->impl_ <<
      "  }\n"
      "  return true;\n";
  }

  return true;
}

static std::string
func(const std::string&, AST_Decl*, const std::string&, AST_Type* br_type, const std::string&,
  bool, Intro&, const std::string&)
{
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  std::stringstream ss;
  const Classification br_cls = classify(br_type);
  ss <<
    "    if (is_mutable && !ser.read_parameter_id(member_id, field_size, must_understand)) {\n"
    "      return false;\n"
    "    }\n";
  if (br_cls & CL_STRING) {
    ss <<
      "    ACE_CDR::ULong len;\n"
      "    if (!(ser >> len && ser.skip(len))) return false;\n";
  } else if (br_cls & CL_SCALAR) {
    size_t sz = 0;
    to_cxx_type(br_type, sz);
    ss <<
      "    if (!ser.skip(1, " << sz << ")) return false;\n";
  } else {
    std::string pre, post;
    if (!use_cxx11 && (br_cls & CL_ARRAY)) {
      post = "_forany";
    } else if (use_cxx11 && (br_cls & (CL_ARRAY | CL_SEQUENCE))) {
      pre = "IDL::DistinctType<";
      post = ", " + dds_generator::get_tag_name(dds_generator::scoped_helper(br_type->name(), "::")) + ">";
    }
    ss <<
      "    if (!gen_skip_over(ser, static_cast<" << pre
      << scoped(br_type->name()) << post << "*>(0))) return false;\n";
  }

  ss <<
    "    return true;\n";
  return ss.str();
}

bool
metaclass_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
  const char*)
{
  const std::string clazz = scoped(name);
  ContentSubscriptionGuard csg;

  if (!generate_raw_adapter(node)) {
    return false;
  }

  NamespaceGuard ng;

  std::vector<AST_Field*> dummy_field_list;
  if (!generate_metaclass(node, name, dummy_field_list, first_struct_, clazz)) {
    return false;
  }

  {
    Function f("gen_skip_over", "bool");
    f.addArg("ser", "Serializer&");
    f.addArg("", clazz + "*");
    f.endArgs();

    const ExtensibilityKind exten = be_global->extensibility(node);
    const bool not_final = exten != extensibilitykind_final;
    const bool is_mutable  = exten == extensibilitykind_mutable;
    be_global->impl_ <<
      "  const Encoding& encoding = ser.encoding();\n"
      "  ACE_UNUSED_ARG(encoding);\n"
      "  const bool is_mutable = " << is_mutable <<";\n"
      "  unsigned member_id;\n"
      "  size_t field_size;\n"
      "  bool must_understand = false;\n";
    marshal_generator::generate_dheader_code(
      "    if (!ser.read_delimiter(total_size)) {\n"
      "      return false;\n"
      "    }\n", not_final);
    if (is_mutable) {
      be_global->impl_ <<
        "  if (!ser.read_parameter_id(member_id, field_size, must_understand)) {\n"
        "    return false;\n"
        "  }\n";
    }
    be_global->impl_ <<
      "  " << scoped(discriminator->name()) << " disc;\n"
      "  if (!(ser >> " << getWrapper("disc", discriminator, WD_INPUT) << ")) {\n"
      "    return false;\n"
      "  }\n";
    if (generateSwitchForUnion(node, "disc", func, branches, discriminator, "", "", "",
                               false, true, false)) {
      be_global->impl_ <<
        "  return true;\n";
    }
  }
  return true;
}
