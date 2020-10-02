/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "value_writer_generator.h"
#include "be_extern.h"
#include "global_extern.h"

#include "utl_identifier.h"
#include "utl_labellist.h"

#include <ast_fixed.h>

using namespace AstTypeClassification;

namespace {

  void generate_write(const std::string& expression, AST_Type* type, const std::string& idx);

  void array_helper(const std::string& expression, AST_Array* array, size_t dim_idx, const std::string& idx)
  {
    if (dim_idx < array->n_dims()) {
      const size_t dim = array->dims()[dim_idx]->ev()->u.ulval;
      be_global->impl_ << "value_writer.begin_array();\n";
      be_global->impl_ << "for (size_t " << idx << " = 0; " << idx << " != " << dim << "; ++" << idx << ") {\n";
      be_global->impl_ << "  value_writer.begin_element(" << idx << ");\n";
      array_helper(expression + "[" + idx + "]", array, dim_idx + 1, idx + "i");
      be_global->impl_ << "  value_writer.end_element();\n";
      be_global->impl_ << "}\n";
      be_global->impl_ << "value_writer.end_array();\n";
    } else {
      generate_write(expression, array->base_type(), idx + "i");
    }
  }

  void generate_write(const std::string& expression, AST_Type* type, const std::string& idx)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const char* length_func = use_cxx11 ? "size" : "length";
    AST_Type* actual = resolveActualType(type);

    Classification c = classify(actual);
    if (c & CL_SEQUENCE) {
      AST_Sequence* sequence = dynamic_cast<AST_Sequence*>(actual);
      be_global->impl_ << "value_writer.begin_sequence();\n";
      be_global->impl_ << "for (size_t " << idx << " = 0; " << idx << " != " << expression << "." << length_func << "(); ++" << idx << ") {\n";
      be_global->impl_ << "  value_writer.begin_element(" << idx << ");\n";
      generate_write(expression + "[" + idx + "]", sequence->base_type(), idx + "i");
      be_global->impl_ << "  value_writer.end_element();\n";
      be_global->impl_ << "}\n";
      be_global->impl_ << "value_writer.end_sequence();\n";
    } else if (c & CL_ARRAY) {
      AST_Array* array = dynamic_cast<AST_Array*>(actual);
      array_helper(expression, array, 0, idx);
    } else if (c & CL_FIXED) {
      be_global->impl_ << "value_writer.write_fixed(" << expression << ");\n";
    } else if (c & CL_STRING) {
      if (c & CL_WIDE) {
        be_global->impl_ << "value_writer.write_wstring(" << expression << ");\n";
      }
      else {
        be_global->impl_ << "value_writer.write_string(" << expression << ");\n";
      }
    } else if (c & CL_PRIMITIVE) {
      switch (dynamic_cast<AST_PredefinedType*>(actual)->pt()) {
      case AST_PredefinedType::PT_long:
        be_global->impl_ << "value_writer.write_int32(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_ulong:
        be_global->impl_ << "value_writer.write_uint32(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_longlong:
        be_global->impl_ << "value_writer.write_int64(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_ulonglong:
        be_global->impl_ << "value_writer.write_uint64(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_short:
        be_global->impl_ << "value_writer.write_int16(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_ushort:
        be_global->impl_ << "value_writer.write_uint16(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_float:
        be_global->impl_ << "value_writer.write_float32(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_double:
        be_global->impl_ << "value_writer.write_float64(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_longdouble:
        be_global->impl_ << "value_writer.write_float128(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_char:
        be_global->impl_ << "value_writer.write_char8(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_wchar:
        be_global->impl_ << "value_writer.write_char16(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_boolean:
        be_global->impl_ << "value_writer.write_boolean(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_octet:
        be_global->impl_ << "value_writer.write_byte(" << expression << ");\n";
        break;
      case AST_PredefinedType::PT_any:
      case AST_PredefinedType::PT_object:
      case AST_PredefinedType::PT_value:
      case AST_PredefinedType::PT_abstract:
      case AST_PredefinedType::PT_void:
      case AST_PredefinedType::PT_pseudo:
        // TODO
        break;
      }
    } else {
      be_global->impl_ << "vwrite(value_writer, " << expression << ");\n";
    }
  }

  std::string branch_helper(const std::string& field_name,
                            AST_Type* type,
                            const std::string&,
                            std::string&,
                            const std::string&)
  {
    be_global->impl_ <<
      "  {\n"
      "    value_writer.begin_field(\"" << field_name << "\");\n";
    generate_write(std::string("value.") + field_name + "()", type, "i");
    be_global->impl_ <<
      "    value_writer.end_field();\n"
      "  }\n";

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

    Function write("vwrite", "template <> void");
    write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
    write.addArg("value", "const " + type_name + "&");
    write.endArgs();

    be_global->impl_ << "switch(value) {\n";
    for (std::vector<AST_EnumVal*>::const_iterator pos = contents.begin(), limit = contents.end(); pos != limit; ++pos) {
      AST_EnumVal* val = *pos;
      const std::string value_name = (use_cxx11 ? (type_name + "::") : module_scope(name)) + val->local_name()->get_string();
      be_global->impl_ << "case " << value_name << ":\n";
      be_global->impl_ << "  value_writer.write_enum(\"" << val->local_name()->get_string() << "\", " << value_name << ");\n";
      be_global->impl_ << "  break;\n";
    }
    be_global->impl_ << "}\n";
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

    Function write("vwrite", "template <> void");
    write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
    write.addArg("value", "const " + type_name + "&");
    write.endArgs();

    be_global->impl_ << "  value_writer.begin_struct();\n";
    for (std::vector<AST_Field*>::const_iterator pos = fields.begin(), limit = fields.end(); pos != limit; ++pos) {
      AST_Field* field = *pos;
      const std::string field_name = field->local_name()->get_string();
      be_global->impl_ << "  value_writer.begin_field(\"" << field_name << "\");\n";
      generate_write(std::string("value.") + field_name + accessor_suffix, field->field_type(), "i");
      be_global->impl_ << "  value_writer.end_field();\n";
    }
    be_global->impl_ << "  value_writer.end_struct();\n";
  }

  return true;
}


bool value_writer_generator::gen_union(AST_Union*,
                                       UTL_ScopedName* name,
                                       const std::vector<AST_UnionBranch*>& branches,
                                       AST_Type* discriminator,
                                       const char*)
{
  be_global->add_include("dds/DCPS/ValueWriter.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);

  {
    NamespaceGuard guard;

    Function write("vwrite", "template <> void");
    write.addArg("value_writer", "OpenDDS::DCPS::ValueWriter&");
    write.addArg("value", "const " + type_name + "&");
    write.endArgs();

    be_global->impl_ << "  value_writer.begin_union();\n";
    be_global->impl_ << "  value_writer.begin_discriminator();\n";
    generate_write("value._d()" , discriminator, "i");
    be_global->impl_ << "  value_writer.end_discriminator();\n";

    generateSwitchForUnion("value._d()", branch_helper, branches, discriminator, "", "", type_name.c_str(), false, false);

    be_global->impl_ << "  value_writer.end_union();\n";
  }

  return true;
}
