/*
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
    explicit ContentSubscriptionGuard(bool activate = true)
      : activate_(activate)
    {
      if (activate) {
        be_global->header_ <<
          "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n";
        be_global->impl_ <<
          "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n";
      }
    }
    ~ContentSubscriptionGuard()
    {
      if (activate_) {
        be_global->header_ << "#endif\n";
        be_global->impl_ << "#endif\n";
      }
    }
    bool activate_;
  };
}

bool metaclass_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char*)
{
  ContentSubscriptionGuard csg(!be_global->v8());
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

  void delegateToNested(const std::string& fieldName, AST_Field* field,
                        const std::string& firstArg, bool skip = false)
  {
    const size_t n = fieldName.size() + 1 /* 1 for the dot */;
    const std::string fieldType = scoped(field->field_type()->name());
    be_global->impl_ <<
      "    if (std::strncmp(field, \"" << fieldName << ".\", " << n
      << ") == 0) {\n"
      "      return getMetaStruct<" << fieldType << ">().getValue("
      << firstArg << ", field + " << n << ");\n"
      "    }" << (skip ? "" : "\n");
    if (skip) {
      be_global->impl_ << " else {\n"
        "      gen_skip_over(" << firstArg << ", static_cast<" << fieldType
        << "*>(0));\n"
        "    }\n";
    }
  }

  void gen_field_getValue(AST_Field* field)
  {
    const Classification cls = classify(field->field_type());
    const std::string fieldName = field->local_name()->get_string();
    if (cls & CL_SCALAR) {
      std::string prefix, suffix;
      if (cls & CL_ENUM) {
        AST_Type* enum_type = resolveActualType(field->field_type());
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
      delegateToNested(fieldName, field, "&typed." + fieldName);
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    }
  }

  std::string to_cxx_type(AST_Type* type, int& size)
  {
    const Classification cls = classify(type);
    if (cls & CL_ENUM) {
      size = 4;
      return "ACE_CDR::ULong";
    }
    if (cls & CL_STRING) {
      size = 4; // encoding of str length is 4 bytes
      return ((cls & CL_WIDE) ? "TAO::W" : "TAO::")
        + std::string("String_Manager");
    }
    if (cls & CL_PRIMITIVE) {
      type = resolveActualType(type);
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
      switch (p->pt()) {
      case AST_PredefinedType::PT_long:
        size = 4;
        return "ACE_CDR::Long";
      case AST_PredefinedType::PT_ulong:
        size = 4;
        return "ACE_CDR::ULong";
      case AST_PredefinedType::PT_longlong:
        size = 8;
        return "ACE_CDR::LongLong";
      case AST_PredefinedType::PT_ulonglong:
        size = 8;
        return "ACE_CDR::ULongLong";
      case AST_PredefinedType::PT_short:
        size = 2;
        return "ACE_CDR::Short";
      case AST_PredefinedType::PT_ushort:
        size = 2;
        return "ACE_CDR::UShort";
      case AST_PredefinedType::PT_float:
        size = 4;
        return "ACE_CDR::Float";
      case AST_PredefinedType::PT_double:
        size = 8;
        return "ACE_CDR::Double";
      case AST_PredefinedType::PT_longdouble:
        size = 16;
        return "ACE_CDR::LongDouble";
      case AST_PredefinedType::PT_char:
        size = 1;
        return "ACE_CDR::Char";
      case AST_PredefinedType::PT_wchar:
        size = 1; // encoding of wchar length is 1 byte
        return "ACE_CDR::WChar";
      case AST_PredefinedType::PT_boolean:
        size = 1;
        return "ACE_CDR::Boolean";
      case AST_PredefinedType::PT_octet:
        size = 1;
        return "ACE_CDR::Octet";
      default:
        break;
      }
    }
    return scoped(type->name());
  }

  void gen_field_getValueFromSerialized(AST_Field* field)
  {
    AST_Type* type = field->field_type();
    const Classification cls = classify(type);
    const std::string fieldName = field->local_name()->get_string();
    int size = 0;
    const std::string cxx_type = to_cxx_type(type, size);
    if (cls & CL_SCALAR) {
      type = resolveActualType(type);
      const std::string val =
        (cls & CL_STRING) ? "val.out()" : getWrapper("val", type, WD_INPUT);
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      " << cxx_type << " val;\n"
        "      if (!(ser >> " << val << ")) {\n"
        "        throw std::runtime_error(\"Field '" << fieldName << "' could "
        "not be deserialized\");\n"
        "      }\n"
        "      return val;\n"
        "    } else {\n";
      if (cls & CL_STRING) {
        be_global->impl_ <<
          "      ACE_CDR::ULong len;\n"
          "      if (!(ser >> len)) {\n"
          "        throw std::runtime_error(\"String '" << fieldName <<
          "' length could not be deserialized\");\n"
          "      }\n"
          "      ser.skip(len);\n";
      } else if (cls & CL_WIDE) {
        be_global->impl_ <<
          "      ACE_CDR::Octet len;\n"
          "      if (!(ser >> ACE_InputCDR::to_octet(len))) {\n"
          "        throw std::runtime_error(\"WChar '" << fieldName <<
          "' length could not be deserialized\");\n"
          "      }\n"
          "      ser.skip(len);\n";
      } else {
        be_global->impl_ <<
          "      ser.skip(1, " << size << ");\n";
      }
      be_global->impl_ <<
        "    }\n";
    } else if (cls & CL_STRUCTURE) {
      delegateToNested(fieldName, field, "ser", true);
    } else { // array, sequence, union:
      be_global->impl_ <<
        "    gen_skip_over(ser, static_cast<" << cxx_type
        << ((cls & CL_ARRAY) ? "_forany" : "") << "*>(0));\n";
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
        "field + " << n << "), next);\n"
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
      AST_Type* unTD = resolveActualType(field->field_type());
      AST_Array* arr = AST_Array::narrow_from_decl(unTD);
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      " << fieldType << "* lhsArr = &static_cast<T*>(lhs)->" <<
        fieldName << ";\n"
        "      const " << fieldType << "* rhsArr = static_cast<const " <<
        fieldType << "*>(rhsMeta.getRawField(rhs, rhsFieldSpec));\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
      AST_Type* elem = arr->base_type();
      AST_Type* elemUnTD = resolveActualType(elem);
      if (classify(elemUnTD) & CL_ARRAY) {
        // array-of-array case, fall back on the Serializer
        be_global->impl_ <<
          "      " << fieldType << "_forany rhsForany(const_cast<" <<
          fieldType << "_slice*>(*rhsArr));\n"
          "      size_t size = 0, padding = 0;\n"
          "      gen_find_size(rhsForany, size, padding);\n"
          "      ACE_Message_Block mb(size);\n"
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
        "      return 0 == ACE_OS::strcmp(static_cast<const T*>(lhs)->"
        << fieldName << ".in(), static_cast<const T*>(rhs)->" << fieldName
        << ".in());\n";
    } else {
      be_global->impl_ <<
        "      return static_cast<const T*>(lhs)->" << fieldName <<
        " == static_cast<const T*>(rhs)->" << fieldName << ";\n";
    }
    be_global->impl_ << "    }\n";
  }
}

bool metaclass_generator::gen_struct(AST_Structure*, UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE, const char*)
{
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/PoolAllocator.h",
    BE_GlobalData::STREAM_CPP);
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
  const std::string exception =
    "    throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \" not "
    "found or its type is not supported (in struct " + clazz + ")\");\n";
  be_global->impl_ <<
    "    ACE_UNUSED_ARG(typed);\n" <<
    exception <<
    "  }\n\n"
    "  Value getValue(Serializer& ser, const char* field) const\n"
    "  {\n";
  std::for_each(fields.begin(), fields.end(), gen_field_getValueFromSerialized);
  be_global->impl_ <<
    "    if (!field[0]) {\n"   // if 'field' is the empty string...
    "      return 0;\n"        // ...we've skipped the entire struct
    "    }\n"                  //    and the return value is ignored
    "    throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \" not "
    "valid for struct " << clazz << "\");\n"
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
  {
    Function f("gen_skip_over", "void");
    f.addArg("ser", "Serializer&");
    f.addArg("", clazz + "*");
    f.endArgs();
    be_global->impl_ <<
      "  MetaStructImpl<" << clazz << ">().getValue(ser, \"\");\n";
  }
  return true;
}

