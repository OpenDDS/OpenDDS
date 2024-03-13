/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "value_writer_generator.h"

#include "be_extern.h"
#include "global_extern.h"

#include <dds/DCPS/Definitions.h>

#include <utl_identifier.h>
#include <utl_labellist.h>
#include <ast_fixed.h>

using namespace AstTypeClassification;

namespace {

  void generate_write(const std::string& expression, AST_Type* type, const std::string& idx, int level = 1);

  std::string primitive_type(AST_PredefinedType::PredefinedType pt)
  {
    switch (pt) {
    case AST_PredefinedType::PT_long:
      return "int32";
    case AST_PredefinedType::PT_ulong:
      return "uint32";
    case AST_PredefinedType::PT_longlong:
      return "int64";
    case AST_PredefinedType::PT_ulonglong:
      return "uint64";
    case AST_PredefinedType::PT_short:
      return "int16";
    case AST_PredefinedType::PT_ushort:
      return "uint16";
#if OPENDDS_HAS_EXPLICIT_INTS
    case AST_PredefinedType::PT_int8:
      return "int8";
    case AST_PredefinedType::PT_uint8:
      return "uint8";
#endif
    case AST_PredefinedType::PT_float:
      return "float32";
    case AST_PredefinedType::PT_double:
      return "float64";
    case AST_PredefinedType::PT_longdouble:
      return "float128";
    case AST_PredefinedType::PT_char:
      return "char8";
    case AST_PredefinedType::PT_wchar:
      return "char16";
    case AST_PredefinedType::PT_boolean:
      return "boolean";
    case AST_PredefinedType::PT_octet:
      return "byte";
    default:
      return "";
    }
  }

