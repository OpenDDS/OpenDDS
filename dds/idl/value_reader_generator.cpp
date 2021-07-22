/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "value_reader_generator.h"

#include "be_extern.h"
#include "be_util.h"

#include <dds/DCPS/Definitions.h>

#include <global_extern.h>
#include <utl_identifier.h>
#include <utl_labellist.h>
#include <ast_fixed.h>

using namespace AstTypeClassification;

namespace {

  void generate_read(const std::string& expression, const std::string& accessor,
                     AST_Type* type, const std::string& idx, int level = 1);

  void array_helper(const std::string& expression, AST_Array* array,
                    size_t dim_idx, const std::string& idx, int level)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const std::string indent(level * 2, ' ');
    if (dim_idx < array->n_dims()) {
      const size_t dim = array->dims()[dim_idx]->ev()->u.ulval;
      be_global->impl_ <<
        indent << "if (!value_reader.begin_array()) return false;\n" <<
        indent << "for (" << (use_cxx11 ? "size_t " : "unsigned int ") << idx << " = 0; "
          << idx << " != " << dim << "; ++" << idx << ") {\n" <<
        indent << "  if (!value_reader.begin_element()) return false;\n";
      array_helper(expression + "[" + idx + "]", array, dim_idx + 1, idx + "i", level + 1);
      be_global->impl_ <<
        indent << "  if (!value_reader.end_element()) return false;\n" <<
        indent << "}\n" <<
        indent << "if (!value_reader.end_array()) return false;\n";
    } else {
      generate_read(expression, "", array->base_type(), idx + "i", level);
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
    // TODO: Take advantage of the size.
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const std::string indent(level * 2, ' ');
    be_global->impl_ <<
      indent << "if (!value_reader.begin_sequence()) return false;\n" <<
      indent << "for (" << (use_cxx11 ? "size_t " : "unsigned int ") << idx << " = 0; "
        "value_reader.elements_remaining(); ++" << idx << ") {\n";
    if (use_cxx11) {
      be_global->impl_ << indent << "  " << expression << ".resize(" << expression << ".size() + 1);\n";
    } else {
      be_global->impl_ << indent << "  " << expression << ".length(" << expression << ".length() + 1);\n";
    }
    be_global->impl_ <<
      indent << "  if (!value_reader.begin_element()) return false;\n";
    generate_read(expression + "[" + idx + "]", "", sequence->base_type(), idx + "i", level + 1);
    be_global->impl_ <<
      indent << "  if (!value_reader.end_element()) return false;\n" <<
      indent << "}\n" <<
      indent << "if (!value_reader.end_sequence()) return false;\n";
  }