bool
metaclass_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* type, const char*)
{
  AST_Array* arr = AST_Array::narrow_from_decl(type);
  AST_Sequence* seq = 0;
  if (!arr && !(seq = AST_Sequence::narrow_from_decl(type))) {
    return true;
  }

  const Classification cls = classify(type);
  const std::string clazz = scoped(name);
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  Function f("gen_skip_over", "void");
  f.addArg("ser", "Serializer&");
  f.addArg("", clazz + ((cls & CL_ARRAY) ? "_forany*" : "*"));
  f.endArgs();

  std::string len;
  AST_Type* elem;

  if (arr) {
    elem = arr->base_type();
    size_t n_elems = 1;
    for (size_t i = 0; i < arr->n_dims(); ++i) {
      n_elems *= arr->dims()[i]->ev()->u.ulval;
    }
    std::ostringstream strstream;
    strstream << n_elems;
    len = strstream.str();
  } else { // Sequence
    elem = seq->base_type();
    be_global->impl_ <<
      "  ACE_CDR::ULong length;\n"
      "  ser >> length;\n";
    len = "length";
  }

  const std::string cxx_elem = scoped(elem->name());
  elem = resolveActualType(elem);
  const Classification elem_cls = classify(elem);

  if ((elem_cls & (CL_PRIMITIVE | CL_ENUM)) && !(elem_cls & CL_WIDE)) {
    // fixed-length sequence/array element -> skip all elements at once
    int sz = 1;
    to_cxx_type(elem, sz);
    be_global->impl_ <<
      "  ser.skip(" << len << ", " << sz << ");\n";
  } else {
    be_global->impl_ <<
      "  for (ACE_CDR::ULong i = 0; i < " << len << "; ++i) {\n";
    if ((elem_cls & CL_PRIMITIVE) && (elem_cls & CL_WIDE)) {
      be_global->impl_ <<
        "    ACE_CDR::Octet o;\n"
        "    ser >> ACE_InputCDR::to_octet(o);\n"
        "    ser.skip(o);\n";
    } else if (elem_cls & CL_STRING) {
      be_global->impl_ <<
        "    ACE_CDR::ULong strlength;\n"
        "    ser >> strlength;\n"
        "    ser.skip(strlength);\n";
    } else if (elem_cls & (CL_ARRAY | CL_SEQUENCE | CL_STRUCTURE)) {
      be_global->impl_ <<
        "    gen_skip_over(ser, static_cast<" << cxx_elem
        << ((elem_cls & CL_ARRAY) ? "_forany" : "") << "*>(0));\n";
    }
    be_global->impl_ <<
      "  }\n";
  }

  return true;
}