  void array_helper(const std::string& expression, AST_Array* array,
                    size_t dim_idx, const std::string& idx, int level)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const std::string indent(level * 2, ' ');
    const Classification c = classify(array->base_type());
    const bool primitive = c & CL_PRIMITIVE;
    // When we have a primitive type the last dimension is written using the write_*_array
    // operation, when we have a not primitive type the last dimension is written element by element
    // in a loop in the generated code
    const std::string elem_kind = type_kind(array->base_type());
    if ((primitive && (dim_idx < array->n_dims() - 1)) || (!primitive && (dim_idx < array->n_dims()))) {
      const size_t dim = array->dims()[dim_idx]->ev()->u.ulval;
      be_global->impl_ <<
        indent << "if (!value_writer.begin_array(" << elem_kind << ")) {\n" <<
        indent << "  return false;\n" <<
        indent << "}\n";
      be_global->impl_ <<
        indent << "for (" << (use_cxx11 ? "size_t " : "::CORBA::ULong ") << idx << " = 0; "
        << idx << " != " << dim << "; ++" << idx << ") {\n" <<
        indent << "  if (!value_writer.begin_element(static_cast<ACE_CDR::ULong>(" << idx << "))) {\n" <<
        indent << "    return false;\n" <<
        indent << "  }\n";
      array_helper(expression + "[" + idx + "]", array, dim_idx + 1, idx + "i", level + 1);
      be_global->impl_ <<
        indent << "  if (!value_writer.end_element()) {\n" <<
        indent << "    return false;\n" <<
        indent << "  }\n" <<
        indent << "}\n" <<
        indent << "if (!value_writer.end_array()) {\n" <<
        indent << "  return false;\n" <<
        indent << "}\n";
    } else {
      if (primitive) {
        const size_t dim = array->dims()[dim_idx]->ev()->u.ulval;
        AST_Type* const actual = resolveActualType(array->base_type());
        const AST_PredefinedType::PredefinedType pt =
          dynamic_cast<AST_PredefinedType*>(actual)->pt();

        be_global->impl_ <<
          indent << "if (!value_writer.begin_array(" << elem_kind << ")) {\n" <<
          indent << "  return false;\n" <<
          indent << "}\n";
        be_global->impl_ <<
          indent <<
          "if (!value_writer.write_" << primitive_type(pt) << "_array (" << expression << (use_cxx11 ? ".data()" : "") << ", " << dim << ")) {\n" <<
          indent << "  return false;\n" <<
          indent << "}\n";
        be_global->impl_ <<
          indent << "if (!value_writer.end_array()) {\n" <<
          indent << "  return false;\n" <<
          indent << "}\n";

      } else {
        generate_write(expression, array->base_type(), idx + "i", level);
      }
    }
  }

  void sequence_helper(const std::string& expression, AST_Sequence* sequence,
                       const std::string& idx, int level)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const char* const length_func = use_cxx11 ? "size" : "length";
    const std::string indent(level * 2, ' ');
    AST_Type* const base_type = sequence->base_type();
    const std::string elem_tk = type_kind(base_type);
    be_global->impl_ <<
      indent << "if (!value_writer.begin_sequence(" << elem_tk <<
      ", static_cast<ACE_CDR::ULong>(" << expression << "." << length_func << "()))) {\n" <<
      indent << "  return false;\n" <<
      indent << "}\n";

    const Classification c = classify(base_type);
    AST_Type* const actual = resolveActualType(base_type);
    bool use_optimized_write_ = false;
    if (c & CL_PRIMITIVE) {
      if (use_cxx11) {
        const AST_PredefinedType::PredefinedType pt = dynamic_cast<AST_PredefinedType*>(actual)->pt();
        use_optimized_write_ = !(pt == AST_PredefinedType::PT_boolean);
      } else {
        use_optimized_write_ = true;
      }
    }

    if (use_optimized_write_) {
      const AST_PredefinedType::PredefinedType pt =
        dynamic_cast<AST_PredefinedType*>(actual)->pt();
      be_global->impl_ << indent <<
        "if (!value_writer.write_" << primitive_type(pt) << "_array (" <<
        expression << (use_cxx11 ? ".data()" : ".get_buffer()") <<
        ", static_cast<ACE_CDR::ULong>(" << expression << "." << length_func << "()))) {\n" <<
        indent << "  return false;\n" <<
        indent << "}\n";
    } else {
      be_global->impl_ <<
        indent << "for (" << (use_cxx11 ? "size_t " : "::CORBA::ULong ") << idx << " = 0; "
        << idx << " != " << expression << "." << length_func << "(); ++" << idx << ") {\n" <<
        indent << "  if (!value_writer.begin_element(static_cast<ACE_CDR::ULong>(" << idx << "))) {\n" <<
        indent << "    return false;\n" <<
        indent << "  }\n";
      generate_write(expression + "[" + idx + "]", base_type, idx + "i", level + 1);
      be_global->impl_ <<
        indent << "  if (!value_writer.end_element()) {\n" <<
        indent << "    return false;\n" <<
        indent << "  }\n" <<
        indent << "}\n";
    }

    be_global->impl_ <<
      indent << "if (!value_writer.end_sequence()) {\n" <<
      indent << "  return false;\n" <<
      indent << "}\n";
  }

  void generate_write(const std::string& expression, AST_Type* type, const std::string& idx, int level)
  {
    AST_Type* const actual = resolveActualType(type);

    const Classification c = classify(actual);
    if (c & CL_SEQUENCE) {
      AST_Sequence* const sequence = dynamic_cast<AST_Sequence*>(actual);
      sequence_helper(expression, sequence, idx, level);
      return;

    } else if (c & CL_ARRAY) {
      AST_Array* const array = dynamic_cast<AST_Array*>(actual);
      array_helper(expression, array, 0, idx, level);
      return;
    }

    be_global->impl_ << std::string(level * 2, ' ');

    if (c & CL_FIXED) {
      be_global->impl_ <<
        "if (!value_writer.write_fixed(" << expression << ".to_ace_fixed())) {\n"
        "  return false;\n"
        "}\n";

    } else if (c & CL_STRING) {
      be_global->impl_ <<
        "if (!value_writer.write_" << ((c & CL_WIDE) ? "w" : "") << "string(" << expression << ")) {\n"
        "  return false;\n"
        "}\n";

    } else if (c & CL_PRIMITIVE) {
      const AST_PredefinedType::PredefinedType pt =
        dynamic_cast<AST_PredefinedType*>(actual)->pt();
      be_global->impl_ <<
        "if (!value_writer.write_" << primitive_type(pt) << '(' << expression << ")) {\n"
        "  return false;\n"
        "}\n";

    } else {
      be_global->impl_ <<
        "if (!vwrite(value_writer, " << expression << ")) {\n"
        "  return false;\n"
        "}\n";
    }
  }

  std::string branch_helper(const std::string&,
                            AST_Decl* branch,
                            const std::string& field_name,
                            AST_Type* type,
                            const std::string&,
                            bool,
                            Intro&,
                            const std::string&)
  {
    // TODO: Update the arguments when @optional is available.
    const OpenDDS::XTypes::MemberId id = be_global->get_id(dynamic_cast<AST_UnionBranch*>(branch));
    const bool must_understand = be_global->is_effectively_must_understand(branch);
    be_global->impl_ <<
      "    if (!value_writer.begin_union_member(MemberParam(" << id << ", " <<
      (must_understand ? "true" : "false") << ", \"" << canonical_name(branch) << "\", false, true))) {\n"
      "      return false;\n"
      "    }\n";
    generate_write("value." + field_name + "()", type, "i", 2);
    be_global->impl_ <<
      "    if (!value_writer.end_union_member()) {\n"
      "      return false;\n"
      "    }\n";
    return "";
  }
}

