/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "typeobject_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include "topic_keys.h"

using std::string;
using namespace AstTypeClassification;

namespace {

void
call_get_minimal_type_identifier(AST_Type* type)
{
  AST_Type* const actual_type = resolveActualType(type);

  if (!type->in_main_file() && type->node_type() != AST_Decl::NT_pre_defined) {
    be_global->add_referenced(type->file_name().c_str());
  }

  // If an alias, then use the name.
  if (type != actual_type) {
    be_global->impl_ << "getMinimalTypeIdentifier<" << typeobject_generator::tag_type(type->name()) << ">()";
    return;
  }

  const Classification fld_cls = classify(type);

  if (fld_cls & CL_FIXED) { // XTypes has no Fixed type in its data model
    be_global->impl_ << "getMinimalTypeIdentifier<void>() /* No Fixed in XTypes */";
    return;
  }

  // If primitive, then use built-in name.
  const Classification CL_WCHAR = CL_SCALAR | CL_PRIMITIVE | CL_WIDE;
  if ((fld_cls & CL_WCHAR) == CL_WCHAR) {
    be_global->impl_ << "getMinimalTypeIdentifier<ACE_OutputCDR::from_wchar>()";
    return;
  }

  if ((fld_cls & (CL_STRING | CL_BOUNDED)) == (CL_STRING | CL_BOUNDED)) {
    AST_String* const str = AST_String::narrow_from_decl(type);
    const unsigned int bound = str->max_size()->ev()->u.ulval;
    if (bound > 255) {
      be_global->impl_ <<
        "XTypes::TypeIdentifier::makeString(" << bool(fld_cls & CL_WIDE)
                                              << ", XTypes::StringLTypeDefn(" << bound << "))";
    } else {
      be_global->impl_ <<
        "XTypes::TypeIdentifier::makeString(" << bool(fld_cls & CL_WIDE)
                                              << ", XTypes::StringSTypeDefn(" << bound << "))";
    }

    return;
  }

  if (((fld_cls & (CL_SCALAR | CL_PRIMITIVE)) == (CL_SCALAR | CL_PRIMITIVE)) ||
      ((fld_cls & CL_STRING) == CL_STRING)) {
    be_global->impl_ << "getMinimalTypeIdentifier<" << scoped(type->name()) << ">()";
    return;
  }

  if (fld_cls & CL_SEQUENCE) {
    // TODO [anonymous]: Is an anonymous sequence always plain?
    AST_Sequence* sequence = AST_Sequence::narrow_from_decl(type);
    ACE_CDR::ULong size = 0;

    if (!sequence->unbounded()) {
      size = sequence->max_size()->ev()->u.ulval;
    }

    if (size > 255) {
      be_global->impl_ <<
        "XTypes::TypeIdentifier::makePlainSequence(";
      call_get_minimal_type_identifier(sequence->base_type());
      be_global->impl_ << ", XTypes::LBound(" << size << "))";
    } else {
      be_global->impl_ <<
        "XTypes::TypeIdentifier::makePlainSequence(";
      call_get_minimal_type_identifier(sequence->base_type());
      be_global->impl_ << ", XTypes::SBound(" << size << "))";
    }
    return;
  }

  if (fld_cls & CL_ARRAY) {
    // TODO [anonymous]: Is an anonymous array always plain?
    AST_Array* array = AST_Array::narrow_from_decl(type);

    ACE_CDR::ULong max_bound = 0;

    for (ACE_CDR::ULong dim = 0; dim != array->n_dims(); ++dim) {
      max_bound = std::max(max_bound, array->dims()[dim]->ev()->u.ulval);
    }

    if (max_bound > 255) {
      be_global->impl_ <<
        "XTypes::TypeIdentifier::makePlainArray(";
      call_get_minimal_type_identifier(array->base_type());
      be_global->impl_ << ", XTypes::LBoundSeq()";
      for (ACE_CDR::ULong dim = 0; dim != array->n_dims(); ++dim) {
        be_global->impl_ << ".append(XTypes::LBound(" << array->dims()[dim]->ev()->u.ulval << "))";
      }
      be_global->impl_ << ")";
    } else {
      be_global->impl_ <<
        "XTypes::TypeIdentifier::makePlainArray(";
      call_get_minimal_type_identifier(array->base_type());
      be_global->impl_ << ", XTypes::SBoundSeq()";
      for (ACE_CDR::ULong dim = 0; dim != array->n_dims(); ++dim) {
        be_global->impl_ << ".append(XTypes::SBound(" << array->dims()[dim]->ev()->u.ulval << "))";
      }
      be_global->impl_ << ")";
    }
    return;
  }

  if (fld_cls & (CL_STRUCTURE | CL_ENUM | CL_UNION)) {
    // Currently, we don't have anonymous structs or enums.
    be_global->impl_ << "getMinimalTypeIdentifier<" << typeobject_generator::tag_type(type->name()) << ">()";
    return;
  }

  // Anonymous.  Construct type object and hash to get type identifier.
  // TODO [anonymous]:
  be_global->impl_ << "ANONYMOUS";
  return;
}

}

