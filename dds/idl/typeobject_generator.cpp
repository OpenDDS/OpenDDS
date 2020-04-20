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

bool
typeobject_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char*)
{
  be_global->add_include("dds/DCPS/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = scoped(name);
  const string decl = "getTypeIdentifier<" + clazz + ">";
  Function gti(decl.c_str(), "RcHandle<XTypes::TypeIdentifier>", "");
  gti.endArgs();
  be_global->impl_ << "  return RcHandle<XTypes::TypeIdentifier>();\n"; //TODO
  return true;
}

namespace {

string tag_type(UTL_ScopedName* name)
{
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  return (use_cxx11 ? dds_generator::scoped_helper(name, "_") : scoped(name))
    + "_tag";
}

void
call_get_type_identifier(AST_Type* type)
{
  const Classification fld_cls = classify(type);

  if (fld_cls & CL_FIXED) { // XTypes has no Fixed type in its data model
    be_global->impl_ << "getTypeIdentifier<void>() /* No Fixed in XTypes */";
    return;
  }

  if ((fld_cls & (CL_STRING | CL_BOUNDED)) == (CL_STRING | CL_BOUNDED)) {
    AST_Type* const atype = resolveActualType(type);
    AST_String* const str = AST_String::narrow_from_decl(atype);
    const unsigned int bound = str->max_size()->ev()->u.ulval;
    be_global->impl_ <<
      "XTypes::TypeIdentifier::makeBoundedString(" << bool(fld_cls & CL_WIDE)
                                                   << ", " << bound << ')';
    return;
  }

  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  const bool use_tag = (fld_cls & CL_ARRAY) ||
    (use_cxx11 && (fld_cls & CL_SEQUENCE));
  const string name = use_tag ? tag_type(type->name()) : scoped(type->name());
  be_global->impl_ << "getTypeIdentifier<" << name << ">()";
}

}

bool
typeobject_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE, const char*)
{
  be_global->add_include("dds/DCPS/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = scoped(name);

  {
    const string decl_gto = "getTypeObject<" + clazz + ">";
    Function gto(decl_gto.c_str(), "const XTypes::TypeObject&", "");
    gto.endArgs();

    // TODO: Pick the appropriate flags.
    // TODO: Support struct inheritance.
    be_global->impl_ <<
      "  static const XTypes::TypeObject to = XTypes::TypeObject::make(\n"
      "    XTypes::MinimalTypeObject::make(\n"
      "      XTypes::MinimalStructType::make(XTypes::IS_FINAL | XTypes::IS_NESTED | XTypes::IS_AUTOID_HASH,\n"
      "        XTypes::MinimalStructHeader::make(\n"
      "          getTypeIdentifier<void>(),\n"
      "          XTypes::MinimalTypeDetail::make()\n"
      "        ),\n"
      "        XTypes::MinimalStructMemberSeq()\n";

    ACE_CDR::ULong member_id = 0;
    for (std::vector<AST_Field*>::const_iterator pos = fields.begin(), limit = fields.end(); pos != limit; ++pos) {
      be_global->impl_ <<
        "        .append(\n"
        "          XTypes::MinimalStructMember::make(\n"
        "            XTypes::CommonStructMember::make(\n"
        "              " << member_id++ << ",\n"
      // TODO: Set StructMemberFlags.
        "              0,\n"
        "              ";
      call_get_type_identifier((*pos)->field_type());
      be_global->impl_ << "\n"
        "            ),\n"
        "            XTypes::MinimalMemberDetail::make(\"" << (*pos)->local_name()->get_string() << "\")\n"
        "          )\n"
        "        )\n";
    }

  be_global->impl_ <<
    "      )\n"
    "    )\n"
    "  );\n"
    "  return to;\n";
  }
  {
    const string decl_gti = "getTypeIdentifier<" + clazz + ">";
    Function gti(decl_gti.c_str(), "RcHandle<XTypes::TypeIdentifier>", "");
    gti.endArgs();
    be_global->impl_ <<
      "  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::makeTypeIdentifier(getTypeObject<" << clazz << ">());\n"
      "  return ti;\n";
  }
  return true;
}

bool
typeobject_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name,
                                  AST_Type* base, const char*)
{
  bool array = false;
  unsigned int bound = 0;
  AST_Type* elem;

  switch (base->node_type()) {
  case AST_Decl::NT_array:
    {
      AST_Array* arr = AST_Array::narrow_from_decl(base);
      elem = resolveActualType(arr->base_type());
      array = true;
      //   for (size_t i = 0; i < arr->n_dims(); ++i) {
      //      n_elems *= arr->dims()[i]->ev()->u.ulval;
      //    }
      break;
    }
  case AST_Decl::NT_sequence:
    {
      AST_Sequence* seq = AST_Sequence::narrow_from_decl(base);
      elem = resolveActualType(seq->base_type());
      if (!seq->unbounded()) {
        bound = seq->max_size()->ev()->u.ulval;
      }
      break;
    }
  default:
    return true;
  }

  Classification elem_cls = classify(elem);

  be_global->add_include("dds/DCPS/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = scoped(name);
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  const bool use_tag = array || use_cxx11;
  const string decl =
    "getTypeIdentifier<" + (use_tag ? tag_type(name) : clazz) + '>';
  Function gti(decl.c_str(), "RcHandle<XTypes::TypeIdentifier>", "");
  gti.endArgs();
  be_global->impl_ << "  return RcHandle<XTypes::TypeIdentifier>();\n"; //TODO
  return true;
}

bool
typeobject_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
  const char*)
{
  be_global->add_include("dds/DCPS/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = scoped(name);

  {
    const string decl_gto = "getTypeObject<" + clazz + ">";
    Function gto(decl_gto.c_str(), "const XTypes::TypeObject&", "");
    gto.endArgs();
    be_global->impl_ <<
      "  static const XTypes::TypeObject to = XTypes::TypeObject::make(XTypes::MinimalTypeObject());\n" //TODO
      "  return to;\n";
  }
  {
    const string decl_gti = "getTypeIdentifier<" + clazz + ">";
    Function gti(decl_gti.c_str(), "RcHandle<XTypes::TypeIdentifier>", "");
    gti.endArgs();
    be_global->impl_ <<
      "  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::makeTypeIdentifier(getTypeObject<" << clazz << ">());\n"
      "  return ti;\n";
  }
  return true;
}