  void generate_read(const std::string& expression, const std::string& accessor,
                     AST_Type* type, const std::string& idx, int level)
  {
    AST_Type* const actual = resolveActualType(type);

    const Classification c = classify(actual);
    if (c & CL_SEQUENCE) {
      AST_Sequence* const sequence = dynamic_cast<AST_Sequence*>(actual);
      sequence_helper(expression + accessor, sequence, idx, level);
      return;

    } else if (c & CL_ARRAY) {
      AST_Array* const array = dynamic_cast<AST_Array*>(actual);
      array_helper(expression + accessor, array, 0, idx, level);
      return;
    }

    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const std::string indent(level * 2, ' ');
    if (c & CL_FIXED) {
      be_global->impl_ <<
        indent << "if (!value_reader.read_fixed(" << expression << ")) return false;\n";

    } else if (c & CL_STRING) {
      be_global->impl_ <<
        indent << "{\n" <<
        indent << "  " << ((c & CL_WIDE) ? "WString" : "String") << " x;\n" <<
        indent << "  if (!value_reader.read_" << ((c & CL_WIDE) ? "w" : "")
          << "string(x)) return false;\n" <<
        indent << "  " << expression << accessor << " = x" << (use_cxx11 ? "" : ".c_str()")
          <<  ";\n" <<
        indent << "}\n";

    } else if (c & CL_PRIMITIVE) {
      const AST_PredefinedType::PredefinedType pt =
        dynamic_cast<AST_PredefinedType*>(actual)->pt();
      if (pt == AST_PredefinedType::PT_boolean) {
        be_global->impl_ <<
          indent << "{\n" <<
          indent << "  " << scoped(type->name()) << " bx;\n" <<
          indent << "  if (!value_reader.read_" << primitive_type(pt)
            << "(bx)) return false;\n" <<
          indent << "  " << expression << accessor << " = bx;\n" <<
          indent << "}\n";
      } else {
        be_global->impl_ <<
          indent << "if (!value_reader.read_" << primitive_type(pt) << '(' << expression << accessor << ")) return false;\n";
      }
    } else {
      be_global->impl_ <<
        indent << "if (!vread(value_reader, " << expression << accessor <<
          ")) return false;\n";
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
    AST_Type* const actual = resolveActualType(type);
    std::string decl = scoped(type->name());

    const Classification c = classify(actual);
    if (c & CL_STRING) {
      decl = (c & CL_WIDE) ? "std::wstring" : "std::string";
    }

    be_global->impl_ <<
      "    if (!value_reader.begin_union_member()) return false;\n"
      "    " << decl << " bv;\n";
    generate_read("bv", "", type, "i", 2);
    be_global->impl_ <<
      "    value." << field_name << "(bv" << ((c & CL_STRING) ? ".c_str()" : "") << ");\n" <<
      "    if (!value_reader.end_union_member()) return false;\n";
    return "";
  }
}

bool value_reader_generator::gen_enum(AST_Enum*,
                                      UTL_ScopedName* name,
                                      const std::vector<AST_EnumVal*>& contents,
                                      const char*)
{
  be_global->add_include("dds/DCPS/ValueReader.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);

  {
    NamespaceGuard guard;

    Function read("vread", "bool");
    read.addArg("value_reader", "OpenDDS::DCPS::ValueReader&");
    read.addArg("value", type_name + "&");
    read.endArgs();

    be_global->impl_ <<
      "  static const ListEnumHelper::Pair pairs[] = {";

    for (size_t i = 0; i != contents.size(); ++i) {
      if (i) {
        be_global->impl_ << ',';
      }
      be_global->impl_ <<
        '{' << '"' << contents[i]->local_name()->get_string() << '"' << ',' << contents[i]->constant_value()->ev()->u.eval << '}';
    }

    be_global->impl_ <<
      ",{0,0}};\n"
      "  ListEnumHelper helper(pairs);\n"
      "  return value_reader.read_enum(value, helper);\n";
  }

  return true;
}

bool value_reader_generator::gen_typedef(AST_Typedef*,
                                         UTL_ScopedName*,
                                         AST_Type*,
                                         const char*)
{
  return true;
}

bool value_reader_generator::gen_struct(AST_Structure*,
                                        UTL_ScopedName* name,
                                        const std::vector<AST_Field*>& fields,
                                        AST_Type::SIZE_TYPE,
                                        const char*)
{
  be_global->add_include("dds/DCPS/ValueReader.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  const std::string accessor = use_cxx11 ? "()" : "";

  {
    NamespaceGuard guard;

    Function read("vread", "bool");
    read.addArg("value_reader", "OpenDDS::DCPS::ValueReader&");
    read.addArg("value", type_name + "&");
    read.endArgs();

    be_global->impl_ <<
      "  static const ListMemberHelper::Pair pairs[] = {";

    for (size_t i = 0; i != fields.size(); ++i) {
      if (i) {
        be_global->impl_ << ',';
      }
      be_global->impl_ <<
        '{' << '"' << fields[i]->local_name()->get_string() << '"' << ',' << be_global->get_id(fields[i]) << '}';
    }

    be_global->impl_ <<
      ",{0,0}};\n"
      "  ListMemberHelper helper(pairs);\n";

    be_global->impl_ <<
      "  if (!value_reader.begin_struct()) return false;\n"
      "  XTypes::MemberId member_id;\n"
      "  while (value_reader.begin_struct_member(member_id, helper)) {\n"
      "    switch (member_id) {\n";

    for (std::vector<AST_Field*>::const_iterator pos = fields.begin(), limit = fields.end();
         pos != limit; ++pos) {
      AST_Field* const field = *pos;
      const std::string field_name = field->local_name()->get_string();
      be_global->impl_ <<
        "    case " << be_global->get_id(field) << ": {\n";
      generate_read("value." + field_name, accessor, field->field_type(), "i", 3);
      be_global->impl_ <<
        "      break;\n"
        "    }\n";
    }

    be_global->impl_ <<
      "    }\n"
      "    if (!value_reader.end_struct_member()) return false;\n"
      "  }\n"
      "  if (!value_reader.end_struct()) return false;\n"
      "  return true;\n";
  }

  return true;
}


bool value_reader_generator::gen_union(AST_Union* u,
                                       UTL_ScopedName* name,
                                       const std::vector<AST_UnionBranch*>& branches,
                                       AST_Type* discriminator,
                                       const char*)
{
  be_global->add_include("dds/DCPS/ValueReader.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);

  {
    NamespaceGuard guard;

    Function read("vread", "bool");
    read.addArg("value_reader", "OpenDDS::DCPS::ValueReader&");
    read.addArg("value", type_name + "&");
    read.endArgs();

    be_global->impl_ <<
      "  if (!value_reader.begin_union()) return false;\n"
      "  if (!value_reader.begin_discriminator()) return false;\n"
      "  {\n"
      "    " << scoped(discriminator->name()) << " d;\n";
    generate_read("d", "", discriminator, "i", 2);
    be_global->impl_ <<
      "    value._d(d);\n"
      "  }\n"
      "  if (!value_reader.end_discriminator()) return false;\n";

    generateSwitchForUnion(u, "value._d()", branch_helper, branches,
                           discriminator, "", "", type_name.c_str(),
                           false, false);
    be_global->impl_ <<
      "  if (!value_reader.end_union()) return false;\n"
      "  return true;\n";
  }

  return true;
}
