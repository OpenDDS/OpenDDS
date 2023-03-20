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

  void generate_op(unsigned indent, bool set, AST_Decl* node, AST_Type* type,
    const std::string& type_name, const std::string& value, const char* rc_dest = "return")
  {
    const bool cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    std::string extra;
    bool is_complex = false;
    const char* op_type = 0;
    switch (resolveActualType(type)->node_type()) {
    case AST_Decl::NT_pre_defined:
      op_type = "simple";
      break;
    case AST_Decl::NT_enum:
      op_type = "enum";
      break;
    case AST_Decl::NT_string:
      if (cxx11) {
        op_type = "cpp11_s8";
      }
      if (set) {
        extra = ".inout()";
      }
      op_type = "s8";
      break;
    case AST_Decl::NT_wstring:
      if (cxx11) {
        op_type = "cpp11_s16";
      }
      if (set) {
        extra = ".inout()";
      }
      op_type = "s16";
      break;
    case AST_Decl::NT_struct:
    case AST_Decl::NT_sequence:
    case AST_Decl::NT_union:
      is_complex = true;
      op_type = set ? "direct_complex" : "complex";
      break;
    case AST_Decl::NT_array:
      is_complex = true;
      if (set) {
        op_type = cxx11 ? "direct_complex" : "indirect_complex";
      } else {
        op_type = "complex";
      }
      break;
    default:
      be_util::misc_error_and_abort(
        "Unsupported type in dynamic_data_adapter_op_type", node);
    }

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
      std::string(indent * 2, ' ') << "  " << rc_dest << " " <<
      op(set) << "_" << op_type << "_raw_value" << template_params << "(method, ";
    if (set) {
      be_global->impl_ << value << extra << ", id, source, tk";
    } else {
      be_global->impl_ << "dest, tk, " << value << extra << ", id";
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
    be_global->impl_ << ": // " << (disc ? "discriminator" : canonical_name(field)) << "\n"
      "      {\n";

    if (field && !be_global->dynamic_data_adapter(field->field_type())) {
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
      generate_op(3, set, field, field_type, type_name, value, rc_dest);
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

      AST_Type* const base_type = seq_node ? seq_node->base_type() : array_node->base_type();
      // For the type name we need the deepest named type, not the actual type.
      // This will be the name of the first typedef if it's an array or
      // sequence, otherwise the name of the type.
      AST_Type* consider = base_type;
      AST_Type* named_type = base_type;
      while (consider->node_type() == AST_Decl::NT_typedef) {
        named_type = consider;
        consider = dynamic_cast<AST_Typedef*>(named_type)->base_type();
      }
      generate_op(2, set, node, base_type, scoped(named_type->name()), wrapper.flat_collection_access("id"));
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
      break;
    case AST_Decl::NT_array:
      array_node = dynamic_cast<AST_Array*>(node);
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

    if (generate) {
      DynamicDataAdapterGuard ddag;

      be_global->impl_ <<
        "template <>\n"
        "class DynamicDataAdapterImpl<" << cpp_name << tag << " > : public DynamicDataAdapter_T<"
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
        be_global->impl_ << wrapper.seq_get_length();
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

    {
      NoSafetyProfileGuard nspg;

      wrapper.generate_tag();

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