static std::string func(const std::string&,
                        AST_Type* br_type,
                        const std::string&,
                        std::string&,
                        const std::string&)
{
  std::stringstream ss;
  const Classification br_cls = classify(br_type);
  if (br_cls & CL_STRING) {
    ss <<
      "      ACE_CDR::ULong len;\n"
      "      ser >> len;\n"
      "      ser.skip(len);\n";
  } else if (br_cls & CL_WIDE) {
    ss <<
      "      ACE_CDR::Octet len;\n"
      "      ser >> ACE_InputCDR::to_octet(len);\n"
      "      ser.skip(len);\n";
  } else if (br_cls & CL_SCALAR) {
    int sz = 1;
    to_cxx_type(br_type, sz);
    ss <<
      "    ser.skip(1, " << sz << ");\n";
  } else {
    ss <<
      "    gen_skip_over(ser, static_cast<" << scoped(br_type->name())
                                            << ((br_cls & CL_ARRAY) ? "_forany" : "") << "*>(0));\n";
  }

  return ss.str();
}

bool
metaclass_generator::gen_union(AST_Union*, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
  const char*)
{
  const std::string clazz = scoped(name);
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  Function f("gen_skip_over", "void");
  f.addArg("ser", "Serializer&");
  f.addArg("", clazz + "*");
  f.endArgs();
  be_global->impl_ <<
    "  " << scoped(discriminator->name()) << " disc;\n"
    "  if (!(ser >> " << getWrapper("disc", discriminator, WD_INPUT) << ")) {\n"
    "    return;\n"
    "  }\n";
  generateSwitchForUnion("disc", func, branches, discriminator, "");
  return true;
}
