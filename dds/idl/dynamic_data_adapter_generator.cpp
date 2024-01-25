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
      : PreprocessorIfGuard(" OPENDDS_HAS_DYNAMIC_DATA_ADAPTER", true, false)
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

  std::string op(bool set)
  {
    return set ? "set" : "get";
  }

  bool access_details(bool set, AST_Decl* node, AST_Type* type, std::string& op_type,
    std::string& extra_access, bool& is_complex)
  {
    const bool cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    AST_Type* const t = resolveActualType(type);
    AST_Type* const node_as_type = dynamic_cast<AST_Type*>(node);
    switch (t->node_type()) {
    case AST_Decl::NT_pre_defined:
      if (set && cxx11 && node_as_type &&
          resolveActualType(node_as_type)->node_type() == AST_Decl::NT_sequence &&
          dynamic_cast<AST_PredefinedType*>(t)->pt() == AST_PredefinedType::PT_boolean) {
        // setting a std::vector<bool> requires a special case
        op_type = "bool_vector_elem";
      } else {
        op_type = "simple";
      }
      return true;
    case AST_Decl::NT_enum:
      op_type = "enum";
      return true;
    case AST_Decl::NT_string:
      if (cxx11) {
        op_type = "cpp11_s8";
      } else {
        if (set) {
          extra_access = ".inout()";
        }
        op_type = "s8";
      }
      return true;
    case AST_Decl::NT_wstring:
      if (cxx11) {
        op_type = "cpp11_s16";
      } else {
        if (set) {
          extra_access = ".inout()";
        }
        op_type = "s16";
      }
      return true;
    case AST_Decl::NT_struct:
    case AST_Decl::NT_union:
    case AST_Decl::NT_sequence:
      is_complex = true;
      op_type = set ? "direct_complex" : "complex";
      return true;
    case AST_Decl::NT_array:
      is_complex = true;
      if (set) {
        op_type = cxx11 ? "direct_complex" : "indirect_complex";
      } else {
        op_type = "complex";
      }
      return true;
    default:
      return false;
    }
  }

  void generate_op(unsigned indent, bool set, AST_Type* type, const std::string& type_name,
    const std::string& op_type, bool is_complex, const std::string& value,
    const char* rc_dest = "return")
  {
    RefWrapper type_wrapper(type, type_name, "");
    type_wrapper.dynamic_data_adapter_ = true;
    type_wrapper.done();
    std::string template_params;
    if (type_wrapper.needs_dda_tag()) {
      template_params = "<" + type_name + ", " + type_wrapper.get_tag_name() + ">";
    } else if (is_complex) {
      template_params = "<" + type_name + ", " + type_name + ">";
    }

    be_global->impl_ <<
      std::string(indent * 2, ' ') << rc_dest << " " <<
      op(set) << "_" << op_type << "_raw_value" << template_params << "(method, ";
    if (set) {
      be_global->impl_ << value << ", id, source, tk";
    } else {
      be_global->impl_ << "dest, tk, " << value << ", id";
    }
    be_global->impl_ << ");\n";
  }

  void generate_dynamic_data_adapter_access_field(
    AST_Union* union_node, bool set,
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
    be_global->impl_ << ":\n"
      "      {\n";

    std::string op_type;
    std::string extra_access;
    bool is_complex = false;
    if ((field && !be_global->dynamic_data_adapter(field->field_type())) ||
        !access_details(set, field, field_type, op_type, extra_access, is_complex)) {
      be_global->impl_ <<
        "        ACE_UNUSED_ARG(" << (set ? "source" : "dest") << ");\n"
        "        ACE_UNUSED_ARG(tk);\n"
        "        return missing_dda(method, id);\n";

    } else {
      const std::string type_name = field_type_name(field, field_type);
      std::string value = "value_.";
      const char* rc_dest = "return";
      if (union_node) {
        if (set) {
          // Setting a union branch or discriminator requires a temporary
          be_global->impl_ <<
            "        " << type_name << " temp;\n";
          rc_dest = "rc =";
          value = "temp";
        } else {
          value += cpp_field_name + "()";
          if (!use_cxx11 && classify(field_type) & CL_ARRAY) {
            // Unions return pointer to array elements, get reference to array.
            value = std::string("*reinterpret_cast<") + type_name + "*>(" + value + ")";
          }
        }
      } else {
        value += (use_cxx11 ? "_" : "") + cpp_field_name;
      }

      if (field != 0) {
        const bool is_optional = be_global->is_optional(field);
        if (is_optional) value += ".value()";
      }

      generate_op(4, set, field_type, type_name, op_type, is_complex,
        value + extra_access, rc_dest);
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

  bool generate_dynamic_data_adapter_access(AST_Decl* node, RefWrapper& wrapper, bool set)
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

    be_global->impl_ << "  DDS::ReturnCode_t " << op(set) << "_raw_value(const char* method, ";
    if (set) {
      be_global->impl_ << "DDS::MemberId id, const void* source, DDS::TypeKind tk";
    } else {
      be_global->impl_ << "void* dest, DDS::TypeKind tk, DDS::MemberId id";
    }

    be_global->impl_ << ")\n"
      "  {\n";

    if (set) {
      be_global->impl_ <<
        "    DDS::ReturnCode_t rc = assert_mutable(method);\n"
        "    if (rc != DDS::RETCODE_OK) {\n"
        "      return rc;\n"
        "    }\n";
    }

    if (struct_node || union_node) {
      be_global->impl_ <<
        "    switch (id) {\n";

      if (union_node) {
        generate_dynamic_data_adapter_access_field(
          union_node, set, OpenDDS::XTypes::DISCRIMINATOR_ID, union_node->disc_type());
      }

      const Fields fields(union_node ? union_node : struct_node);
      for (Fields::Iterator i = fields.begin(); i != fields.end(); ++i) {
        AST_Field* const field = *i;
        generate_dynamic_data_adapter_access_field(
          union_node, set, be_global->get_id(field), field->field_type(), field);
      }

      be_global->impl_ <<
        "    default:\n"
        "      return invalid_id(method, id);\n"
        "    }\n";
    } else if (seq_node || array_node) {
      AST_Type* const base_type = seq_node ? seq_node->base_type() : array_node->base_type();
      AST_Type* const named_type = deepest_named_type(base_type);

      std::string op_type;
      std::string extra_access;
      bool is_complex = false;
      if (access_details(set, node, base_type, op_type, extra_access, is_complex)) {
        be_global->impl_ << "    ";
        if (!set) {
          be_global->impl_ << "const DDS::ReturnCode_t ";
        }
        be_global->impl_ << "rc = check_index(method, id, ";
        if (seq_node) {
          be_global->impl_ << wrapper.seq_get_length();
        } else {
          be_global->impl_ << array_element_count(array_node);
        }
        be_global->impl_ << ");\n"
          "    if (rc != DDS::RETCODE_OK) {\n"
          "      return rc;\n"
          "    }\n";
        generate_op(2, set, base_type, scoped(named_type->name()), op_type, is_complex,
          wrapper.flat_collection_access("id") + extra_access);
      } else {
        be_global->impl_ <<
          "    ACE_UNUSED_ARG(" << (set ? "source" : "dest") << ");\n"
          "    ACE_UNUSED_ARG(tk);\n"
          "    return missing_dda(method, id);\n";
      }
    } else {
      return false;
    }

    be_global->impl_ <<
      "  }\n";

    return true;
  }

  void generate_get_dynamic_data_adapter(
    const std::string& cpp_name, const std::string& tag, const std::string& export_macro,
    bool is_const, bool dda_generated)
  {
    be_global->header_ <<
      "template <>\n" <<
      export_macro << "DDS::DynamicData_ptr get_dynamic_data_adapter<" << cpp_name << tag <<
        ">(DDS::DynamicType_ptr type, " << (is_const ? "const " : "") << cpp_name << "& value);\n"
      "\n";

    be_global->impl_ <<
      "template <>\n"
      "DDS::DynamicData_ptr get_dynamic_data_adapter<" << cpp_name << tag <<
        ">(DDS::DynamicType_ptr type, " << (is_const ? "const " : "") << cpp_name << "& value)\n"
      "{\n";
    if (dda_generated) {
      be_global->impl_ <<
        "#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER\n"
        "  if (type) {\n"
        "    return new DynamicDataAdapterImpl<" << cpp_name << tag << ">(type, value);\n"
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

  template <typename Type>
  bool is_collection_of_interface_or_value_type(Type* type)
  {
    const AST_Decl::NodeType kind = resolveActualType(type->base_type())->node_type();
    return kind == AST_Decl::NT_interface || kind == AST_Decl::NT_valuetype;
  }

  bool generate_dynamic_data_adapter(
    AST_Decl* node, const std::string* use_scoped_name = 0, AST_Typedef* typedef_node = 0)
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
      if (is_collection_of_interface_or_value_type(seq_node)) {
        return true;
      }
      break;
    case AST_Decl::NT_array:
      array_node = dynamic_cast<AST_Array*>(node);
      if (is_collection_of_interface_or_value_type(array_node)) {
        return true;
      }
      break;
    default:
      return true;
    }
    const std::string cpp_name = use_scoped_name ? *use_scoped_name : scoped(node->name());
    const bool generate = be_global->dynamic_data_adapter(typedef_node ? typedef_node : node);
    RefWrapper wrapper(dynamic_cast<AST_Type*>(node), cpp_name, "value_", false);
    wrapper.dynamic_data_adapter_ = true;
    wrapper.done();

    be_global->add_include("dds/DCPS/XTypes/DynamicDataAdapter.h", BE_GlobalData::STREAM_H);

    if (struct_node || union_node) {
      const Fields fields(union_node ? union_node : struct_node);
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

    std::string tag;
    if (wrapper.needs_dda_tag()) {
      tag = ", " + wrapper.get_tag_name();
    } else {
      tag = ", " + cpp_name;
    }

    {
      NoSafetyProfileGuard nspg;
      wrapper.generate_tag();

      if (generate) {
        DynamicDataAdapterGuard ddag;

        std::string impl_classname = "DynamicDataAdapterImpl<" + cpp_name + tag + " >";
        be_global->impl_ <<
          "template <>\n"
          "class " << impl_classname << " : public DynamicDataAdapter_T<" << cpp_name << "> {\n"
          "public:\n"
          "  DynamicDataAdapterImpl(DDS::DynamicType_ptr type, " << cpp_name << "& value)\n"
          "    : DynamicDataAdapter_T<" << cpp_name << ">(type, value)\n"
          "  {\n"
          "  }\n"
          "\n"
          "  DynamicDataAdapterImpl(DDS::DynamicType_ptr type, const " << cpp_name << "& value)\n"
          "    : DynamicDataAdapter_T<" << cpp_name << ">(type, value)\n"
          "  {\n"
          "  }\n"
          "\n";
        if (struct_node || seq_node || array_node) {
          be_global->impl_ <<
            "  DDS::UInt32 get_item_count()\n"
            "  {\n"
            "    return ";
          if (struct_node) {
            be_global->impl_ << struct_node->nfields();
          } else if (seq_node) {
            be_global->impl_ << wrapper.seq_get_length();
          } else if (array_node) {
            be_global->impl_ << array_element_count(array_node);
          }
          be_global->impl_ << ";\n"
            "  }\n"
            "\n";
        }

        if (seq_node) {
          // TODO: Set new elements to default values?
          be_global->impl_ <<
            "  DDS::MemberId get_member_id_at_index_impl(DDS::UInt32 index)\n"
            "  {\n"
            "    const DDS::UInt32 count = " << wrapper.seq_get_length() << ";\n"
            "    if (!read_only_ && index >= count) {\n"
            "      " << wrapper.seq_resize("index + 1") <<
            "      return index;\n"
            "    }\n"
            "    return check_index(\"get_member_id_at_index\", index, count)"
            " == DDS::RETCODE_OK ? index : MEMBER_ID_INVALID;\n"
            "  }\n"
            "\n";
        }

        const bool is_topic_type = be_global->is_topic_type(node);
        AST_Type* const node_as_type = dynamic_cast<AST_Type*>(node);
        const bool forany = needs_forany(node_as_type);
        const bool distinct_type = needs_distinct_type(node_as_type);
        be_global->impl_ <<
          "  DDS::DynamicData_ptr clone()\n"
          "  {\n"
          "    return new DynamicDataAdapterImpl(type_, value_);\n"
          "  }\n"
          "\n"
          "  bool serialized_size(const OpenDDS::DCPS::Encoding& enc, size_t& size, OpenDDS::DCPS::Sample::Extent ext) const\n"
          "  {\n";
        if (struct_node || union_node) {
          be_global->impl_ <<
            "    using namespace OpenDDS::DCPS;\n"
            "    if (ext == Sample::Full) {\n"
            "      DCPS::serialized_size(enc, size, value_);\n"
            "    } else if (ext == Sample::NestedKeyOnly) {\n"
            "      const NestedKeyOnly<const " << cpp_name << "> nested_key_only(value_);\n"
            "      DCPS::serialized_size(enc, size, nested_key_only);\n"
            "    } else {\n";
          if (is_topic_type) {
            be_global->impl_ <<
              "      KeyOnly<const " << cpp_name << "> key_only(value_);\n"
              "      DCPS::serialized_size(enc, size, key_only);\n";
          } else {
            be_global->impl_ <<
              "      return false; // Non-topic type\n";
          }
          be_global->impl_ <<
            "    }\n"
            "    return true;\n";
        } else {
          be_global->impl_ <<
            "    ACE_UNUSED_ARG(ext);\n";
          if (distinct_type) { // sequence or array (C++11 mapping)
            RefWrapper distinct_type_wrapper(node_as_type, cpp_name, "");
            distinct_type_wrapper.done();
            be_global->impl_ <<
              "    using namespace OpenDDS::DCPS;\n"
              "    " << distinct_type_wrapper.wrapped_type_name() << " distinct_value(value_);\n"
              "    DCPS::serialized_size(enc, size, distinct_value);\n";
          } else if (forany) { // array only (classic mapping)
            be_global->impl_ <<
              "    OpenDDS::DCPS::serialized_size(enc, size, " << cpp_name << "_forany(value_));\n";
          } else {
            be_global->impl_ <<
              "    OpenDDS::DCPS::serialized_size(enc, size, value_);\n";
          }
          be_global->impl_ <<
            "    return true;\n";
        }

        be_global->impl_ <<
          "  }\n"
          "\n"
          "  bool serialize(OpenDDS::DCPS::Serializer& ser, OpenDDS::DCPS::Sample::Extent ext) const\n"
          "  {\n";
        if (struct_node || union_node) {
          be_global->impl_ <<
            "    using namespace OpenDDS::DCPS;\n"
            "    if (ext == Sample::Full) {\n"
            "      return ser << value_;\n"
            "    } else if (ext == Sample::NestedKeyOnly) {\n"
            "      NestedKeyOnly<const " << cpp_name << "> nested_key_only(value_);\n"
            "      return ser << nested_key_only;\n"
            "    } else {\n";
          if (is_topic_type) {
            be_global->impl_ <<
              "      KeyOnly<const " << cpp_name << "> key_only(value_);\n"
              "      return ser << key_only;\n";
          } else {
            be_global->impl_ <<
              "      return false; // Non-topic type\n";
          }
          be_global->impl_ <<
            "    }\n";
        } else {
          be_global->impl_ <<
            "    ACE_UNUSED_ARG(ext);\n"
            "    using namespace OpenDDS::DCPS;\n";
          if (distinct_type) {
            RefWrapper distinct_type_wrapper(node_as_type, cpp_name, "");
            distinct_type_wrapper.done();
            be_global->impl_ <<
              "    " << distinct_type_wrapper.wrapped_type_name() << " distinct_value(value_);\n"
              "    return ser << distinct_value;\n";
          } else if (forany) {
            be_global->impl_ <<
              "    return ser << " << cpp_name << "_forany(value_);\n";
          } else {
            be_global->impl_ <<
              "    return ser << value_;\n";
          }
        }
        be_global->impl_ <<
          "  }\n"
          "\n";

        be_global->impl_ <<
          "protected:\n";

        if (!generate_dynamic_data_adapter_access(node, wrapper, false)) {
          return false;
        }
        be_global->impl_ << "\n";
        if (!generate_dynamic_data_adapter_access(node, wrapper, true)) {
          return false;
        }

        be_global->impl_ <<
          "};\n"
          "\n";
      }

      std::string export_macro = be_global->export_macro().c_str();
      if (export_macro.size()) {
        export_macro += " ";
      }

      generate_get_dynamic_data_adapter(cpp_name, tag, export_macro, true, generate);
      generate_get_dynamic_data_adapter(cpp_name, tag, export_macro, false, generate);

      be_global->header_ <<
        "template <>\n" <<
        export_macro << "const " << cpp_name << "* get_dynamic_data_adapter_value<" <<
        cpp_name << tag << ">(DDS::DynamicData_ptr dd);\n"
        "\n";

      be_global->impl_ <<
        "template <>\n"
        "const " << cpp_name << "* get_dynamic_data_adapter_value<" <<
        cpp_name << tag << ">(DDS::DynamicData_ptr dd)\n"
        "{\n"
        "  ACE_UNUSED_ARG(dd);\n";
      if (generate) {
        be_global->impl_ <<
          "#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER\n"
          "  typedef DynamicDataAdapterImpl<" << cpp_name << tag << "> Dda;\n"
          "  const Dda* const dda = dynamic_cast<Dda*>(dd);\n"
          "  if (dda) {\n"
          "    return &dda->wrapped();\n"
          "  }\n"
          "#  endif\n";
      }
      be_global->impl_ <<
        "  return 0;\n"
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

bool dynamic_data_adapter_generator::gen_typedef(AST_Typedef* typedef_node, UTL_ScopedName* name,
  AST_Type* type, const char*)
{
  const AST_Decl::NodeType nt = type->node_type();
  if (nt != AST_Decl::NT_sequence && nt != AST_Decl::NT_array) {
    return true;
  }
  const std::string cpp_name = scoped(name);
  return generate_dynamic_data_adapter(type, &cpp_name, typedef_node);
}

bool dynamic_data_adapter_generator::gen_union(AST_Union* node, UTL_ScopedName*,
  const std::vector<AST_UnionBranch*>&, AST_Type*, const char*)
{
  return generate_dynamic_data_adapter(node);
}
