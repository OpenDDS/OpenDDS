/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "metaclass_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

using namespace AstTypeClassification;

namespace {
  struct ContentSubscriptionGuard {
    ContentSubscriptionGuard()
    {
      be_global->header_ << "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n";
      be_global->impl_ << "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n";
    }
    ~ContentSubscriptionGuard()
    {
      be_global->header_ << "#endif\n";
      be_global->impl_ << "#endif\n";
    }
  };
}

bool metaclass_generator::gen_enum(UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char*)
{
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  std::string decl = "const char* gen_" + scoped_helper(name, "_") + "_names[]";
  be_global->header_ << "extern " << decl << ";\n";
  be_global->impl_ << decl << " = {\n";
  for (size_t i = 0; i < contents.size(); ++i) {
    be_global->impl_ << "  \"" << contents[i]->local_name()->get_string()
      << ((i < contents.size() - 1) ? "\",\n" : "\"\n");
  }
  be_global->impl_ << "};\n";
  return true;
}

namespace {

  void gen_field_getValue(AST_Field* field)
  {
    Classification cls = classify(field->field_type());
    const std::string fieldName = field->local_name()->get_string();
    if (cls & CL_SCALAR) {
      std::string prefix, suffix;
      if (cls & CL_ENUM) {
        AST_Type* enum_type = field->field_type();
        resolveActualType(enum_type);
        prefix = "gen_" +
          dds_generator::scoped_helper(enum_type->name(), "_")
          + "_names[";
        suffix = "]";
      }
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      return " + prefix + "typed." + fieldName
        + (cls & CL_STRING ? ".in()" : "") + suffix + ";\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_STRUCTURE) {
      size_t n = fieldName.size() + 1 /* 1 for the dot */;
      std::string fieldType = scoped(field->field_type()->name());
      be_global->impl_ <<
        "    if (std::strncmp(field, \"" << fieldName << ".\", " << n
        << ") == 0) {\n"
        "      return getMetaStruct<" << fieldType << ">().getValue(&typed."
        << fieldName << ", field + " << n << ");\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    }
  }

  void gen_field_createQC(AST_Field* field)
  {
    Classification cls = classify(field->field_type());
    const std::string fieldName = field->local_name()->get_string();
    if (cls & CL_SCALAR) {
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      return make_field_cmp(&T::" << fieldName << ", next);\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_STRUCTURE) {
      size_t n = fieldName.size() + 1 /* 1 for the dot */;
      std::string fieldType = scoped(field->field_type()->name());
      be_global->impl_ <<
        "    if (std::strncmp(field, \"" << fieldName << ".\", " << n <<
        ") == 0) {\n"
        "      return make_struct_cmp(&T::" << fieldName <<
        ", getMetaStruct<" << fieldType << ">().create_qc_comparator("
        "field + " << n << ", 0), next);\n"
        "    }\n";
    }
  }

  void print_field_name(AST_Field* field)
  {
    be_global->impl_ << '"' << field->local_name()->get_string() << '"' << ", ";
  }

  void get_raw_field(AST_Field* field)
  {
    const char* fieldName = field->local_name()->get_string();
    be_global->impl_ <<
      "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
      "      return &static_cast<const T*>(stru)->" << fieldName << ";\n"
      "    }\n";
    be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
  }

  void assign_field(AST_Field* field)
  {
    Classification cls = classify(field->field_type());
    if (!cls) return; // skip CL_UNKNOWN types
    const char* fieldName = field->local_name()->get_string();
    const std::string fieldType = (cls & CL_STRING) ?
      ((cls & CL_WIDE) ? "TAO::WString_Manager" : "TAO::String_Manager")
      : scoped(field->field_type()->name());
    if (cls & (CL_SCALAR | CL_STRUCTURE | CL_SEQUENCE | CL_UNION)) {
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      static_cast<T*>(lhs)->" << fieldName <<
        " = *static_cast<const " << fieldType <<
        "*>(rhsMeta.getRawField(rhs, rhsFieldSpec));\n"
        "      return;\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_ARRAY) {
      AST_Type* unTD = field->field_type();
      resolveActualType(unTD);
      AST_Array* arr = AST_Array::narrow_from_decl(unTD);
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      " << fieldType << "* lhsArr = &static_cast<T*>(lhs)->" <<
        fieldName << ";\n"
        "      const " << fieldType << "* rhsArr = static_cast<const " <<
        fieldType << "*>(rhsMeta.getRawField(rhs, rhsFieldSpec));\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
      AST_Type* elem = arr->base_type();
      AST_Type* elemUnTD = elem;
      resolveActualType(elemUnTD);
      if (classify(elemUnTD) & CL_ARRAY) {
        // array-of-array case, fall back on the Serializer
        be_global->impl_ <<
          "      " << fieldType << "_forany rhsForany(const_cast<" <<
          fieldType << "_slice*>(*rhsArr));\n"
          "      ACE_Message_Block mb(gen_find_size(rhsForany));\n"
          "      Serializer ser_out(&mb);\n"
          "      ser_out << rhsForany;\n"
          "      " << fieldType << "_forany lhsForany(*lhsArr);\n"
          "      Serializer ser_in(&mb);\n"
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

  void compare_field(AST_Field* field)
  {
    Classification cls = classify(field->field_type());
    if (!(cls & CL_SCALAR)) return;
    const char* fieldName = field->local_name()->get_string();
    be_global->impl_ <<
      "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n";
    be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    if (cls & CL_STRING) {
      be_global->impl_ << // ACE_OS::strcmp has overloads for narrow & wide
        "      " << "return 0 == ACE_OS::strcmp(static_cast<const T*>(lhs)->" <<
        fieldName << ", static_cast<const T*>(rhs)->" << fieldName << ");\n";
    } else {
      be_global->impl_ <<
        "      return static_cast<const T*>(lhs)->" << fieldName <<
        " == static_cast<const T*>(rhs)->" << fieldName << ";\n";
    }
    be_global->impl_ << "    }\n";
  }
}

bool metaclass_generator::gen_struct(UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, const char*)
{
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/FilterEvaluator.h",
    BE_GlobalData::STREAM_CPP);
  if (first_struct_) {
    be_global->header_ <<
      "class MetaStruct;\n\n"
      "template<typename T>\n"
      "const MetaStruct& getMetaStruct();\n\n";
    first_struct_ = false;
  }

  size_t nKeys = 0;
  IDL_GlobalData::DCPS_Data_Type_Info* info = idl_global->is_dcps_type(name);
  if (info) {
    nKeys = info->key_list_.size();
  }

  std::string clazz = scoped(name);
  std::string decl = "const MetaStruct& getMetaStruct<" + clazz + ">()",
    exp = be_global->export_macro().c_str();
  be_global->header_ << "template<>\n" << exp << (exp.length() ? "\n" : "")
    << decl << ";\n";

  be_global->impl_ <<
    "template<>\n"
    "struct MetaStructImpl<" << clazz << "> : MetaStruct {\n"
    "  typedef " << clazz << " T;\n\n"
    "  void* allocate() const { return new T; }\n\n"
    "  void deallocate(void* stru) const { delete static_cast<T*>(stru); }\n\n"
    "  size_t numDcpsKeys() const { return " << nKeys << "; }\n\n"
    "  Value getValue(const void* stru, const char* field) const\n"
    "  {\n"
    "    const " << clazz << "& typed = *static_cast<const " << clazz
    << "*>(stru);\n";
  std::for_each(fields.begin(), fields.end(), gen_field_getValue);
  std::string exception =
    "    throw std::runtime_error(\"Field \" + std::string(field) + \" not "
    "found or its type is not supported (in Struct " + clazz + ")\");\n";
  be_global->impl_ <<
    "    ACE_UNUSED_ARG(typed);\n" <<
    exception <<
    "  }\n\n"
    "  ComparatorBase::Ptr create_qc_comparator(const char* field, "
    "ComparatorBase::Ptr next) const\n"
    "  {\n"
    "    ACE_UNUSED_ARG(next);\n";
  be_global->add_include("<stdexcept>", BE_GlobalData::STREAM_CPP);
  std::for_each(fields.begin(), fields.end(), gen_field_createQC);
  be_global->impl_ <<
    exception <<
    "  }\n\n"
    "  const char** getFieldNames() const\n"
    "  {\n"
    "    static const char* names[] = {";
  std::for_each(fields.begin(), fields.end(), print_field_name);
  be_global->impl_ <<
    "0};\n"
    "    return names;\n"
    "  }\n\n"
    "  const void* getRawField(const void* stru, const char* field) const\n"
    "  {\n";
  std::for_each(fields.begin(), fields.end(), get_raw_field);
  be_global->impl_ <<
    exception <<
    "  }\n\n"
    "  void assign(void* lhs, const char* field, const void* rhs,\n"
    "    const char* rhsFieldSpec, const MetaStruct& rhsMeta) const\n"
    "  {\n"
    "    ACE_UNUSED_ARG(lhs);\n"
    "    ACE_UNUSED_ARG(field);\n"
    "    ACE_UNUSED_ARG(rhs);\n"
    "    ACE_UNUSED_ARG(rhsFieldSpec);\n"
    "    ACE_UNUSED_ARG(rhsMeta);\n";
  std::for_each(fields.begin(), fields.end(), assign_field);
  be_global->impl_ <<
    exception <<
    "  }\n\n"
    "  bool compare(const void* lhs, const void* rhs, const char* field) "
    "const\n"
    "  {\n"
    "    ACE_UNUSED_ARG(lhs);\n"
    "    ACE_UNUSED_ARG(field);\n"
    "    ACE_UNUSED_ARG(rhs);\n";
  std::for_each(fields.begin(), fields.end(), compare_field);
  be_global->impl_ <<
    exception <<
    "  }\n"
    "};\n\n"
    "template<>\n"
    << decl << "\n"
    "{\n"
    "  static MetaStructImpl<" << clazz << "> msi;\n"
    "  return msi;\n"
    "}\n\n";
  return true;
}