string typeobject_generator::tag_type(UTL_ScopedName* name)
{
  return dds_generator::scoped_helper(name, "_") + "_xtag";
}

bool
typeobject_generator::gen_enum(AST_Enum* node, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char*)
{
  be_global->add_include("dds/DCPS/XTypes/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = tag_type(name);

  be_global->header_ << "struct " << clazz << " {};\n";

  {
    const string decl_gto = "getMinimalTypeObject<" + clazz + ">";
    Function gto(decl_gto.c_str(), "const XTypes::TypeObject&", "");
    gto.endArgs();
    const ExtensibilityKind exten = be_global->extensibility(node);
    std::string type_flag_str;
    gen_type_flag_str(type_flag_str, exten, false, node);
    be_global->impl_ <<
      "  static const XTypes::TypeObject to = XTypes::TypeObject(\n"
      "    XTypes::MinimalTypeObject(\n"
      "      XTypes::MinimalEnumeratedType(\n"
      "        XTypes::EnumTypeFlag("<< type_flag_str <<"),\n"
      "        XTypes::MinimalEnumeratedHeader(\n"
      "          XTypes::CommonEnumeratedHeader(\n"
      "            XTypes::BitBound(32)\n" // TODO:  Fill in with @bit_bound annotation.
      "          )\n"
      "        ),\n"
      "        XTypes::MinimalEnumeratedLiteralSeq()\n";

    size_t default_literal_idx = 0;
    for (size_t i = 0; i != contents.size(); ++i) {
      if (contents[i]->annotations().find("@default_literal")) {
        default_literal_idx = i;
      }
    }

    for (size_t i = 0; i != contents.size(); ++i) {
      be_global->impl_ <<
        "        .append(\n"
        "          XTypes::MinimalEnumeratedLiteral(\n"
        "            XTypes::CommonEnumeratedLiteral(\n"
        "              " << contents[i]->constant_value()->ev()->u.eval << ",\n"
        "              XTypes::EnumeratedLiteralFlag(\n"
        "                " << (i == default_literal_idx ? "XTypes::IS_DEFAULT" : "0") <<
        "              )"
        "            ),\n"
        "            XTypes::MinimalMemberDetail(\n"
        "              \"" << contents[i]->local_name()->get_string() << "\"\n"
        "            )\n"
        "          )\n"
        "        )\n";
    }

    be_global->impl_ <<
      "        .sort()\n"
      "      )\n"
      "    )\n"
      "  );\n"
      "  return to;\n";
  }
  {
    const string decl_gti = "getMinimalTypeIdentifier<" + clazz + ">";
    Function gti(decl_gti.c_str(), "XTypes::TypeIdentifier", "");
    gti.endArgs();
    be_global->impl_ <<
      "  static const XTypes::TypeIdentifier ti = XTypes::makeTypeIdentifier(getMinimalTypeObject<" << clazz << ">());\n"
      "  return ti;\n";
  }
  return true;
}

bool
typeobject_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE, const char*)
{
  be_global->add_include("dds/DCPS/XTypes/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = tag_type(name);

  be_global->header_ << "struct " << clazz << " {};\n";

  {
    const string decl_gto = "getMinimalTypeObject<" + clazz + ">";
    Function gto(decl_gto.c_str(), "const XTypes::TypeObject&", "");
    gto.endArgs();
    const ExtensibilityKind exten = be_global->extensibility(node);
    std::string type_flag_str;
    gen_type_flag_str(type_flag_str, exten, true, node);
    if (!be_global->is_topic_type(node)) {
      type_flag_str += " | XTypes::IS_NESTED";
    }
    if (0) { //TODO: Change once HASHID is supported in be_global.cpp
      type_flag_str += " | XTypes::IS_AUTOID_HASH";
    }
    // TODO: Support struct inheritance.
    be_global->impl_ <<
      "  static const XTypes::TypeObject to = XTypes::TypeObject(\n"
      "    XTypes::MinimalTypeObject(\n"
      "      XTypes::MinimalStructType(\n"
      "        XTypes::StructTypeFlag( " <<  type_flag_str << " ),\n"
      "        XTypes::MinimalStructHeader(\n"
      "          getMinimalTypeIdentifier<void>(),\n"
      "          XTypes::MinimalTypeDetail()\n"
      "        ),\n"
      "        XTypes::MinimalStructMemberSeq()\n";

    ACE_CDR::ULong member_id = 0;

    for (std::vector<AST_Field*>::const_iterator pos = fields.begin(), limit = fields.end(); pos != limit; ++pos) {
      string member_string;
      const TryConstructFailAction trycon = be_global->try_construct(*pos);
      gen_member_flag_str(member_string, trycon, *pos);
      bool is_key;
      be_global->check_key(*pos, is_key);
      if (is_key) {
        member_string += " | XTypes::IS_KEY";
      }
      be_global->impl_ <<
        "        .append(\n"
        "          XTypes::MinimalStructMember(\n"
        "            XTypes::CommonStructMember(\n"
        "              " << member_id++ << ",\n" // TODO: other kinds of memberid, see marshal_generator::gen_struct calling BE_GlobalData::get_id

        "              XTypes::StructMemberFlag( "<< member_string << " ),\n" //TODO: Additional flags are possible but not implemented
        "              ";
      call_get_minimal_type_identifier((*pos)->field_type());
      be_global->impl_ << "\n"
        "            ),\n"
        "            XTypes::MinimalMemberDetail(\"" << (*pos)->local_name()->get_string() << "\")\n"
        "          )\n"
        "        )\n";
    }

    be_global->impl_ <<
      "        .sort()\n"
      "        )\n"
      "      )\n"
      "    );\n"
      "  return to;\n";
  }
  {
    const string decl_gti = "getMinimalTypeIdentifier<" + clazz + ">";
    Function gti(decl_gti.c_str(), "XTypes::TypeIdentifier", "");
    gti.endArgs();
    be_global->impl_ <<
      "  static const XTypes::TypeIdentifier ti = XTypes::makeTypeIdentifier(getMinimalTypeObject<" << clazz << ">());\n"
      "  return ti;\n";
  }
  return true;
}

bool
typeobject_generator::gen_typedef(AST_Typedef* node, UTL_ScopedName* name,
                                  AST_Type* base, const char*)
{
  be_global->add_include("dds/DCPS/XTypes/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = tag_type(name);

  be_global->header_ << "struct " << clazz << " {};\n";

  {
    const string decl_gto = "getMinimalTypeObject<" + clazz + ">";
    Function gto(decl_gto.c_str(), "const XTypes::TypeObject&", "");
    gto.endArgs();
    be_global->impl_ <<
      "  static const XTypes::TypeObject to = XTypes::TypeObject(\n"
      "    XTypes::MinimalTypeObject(\n"
      "      XTypes::MinimalAliasType(\n"
      "        XTypes::AliasTypeFlag(0),\n" //TODO: Currently not used according to spec
      "        XTypes::MinimalAliasHeader(),\n"
      "        XTypes::MinimalAliasBody(\n"
      "          XTypes::CommonAliasBody(\n"
      "            XTypes::AliasMemberFlag(),\n" // TODO: Currently not used according to spec
      "            ";
    call_get_minimal_type_identifier(base);
    be_global->impl_ <<
      "\n"
      "          )\n"
      "        )\n"
      "      )\n"
      "    )\n"
      "  );\n"
      "  return to;\n";
  }
  {
    const string decl_gti = "getMinimalTypeIdentifier<" + clazz + ">";
    Function gti(decl_gti.c_str(), "XTypes::TypeIdentifier", "");
    gti.endArgs();
    be_global->impl_ <<
      "  static const XTypes::TypeIdentifier ti = XTypes::makeTypeIdentifier(getMinimalTypeObject<" << clazz << ">());\n"
      "  return ti;\n";
  }

  return true;
}

bool
typeobject_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
  const char*)
{
  be_global->add_include("dds/DCPS/XTypes/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = tag_type(name);

  be_global->header_ << "struct " << clazz << " {};\n";

  {
    const string decl_gto = "getMinimalTypeObject<" + clazz + ">";
    Function gto(decl_gto.c_str(), "const XTypes::TypeObject&", "");
    gto.endArgs();
    const ExtensibilityKind exten = be_global->extensibility(node);
    std::string type_flag_str;
    gen_type_flag_str(type_flag_str, exten, true, node);
    if (!be_global->is_topic_type(node)) {
      type_flag_str += " | XTypes::IS_NESTED";
    }
    string member_string;
    const TryConstructFailAction trycon = be_global->try_construct(discriminator);
    gen_member_flag_str(member_string, trycon, discriminator);
    if (be_global->has_key(node)) {
      member_string += " | XTypes::IS_KEY";
    }
    be_global->impl_ <<
      "  static const XTypes::TypeObject to = XTypes::TypeObject(\n"
      "    XTypes::MinimalTypeObject(\n"
      "      XTypes::MinimalUnionType(\n"
      "        XTypes::UnionTypeFlag( " << type_flag_str <<" ),\n"
      "        XTypes::MinimalUnionHeader(\n"
      "          XTypes::MinimalTypeDetail()\n" // Not used.
      "        ),\n"
      "        XTypes::MinimalDiscriminatorMember(\n"
      "          XTypes::CommonDiscriminatorMember(\n"
      "            XTypes::UnionDiscriminatorFlag(" << member_string << "),\n"
      "            ";
    call_get_minimal_type_identifier(discriminator);
    be_global->impl_ <<
      "\n"
      "          )\n"
      "        ),\n"
      "        XTypes::MinimalUnionMemberSeq()\n";
    ACE_CDR::ULong member_id = 0;
    for (std::vector<AST_UnionBranch*>::const_iterator pos = branches.begin(), limit = branches.end(); pos != limit; ++pos) {
      AST_UnionBranch* branch = *pos;
      bool is_default = false;
      for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
        AST_UnionLabel* label = branch->label(j);
        if (label->label_kind() == AST_UnionLabel::UL_default) {
          is_default = true;
          break;
        }
      }
      string member_string;
      const TryConstructFailAction trycon = be_global->try_construct(*pos);
      gen_member_flag_str(member_string, trycon, *pos);
      be_global->impl_ <<
        "        .append(\n"
        "          XTypes::MinimalUnionMember(\n"
        "            XTypes::CommonUnionMember(\n"
        "              " << member_id++ << ",\n"
        "              XTypes::UnionMemberFlag(\n"
        "                " << member_string << "\n"; //TODO: Additional flags are possible but not implemented
      if (is_default) {
        be_global->impl_ <<
          "              | XTypes::IS_DEFAULT\n";
      }

      be_global->impl_ <<
        "              ),\n"
        "              ";
      call_get_minimal_type_identifier(branch->field_type());
      be_global->impl_ <<
        ",\n"
        "              XTypes::UnionCaseLabelSeq()\n";

      for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
        AST_UnionLabel* label = branch->label(j);
        if (label->label_kind() == AST_UnionLabel::UL_default) {
          // Do nothing.
        } else if (discriminator->node_type() == AST_Decl::NT_enum) {
          be_global->impl_ <<
            "              .append(static_cast<ACE_CDR::Long>(" << getEnumLabel(label->label_val(), discriminator) << "))\n";
        } else {
          be_global->impl_ <<
            "              .append(" << *label->label_val()->ev() << ")\n";
        }
      }

      be_global->impl_ <<
        "              .sort()\n"
        "            ),\n"
        "            XTypes::MinimalMemberDetail(\"" << (*pos)->local_name()->get_string() << "\")\n"
        "          )\n"
        "        )\n";
    }

    be_global->impl_ <<
      "      .sort()\n"
      "      )\n"
      "    )\n"
      "  );\n"
      "  return to;\n";
  }
  {
    const string decl_gti = "getMinimalTypeIdentifier<" + clazz + ">";
    Function gti(decl_gti.c_str(), "XTypes::TypeIdentifier", "");
    gti.endArgs();
    be_global->impl_ <<
      "  static const XTypes::TypeIdentifier ti = XTypes::makeTypeIdentifier(getMinimalTypeObject<" << clazz << ">());\n"
      "  return ti;\n";
  }
  return true;
}

