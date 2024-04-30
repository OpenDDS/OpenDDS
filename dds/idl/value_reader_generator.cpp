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
                     const std::string& field_name, AST_Type* type, const std::string& idx,
                     int level = 1, FieldFilter field_filter = FieldFilter_All, bool optional = false);

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

  void array_helper(const std::string& expression, AST_Array* array, size_t dim_idx,
                    const std::string& idx, int level, FieldFilter filter_kind)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const std::string indent(level * 2, ' ');
    const Classification c = classify(array->base_type());
    const bool primitive = c & CL_PRIMITIVE;
    // When we have a primitive type the last dimension is read using the read_*_array
    // operation, when we have a not primitive type the last dimension is read element by element
    // in a loop in the generated code
    const std::string elem_kind = type_kind(array->base_type());
    if ((primitive && (dim_idx < array->n_dims() - 1)) || (!primitive && (dim_idx < array->n_dims()))) {
      const size_t dim = array->dims()[dim_idx]->ev()->u.ulval;
      be_global->impl_ <<
        indent << "if (!value_reader.begin_array(" << elem_kind << ")) return false;\n" <<
        indent << "for (" << (use_cxx11 ? "size_t " : "unsigned int ") << idx << " = 0; "
          << idx << " != " << dim << "; ++" << idx << ") {\n" <<
        indent << "  if (!value_reader.begin_element()) return false;\n";
      array_helper(expression + "[" + idx + "]", array, dim_idx + 1, idx + "i", level + 1, filter_kind);
      be_global->impl_ <<
        indent << "  if (!value_reader.end_element()) return false;\n" <<
        indent << "}\n" <<
        indent << "if (!value_reader.end_array()) return false;\n";
    } else {
      if (primitive) {
        const size_t dim = array->dims()[dim_idx]->ev()->u.ulval;
        AST_Type* const actual = resolveActualType(array->base_type());
        const AST_PredefinedType::PredefinedType pt =
          dynamic_cast<AST_PredefinedType*>(actual)->pt();
        be_global->impl_ <<
          indent << "if (!value_reader.begin_array(" << elem_kind << ")) return false;\n";
        be_global->impl_ << indent <<
          "if (!value_reader.read_" << primitive_type(pt) << "_array (" << expression << (use_cxx11 ? ".data()" : "") << ", " << dim << ")) return false;\n";
        be_global->impl_ <<
          indent << "if (!value_reader.end_array()) return false;\n";

      } else {
        generate_read(expression, "", "elem", array->base_type(), idx, level, nested(filter_kind));
      }
    }
  }

  void sequence_helper(const std::string& expression, AST_Sequence* sequence,
                       const std::string& idx, int level, FieldFilter filter_kind)
  {
    // TODO: Take advantage of the size.
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const std::string indent(level * 2, ' ');
    const std::string elem_tk = type_kind(sequence->base_type());
    be_global->impl_ <<
      indent << "if (!value_reader.begin_sequence(" << elem_tk << ")) return false;\n" <<
      indent << "for (" << (use_cxx11 ? "size_t " : "unsigned int ") << idx << " = 0; "
        "value_reader.elements_remaining(); ++" << idx << ") {\n";
    if (use_cxx11) {
      be_global->impl_ << indent << "  " << expression << ".resize(" << expression << ".size() + 1);\n";
    } else {
      if (!sequence->unbounded()) {
        be_global->impl_ << indent << "  if (" << idx << " >= " << expression << ".maximum()) return false;\n";
        be_global->impl_ << indent << "  const ACE_CDR::ULong len = " << expression << ".length();\n";
        be_global->impl_ << indent << "  " << expression << ".length(len + 1);\n";
      } else {
        be_global->impl_ << indent << " OpenDDS::DCPS::grow(" << expression << ");\n";
      }
    }
    be_global->impl_ <<
      indent << "  if (!value_reader.begin_element()) return false;\n";
    generate_read(expression + "[" + idx + "]", "", "elem", sequence->base_type(),
                  idx + "i", level + 1, nested(filter_kind));
    be_global->impl_ <<
      indent << "  if (!value_reader.end_element()) return false;\n" <<
      indent << "}\n" <<
      indent << "if (!value_reader.end_sequence()) return false;\n";
  }

  void generate_read(const std::string& expression, const std::string& accessor,
                     const std::string& field_name, AST_Type* type, const std::string& idx,
                     int level, FieldFilter filter_kind, bool optional)
  {
    AST_Type* const actual = resolveActualType(type);

    const Classification c = classify(actual);

    const std::string indent(level * 2 + (optional ? 2 : 0), ' ');
    if (optional) {
      be_global->impl_ <<
        indent.substr(0, indent.size() - 2) << "if (value_reader.member_has_value()) {\n";
    }

    // If the field is optional we need to create a temporary to read the value into before we can assign it to the optional
    const bool create_tmp  = optional && !(c & CL_STRING);
    const std::string var_name = create_tmp ? "tmp" : expression + accessor;
    if (create_tmp) {
      const std::string tmp_type = scoped(type->name());
      be_global->impl_ <<
        indent << tmp_type << " tmp;\n";
    }

    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    if (c & CL_SEQUENCE) {
      AST_Sequence* const sequence = dynamic_cast<AST_Sequence*>(actual);
      sequence_helper(var_name, sequence, idx, level, filter_kind);
    } else if (c & CL_ARRAY) {
      AST_Array* const array = dynamic_cast<AST_Array*>(actual);
      array_helper(var_name, array, 0, idx, level, filter_kind);
    } else if (c & CL_FIXED) {
      be_global->impl_ <<
        indent << "::ACE_CDR::Fixed fixed;\n" <<
        indent << "if (!value_reader.read_fixed(fixed)) return false;\n" <<
        indent << expression << " = ::OpenDDS::FaceTypes::Fixed(fixed);\n";

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
          indent << "if (!value_reader.read_" << primitive_type(pt) << '(' << var_name << ")) return false;\n";
      }
    } else {
      std::string value_expr = (create_tmp ? "tmp" : expression + accessor);
      if (!(c & CL_ENUM)) {
        const std::string type_name = scoped(type->name());
        switch (filter_kind) {
        case FieldFilter_NestedKeyOnly:
          value_expr = field_name + "_nested_key_only";
          be_global->impl_ <<
            indent << "const NestedKeyOnly<" << type_name << "> " <<
            value_expr << "(" << var_name << ");\n";
          break;
        case FieldFilter_KeyOnly:
          value_expr = field_name + "_key_only";
          be_global->impl_ <<
            indent << "const KeyOnly<" << type_name << "> " <<
            value_expr << "(" << var_name << ");\n";
          break;
        default:
          break;
        }
      }

      be_global->impl_ <<
        indent << "if (!vread(value_reader, " << value_expr <<
          ")) return false;\n";
    }

    if (create_tmp) {
      be_global->impl_ <<
        indent << expression << accessor << " = std::move(tmp);\n";
    }

    if (optional) {
      be_global->impl_ <<
        indent.substr(0, indent.size() - 2) << "}\n";
    }
  }

  std::string branch_helper(const std::string&, AST_Decl* branch,
                            const std::string& field_name,
                            AST_Type* type,
                            const std::string&,
                            bool,
                            Intro&,
                            const std::string&)
  {
    AST_Type* const actual = resolveActualType(type);
    std::string decl = field_type_name(dynamic_cast<AST_Field*>(branch), type);

    const Classification c = classify(actual);
    if (c & CL_STRING) {
      decl = (c & CL_WIDE) ? "std::wstring" : "std::string";
    }

    be_global->impl_ <<
      "    if (!value_reader.begin_union_member()) return false;\n"
      "    " << decl << " bv;\n";
    generate_read("bv", "", field_name, type, "i", 2);
    be_global->impl_ <<
      "    value." << field_name << "(bv" << ((c & CL_STRING) ? ".c_str()" : "") << ");\n" <<
      "    if (!value_reader.end_union_member()) return false;\n";
    return "";
  }

  bool gen_struct_i(AST_Structure* node, const std::string& type_name,
                    bool use_cxx11, ExtensibilityKind ek, FieldFilter field_filter)
  {
    const std::string wrapped_name = key_only_type_name(node, type_name, field_filter, false);
    Function read("vread", "bool");
    read.addArg("value_reader", "OpenDDS::DCPS::ValueReader&");
    read.addArg("value", wrapped_name);
    read.endArgs();

    be_global->impl_ <<
      "  static const ListMemberHelper::Pair pairs[] = {";

    const std::string value_prefix = field_filter == FieldFilter_All ? "value." : "value.value.";
    const Fields fields(node, field_filter);

    // KeyOnly wrapper has no field if there is no explicit key.
    bool has_field = false;
    for (Fields::Iterator it = fields.begin(); it != fields.end(); ++it) {
      AST_Field* const field = *it;
      be_global->impl_ <<
        "{\"" << canonical_name(field) << "\"," << be_global->get_id(field) << "},";
      has_field = true;
    }

    be_global->impl_ <<
      "{0,0}};\n"
      "  ListMemberHelper helper(pairs);\n";

    be_global->impl_ <<
      "  if (!value_reader.begin_struct(" << extensibility_kind(ek) << ")) return false;\n";

    if (has_field) {
      be_global->impl_ <<
        "  XTypes::MemberId member_id;\n"
        "  while (value_reader.members_remaining()) {\n"
        "    if (!value_reader.begin_struct_member(member_id, helper)) return false;\n"
        "    switch (member_id) {\n";

      for (Fields::Iterator it = fields.begin(); it != fields.end(); ++it) {
        AST_Field* const field = *it;
        const std::string field_name = field->local_name()->get_string();
        be_global->impl_ <<
          "    case " << be_global->get_id(field) << ": {\n";
        generate_read(value_prefix + field_name, use_cxx11 ? "()" : "", field_name,
                      field->field_type(), "i", 3, nested(field_filter), be_global->is_optional(field));
        be_global->impl_ <<
          "      break;\n"
          "    }\n";
      }

      be_global->impl_ <<
        "    }\n"
        "    if (!value_reader.end_struct_member()) return false;\n"
        "  }\n";
    }

    be_global->impl_ <<
      "  if (!value_reader.end_struct()) return false;\n"
      "  return true;\n";
    return true;
  }

  bool gen_union_i(AST_Union* u, const std::string& type_name,
                   const std::vector<AST_UnionBranch*>& branches,
                   AST_Type* discriminator, ExtensibilityKind ek, FieldFilter filter_kind)
  {
    const std::string wrapped_name = key_only_type_name(u, type_name, filter_kind, false);
    Function read("vread", "bool");
    read.addArg("value_reader", "OpenDDS::DCPS::ValueReader&");
    read.addArg("value", wrapped_name);
    read.endArgs();

    const std::string value_prefix = filter_kind == FieldFilter_All ? "value." : "value.value.";
    be_global->impl_ <<
      "  if (!value_reader.begin_union(" << extensibility_kind(ek) << ")) return false;\n";

    const bool has_disc = has_discriminator(u, filter_kind);
    if (has_disc) {
      be_global->impl_ <<
        "  if (!value_reader.begin_discriminator()) return false;\n"
        " " << scoped(discriminator->name()) << " d;\n";
      generate_read("d", "", "disc", discriminator, "i", 1, nested(filter_kind));
      be_global->impl_ <<
        "  if (!value_reader.end_discriminator()) return false;\n";
    }

    if (filter_kind == FieldFilter_All) {
      generateSwitchForUnion(u, "d", branch_helper, branches,
                             discriminator, "", "", type_name.c_str(),
                             false, false);
      be_global->impl_ <<
        "  value._d(d);\n";
    } else if (has_disc) {
      // Assigning the discriminator directly before a branch is set doesn't strictly
      // conform to the IDL-to-C++ mapping. However, this case cares only about the
      // discriminator, i.e. KeyOnly or NestedKeyOnly samples, so it's probably fine.
      be_global->impl_ <<
        "  value.value._d(d);\n";
    }

    be_global->impl_ <<
      "  if (!value_reader.end_union()) return false;\n"
      "  return true;\n";
    return true;
  }
}

