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

  void array_helper(const std::string& expression, AST_Array* array,
                    size_t dim_idx, const std::string& idx, int level)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const std::string indent(level * 2, ' ');
    if (dim_idx < array->n_dims()) {
      const size_t dim = array->dims()[dim_idx]->ev()->u.ulval;
      be_global->impl_ <<
        indent << "value_writer.begin_array();\n" <<
        indent << "for (" << (use_cxx11 ? "size_t " : "unsigned int ") << idx << " = 0; "
        << idx << " != " << dim << "; ++" << idx << ") {\n" <<
        indent << "  value_writer.begin_element(" << idx << ");\n";
      array_helper(expression + "[" + idx + "]", array, dim_idx + 1, idx + "i", level + 1);
      be_global->impl_ <<
        indent << "  value_writer.end_element();\n" <<
        indent << "}\n" <<
        indent << "value_writer.end_array();\n";
    } else {
      generate_write(expression, array->base_type(), idx + "i", level);
    }
  }

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

  void sequence_helper(const std::string& expression, AST_Sequence* sequence,
                       const std::string& idx, int level)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const char* const length_func = use_cxx11 ? "size" : "length";
    const std::string indent(level * 2, ' ');
    be_global->impl_ <<
      indent << "value_writer.begin_sequence();\n" <<
      indent << "for (" << (use_cxx11 ? "size_t " : "unsigned int ") << idx << " = 0; "
      << idx << " != " << expression << "." << length_func << "(); ++" << idx << ") {\n" <<
      indent << "  value_writer.begin_element(" << idx << ");\n";
    generate_write(expression + "[" + idx + "]", sequence->base_type(), idx + "i", level + 1);
    be_global->impl_ <<
      indent << "  value_writer.end_element();\n" <<
      indent << "}\n" <<
      indent << "value_writer.end_sequence();\n";
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
        "value_writer.write_fixed(" << expression << ");\n";

    } else if (c & CL_STRING) {
      be_global->impl_ <<
        "value_writer.write_" << ((c & CL_WIDE) ? "w" : "") << "string(" << expression << ");\n";

    } else if (c & CL_PRIMITIVE) {
      const AST_PredefinedType::PredefinedType pt =
        dynamic_cast<AST_PredefinedType*>(actual)->pt();
      be_global->impl_ <<
        "value_writer.write_" << primitive_type(pt) << '(' << expression << ");\n";

    } else {
      be_global->impl_ <<
        "vwrite(value_writer, " << expression << ");\n";
    }
  }

  std::string branch_helper(const std::string&,
                            const std::string& field_name,
                            AST_Type* type,
                            const std::string&,
                            bool,
                            Intro&,
                            const std::string&,
                            bool)
  {
    be_global->impl_ <<
      "    value_writer.begin_union_member(\"" << field_name << "\");\n";
    generate_write("value." + field_name + "()", type, "i", 2);
    be_global->impl_ <<
      "    value_writer.end_union_member();\n";
    return "";
  }
}

bool value_writer_generator::gen_enum(AST_Enum*,
                                      UTL_ScopedName* name,
                                      const std::vector<AST_EnumVal*>& contents,
                                      const char*)
{
  be_global->add_include("dds/DCPS/ValueWriter.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

  {
    NamespaceGuard guard;

    Function write("vwrite", "void");
    write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
    write.addArg("value", "const " + type_name + "&");
    write.endArgs();

    be_global->impl_ <<
      "  switch (value) {\n";
    for (std::vector<AST_EnumVal*>::const_iterator pos = contents.begin(), limit = contents.end();
         pos != limit; ++pos) {
      AST_EnumVal* const val = *pos;
      const std::string value_name = (use_cxx11 ? (type_name + "::") : module_scope(name))
        + val->local_name()->get_string();
      be_global->impl_ <<
        "  case " << value_name << ":\n"
        "    value_writer.write_enum(\"" << val->local_name()->get_string() << "\", " << value_name << ");\n"
        "    break;\n";
    }
    be_global->impl_ << "  }\n";
  }

  return true;
}

bool value_writer_generator::gen_typedef(AST_Typedef*,
                                         UTL_ScopedName*,
                                         AST_Type*,
                                         const char*)
{
  return true;
}

bool value_writer_generator::gen_struct(AST_Structure*,
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

    Function write("vwrite", "void");
    write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
    write.addArg("value", "const " + type_name + "&");
    write.endArgs();

    be_global->impl_ <<
      "  value_writer.begin_struct();\n";
    for (std::vector<AST_Field*>::const_iterator pos = fields.begin(), limit = fields.end();
         pos != limit; ++pos) {
      AST_Field* const field = *pos;
      const std::string field_name = field->local_name()->get_string();
      be_global->impl_ <<
        "  value_writer.begin_struct_member(\"" << field_name << "\");\n";
      generate_write("value." + field_name + accessor_suffix, field->field_type(), "i");
      be_global->impl_ <<
        "  value_writer.end_struct_member();\n";
    }
    be_global->impl_ <<
      "  value_writer.end_struct();\n";
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

    Function write("vwrite", "void");
    write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
    write.addArg("value", "const " + type_name + "&");
    write.endArgs();

    be_global->impl_ <<
      "  value_writer.begin_union();\n"
      "  value_writer.begin_discriminator();\n";
    generate_write("value._d()" , discriminator, "i");
    be_global->impl_ <<
      "  value_writer.end_discriminator();\n";

    generateSwitchForUnion(u, "value._d()", branch_helper, branches,
                           discriminator, "", "", type_name.c_str(),
                           false, false);
    be_global->impl_ <<
      "  value_writer.end_union();\n";
  }

  return true;
}