void typeobject_generator::gen_type_flag_str(std::string& type_flag_str,
                                             ExtensibilityKind exten,
                                             bool can_be_mutable,
                                             AST_Decl* node)
{
  switch (exten) {
  case extensibilitykind_final:
    type_flag_str = "XTypes::IS_FINAL";
    break;
  case extensibilitykind_appendable:
    type_flag_str = "XTypes::IS_APPENDABLE";
    break;
  case extensibilitykind_mutable:
    if (can_be_mutable) {
      type_flag_str = "XTypes::IS_MUTABLE";
    } else {
      idl_global->err()->misc_error(
        "Unexpected extensibility while setting flags: type cannot be mutable", node);
    }
    break;
  default:
    idl_global->err()->misc_error(
      "Unexpected extensibility while setting flags", node);
  }
}

void typeobject_generator::gen_member_flag_str(std::string& member_flag_str,
                                               TryConstructFailAction trycon,
                                               AST_Decl* node)
{
  switch (trycon) {
  case tryconstructfailaction_discard:
    member_flag_str = "XTypes::TryConstructDiscardValue";
    break;
  case tryconstructfailaction_use_default:
    member_flag_str = "XTypes::TryConstructUseDefaultValue";
    break;
  case tryconstructfailaction_trim:
    member_flag_str = "XTypes::TryConstructTrimValue";
    break;
  default:
    idl_global->err()->misc_error(
      "Unexpected try_construct value while setting flags", node);
  }
}