bool value_writer_generator::gen_enum(AST_Enum*,
                                      UTL_ScopedName* name,
                                      const std::vector<AST_EnumVal*>&,
                                      const char*)
{
  be_global->add_include("dds/DCPS/ValueWriter.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);

  NamespaceGuard guard;
  Function write("vwrite", "bool");
  write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
  write.addArg("value", "const " + type_name + "&");
  write.endArgs();
  be_global->impl_ <<
    "  return value_writer.write_enum(value, * ::OpenDDS::DCPS::gen_"
    << scoped_helper(name, "_") << "_helper);\n";
  return true;
}

bool value_writer_generator::gen_typedef(AST_Typedef*,
                                         UTL_ScopedName*,
                                         AST_Type*,
                                         const char*)
{
  return true;
}

bool value_writer_generator::gen_struct(AST_Structure* node,
                                        UTL_ScopedName* name,
                                        const std::vector<AST_Field*>& fields,
                                        AST_Type::SIZE_TYPE,
                                        const char*)
{
  be_global->add_include("dds/DCPS/ValueWriter.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  const std::string accessor_suffix = use_cxx11 ? "()" : "";

  {
    NamespaceGuard guard;

    Function write("vwrite", "bool");
    write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
    write.addArg("value", "const " + type_name + "&");
    write.endArgs();

    const ExtensibilityKind ek = be_global->extensibility(node);
    be_global->impl_ <<
      "  if (!value_writer.begin_struct(" << extensibility_kind(ek) << ")) {\n"
      "    return false;\n"
      "  }\n";
    for (std::vector<AST_Field*>::const_iterator pos = fields.begin(), limit = fields.end();
         pos != limit; ++pos) {
      AST_Field* const field = *pos;
      const std::string field_name = field->local_name()->get_string();
      const std::string idl_name = canonical_name(field);
      // TODO: Update the arguments when @optional is available.
      const OpenDDS::XTypes::MemberId id = be_global->get_id(field);
      const bool must_understand = be_global->is_effectively_must_understand(field);
      be_global->impl_ <<
        "  if (!value_writer.begin_struct_member(MemberParam(" << id << ", " <<
        (must_understand ? "true" : "false") << ", \"" << idl_name << "\", false, true))) {\n"
        "    return false;\n"
        "  }\n";
      generate_write("value." + field_name + accessor_suffix, field->field_type(), "i");
      be_global->impl_ <<
        "  if (!value_writer.end_struct_member()) {\n"
        "    return false;\n"
        "  }\n";
    }
    be_global->impl_ <<
      "  return value_writer.end_struct();\n";
  }

  return true;
}


bool value_writer_generator::gen_union(AST_Union* u,
                                       UTL_ScopedName* name,
                                       const std::vector<AST_UnionBranch*>& branches,
                                       AST_Type* discriminator,
                                       const char*)
{
  be_global->add_include("dds/DCPS/ValueWriter.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);

  {
    NamespaceGuard guard;

    Function write("vwrite", "bool");
    write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
    write.addArg("value", "const " + type_name + "&");
    write.endArgs();

    const ExtensibilityKind ek = be_global->extensibility(u);
    be_global->impl_ <<
      "  if (!value_writer.begin_union(" << extensibility_kind(ek) << ")) {\n"
      "    return false;\n"
      "  }\n";
    const bool must_understand = be_global->is_effectively_must_understand(discriminator);
    be_global->impl_ <<
      "  if (!value_writer.begin_discriminator(MemberParam(0, " <<
      (must_understand ? "true" : "false") << "))) {\n"
      "    return false;\n"
      "  }\n";
    generate_write("value._d()" , discriminator, "i");
    be_global->impl_ <<
      "  if (!value_writer.end_discriminator()) {\n"
      "    return false;\n"
      "  }\n";

    generateSwitchForUnion(u, "value._d()", branch_helper, branches,
                           discriminator, "", "", type_name.c_str(),
                           false, false);
    be_global->impl_ <<
      "  return value_writer.end_union();\n";
  }

  return true;
}