bool value_reader_generator::gen_enum(AST_Enum*,
                                      UTL_ScopedName* name,
                                      const std::vector<AST_EnumVal*>&,
                                      const char*)
{
  be_global->add_include("dds/DCPS/ValueReader.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);

  NamespaceGuard guard;
  Function read("vread", "bool");
  read.addArg("value_reader", "OpenDDS::DCPS::ValueReader&");
  read.addArg("value", type_name + "&");
  read.endArgs();
  be_global->impl_ <<
    "  return value_reader.read_enum(value, * ::OpenDDS::DCPS::gen_"
    << scoped_helper(name, "_") << "_helper);\n";
  return true;
}

bool value_reader_generator::gen_typedef(AST_Typedef*,
                                         UTL_ScopedName*,
                                         AST_Type*,
                                         const char*)
{
  return true;
}

bool value_reader_generator::gen_struct(AST_Structure* node,
                                        UTL_ScopedName* name,
                                        const std::vector<AST_Field*>& /*fields*/,
                                        AST_Type::SIZE_TYPE,
                                        const char*)
{
  be_global->add_include("dds/DCPS/Util.h", BE_GlobalData::STREAM_H);
  be_global->add_include("dds/DCPS/ValueReader.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  const ExtensibilityKind ek = be_global->extensibility(node);

  NamespaceGuard guard;
  if (!gen_struct_i(node, type_name, use_cxx11, ek, FieldFilter_All) ||
      !gen_struct_i(node, type_name, use_cxx11, ek, FieldFilter_NestedKeyOnly)) {
    return false;
  }

  if (be_global->is_topic_type(node)) {
    if (!gen_struct_i(node, type_name, use_cxx11, ek, FieldFilter_KeyOnly)) {
      return false;
    }
  }
  return true;
}

bool value_reader_generator::gen_union(AST_Union* u,
                                       UTL_ScopedName* name,
                                       const std::vector<AST_UnionBranch*>& branches,
                                       AST_Type* discriminator,
                                       const char*)
{
  be_global->add_include("dds/DCPS/Util.h", BE_GlobalData::STREAM_H);
  be_global->add_include("dds/DCPS/ValueReader.h", BE_GlobalData::STREAM_H);

  const std::string type_name = scoped(name);
  const ExtensibilityKind ek = be_global->extensibility(u);

  NamespaceGuard guard;
  if (!gen_union_i(u, type_name, branches, discriminator, ek, FieldFilter_All) ||
      !gen_union_i(u, type_name, branches, discriminator, ek, FieldFilter_NestedKeyOnly)) {
    return false;
  }

  if (be_global->is_topic_type(u)) {
    if (!gen_union_i(u, type_name, branches, discriminator, ek, FieldFilter_KeyOnly)) {
      return false;
    }
  }
  return true;
}
