/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "metaclass_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include "topic_keys.h"

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

bool
metaclass_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char*)
{
  NamespaceGuard ng;
  std::string array_decl = "const char* gen_" + scoped_helper(name, "_") + "_names[]";
  std::string size_decl = "const size_t gen_" + scoped_helper(name, "_") + "_names_size";
  std::string decl_prefix = ((be_global->export_macro() == "") ? std::string("extern ") : (std::string(be_global->export_macro().c_str()) + " extern "));
  be_global->header_ << decl_prefix << array_decl << ";\n";
  be_global->header_ << decl_prefix << size_decl << ";\n";
  be_global->impl_ << array_decl << " = {\n";
  for (size_t i = 0; i < contents.size(); ++i) {
    be_global->impl_ << "  \"" << contents[i]->local_name()->get_string()
      << ((i < contents.size() - 1) ? "\",\n" : "\"\n");
  }
  be_global->impl_ << "};\n";
  be_global->impl_ << size_decl << " = " << contents.size() << ";\n";
  return true;
}

namespace {

  void
  delegateToNested(const std::string& fieldName, AST_Field* field,
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
        "      if (!gen_skip_over(" << firstArg << ", static_cast<" << fieldType
        << "*>(0))) {\n"
        "        throw std::runtime_error(\"Field '" << fieldName <<
        "' could not be skipped\");\n"
        "      }\n"
        "    }\n";
    }
  }

  void
  gen_field_getValue(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const Classification cls = classify(field->field_type());
    const std::string fieldName = field->local_name()->get_string();
    if (cls & CL_SCALAR) {
      std::string prefix, suffix;
      if (cls & CL_ENUM) {
        AST_Type* enum_type = resolveActualType(field->field_type());
        prefix = "gen_" +
          dds_generator::scoped_helper(enum_type->name(), "_")
          + "_names[";
        if (use_cxx11) {
          prefix += "static_cast<int>(";
        }
        suffix = use_cxx11 ? "())]" : "]";
      } else if (use_cxx11) {
        suffix += "()";
      }
      const std::string string_to_ptr = use_cxx11 ? "" : ".in()";
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      return " + prefix + "typed." + fieldName
        + (cls & CL_STRING ? string_to_ptr : "") + suffix + ";\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_STRUCTURE) {
      delegateToNested(fieldName, field,
                       "&typed." + std::string(use_cxx11 ? "_" : "") + fieldName);
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    }
  }

  std::string string_type(Classification cls)
  {
    return be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11 ?
      ((cls & CL_WIDE) ? "std::wstring" : "std::string") :
      (cls & CL_WIDE) ? "TAO::WString_Manager" : "TAO::String_Manager";
  }

  std::string
  to_cxx_type(AST_Type* type, int& size)
  {
    const Classification cls = classify(type);
    if (cls & CL_ENUM) {
      size = 4;
      return "ACE_CDR::ULong";
    }
    if (cls & CL_STRING) {
      size = 4; // encoding of str length is 4 bytes
      return string_type(cls);
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

  void
  gen_field_getValueFromSerialized(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    AST_Type* type = field->field_type();
    const Classification cls = classify(type);
    const std::string fieldName = field->local_name()->get_string();
    int size = 0;
    const std::string cxx_type = to_cxx_type(type, size);
    if (cls & CL_SCALAR) {
      type = resolveActualType(type);
      const std::string val = (cls & CL_STRING) ? (use_cxx11 ? "val" : "val.out()")
        : getWrapper("val", type, WD_INPUT);
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
          "      if (!ser.skip(static_cast<ACE_UINT16>(len))) {\n"
          "        throw std::runtime_error(\"String '" << fieldName <<
          "' contents could not be skipped\");\n"
          "      }\n";
      } else if (cls & CL_WIDE) {
        be_global->impl_ <<
          "      ACE_CDR::Octet len;\n"
          "      if (!(ser >> ACE_InputCDR::to_octet(len))) {\n"
          "        throw std::runtime_error(\"WChar '" << fieldName <<
          "' length could not be deserialized\");\n"
          "      }\n"
          "      if (!ser.skip(static_cast<ACE_UINT16>(len))) {\n"
          "        throw std::runtime_error(\"WChar '" << fieldName <<
          "' contents could not be skipped\");\n"
          "      }\n";
      } else {
        be_global->impl_ <<
          "      if (!ser.skip(1, " << size << ")) {\n"
          "        throw std::runtime_error(\"Field '" << fieldName <<
          "' could not be skipped\");\n"
          "      }\n";
      }
      be_global->impl_ <<
        "    }\n";
    } else if (cls & CL_STRUCTURE) {
      delegateToNested(fieldName, field, "ser", true);
    } else { // array, sequence, union:
      std::string pre, post;
      if (!use_cxx11 && (cls & CL_ARRAY)) {
        post = "_forany";
      } else if (use_cxx11 && (cls & (CL_ARRAY | CL_SEQUENCE))) {
        pre = "IDL::DistinctType<";
        post = ", " + dds_generator::scoped_helper(type->name(), "_") + "_tag>";
      }
      be_global->impl_ <<
        "    if (!gen_skip_over(ser, static_cast<" << pre << cxx_type << post
        << "*>(0))) {\n"
        "      throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \""
        " could not be skipped\");\n"
        "    }\n";
    }
  }

  void
  gen_field_createQC(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    Classification cls = classify(field->field_type());
    const std::string fieldName = field->local_name()->get_string();
    if (cls & CL_SCALAR) {
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      return make_field_cmp(&T::" << (use_cxx11 ? "_" : "")
        << fieldName << ", next);\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_STRUCTURE) {
      size_t n = fieldName.size() + 1 /* 1 for the dot */;
      std::string fieldType = scoped(field->field_type()->name());
      be_global->impl_ <<
        "    if (std::strncmp(field, \"" << fieldName << ".\", " << n <<
        ") == 0) {\n"
        "      return make_struct_cmp(&T::" << (use_cxx11 ? "_" : "")
        << fieldName <<
        ", getMetaStruct<" << fieldType << ">().create_qc_comparator("
        "field + " << n << "), next);\n"
        "    }\n";
    }
  }

  void
  print_field_name(AST_Field* field)
  {
    be_global->impl_ << '"' << field->local_name()->get_string() << '"' << ", ";
  }

  void
  get_raw_field(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const char* fieldName = field->local_name()->get_string();
    be_global->impl_ <<
      "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
      "      return &static_cast<const T*>(stru)->" << (use_cxx11 ? "_" : "")
      << fieldName << ";\n"
      "    }\n";
    be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
  }

  void
  assign_field(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    Classification cls = classify(field->field_type());
    if (!cls) return; // skip CL_UNKNOWN types
    const char* fieldName = field->local_name()->get_string();
    const std::string fieldType = (cls & CL_STRING) ?
      string_type(cls)
      : scoped(field->field_type()->name());
    if ((cls & (CL_SCALAR | CL_STRUCTURE | CL_SEQUENCE | CL_UNION))
        || (use_cxx11 && (cls & CL_ARRAY))) {
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      static_cast<T*>(lhs)->" << (use_cxx11 ? "_" : "") << fieldName <<
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

  void
  compare_field(AST_Field* field)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    Classification cls = classify(field->field_type());
    if (!(cls & CL_SCALAR)) return;
    const char* fieldName = field->local_name()->get_string();
    be_global->impl_ <<
      "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n";
    be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    if (!use_cxx11 && (cls & CL_STRING)) {
      be_global->impl_ << // ACE_OS::strcmp has overloads for narrow & wide
        "      return 0 == ACE_OS::strcmp(static_cast<const T*>(lhs)->"
        << fieldName << ".in(), static_cast<const T*>(rhs)->" << fieldName
        << ".in());\n";
    } else {
      be_global->impl_ <<
        "      return static_cast<const T*>(lhs)->" << (use_cxx11 ? "_" : "")
        << fieldName << " == static_cast<const T*>(rhs)->" << (use_cxx11 ? "_" : "")
        << fieldName << ";\n";
    }
    be_global->impl_ << "    }\n";
  }

  void
  gen_isDcpsKey_i(const char* key)
  {
    be_global->impl_ <<
      "    if (!ACE_OS::strcmp(field, \"" <<  key << "\")) {\n"
      "      return true;\n"
      "    }\n";
  }

  void
  gen_isDcpsKey(IDL_GlobalData::DCPS_Data_Type_Info* info)
  {
    IDL_GlobalData::DCPS_Key_List::CONST_ITERATOR i(info->key_list_);
    for (ACE_TString* key = 0; i.next(key); i.advance()) {
      gen_isDcpsKey_i(ACE_TEXT_ALWAYS_CHAR(key->c_str()));
    }
  }

  void
  gen_isDcpsKey(TopicKeys& keys)
  {
    TopicKeys::Iterator finished = keys.end();
    for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
      gen_isDcpsKey_i(i.path().c_str());
    }
  }

  bool
  generate_metaclass(AST_Decl* node, UTL_ScopedName* name,
    const std::vector<AST_Field*>& fields, bool& first_struct_,
    const std::string& clazz)
  {
    AST_Structure* struct_node = 0;
    AST_Union* union_node = 0;
    if (!node || !name) {
      return false;
    }
    if (node->node_type() == AST_Decl::NT_struct) {
      struct_node = dynamic_cast<AST_Structure*>(node);
    } else if (node->node_type() == AST_Decl::NT_union) {
      union_node = dynamic_cast<AST_Union*>(node);
    } else {
      return false;
    }

    if (first_struct_) {
      be_global->header_ <<
        "class MetaStruct;\n\n"
        "template<typename T>\n"
        "const MetaStruct& getMetaStruct();\n\n";
      first_struct_ = false;
    }

    be_global->add_include("dds/DCPS/FilterEvaluator.h",
      BE_GlobalData::STREAM_CPP);

    std::string decl = "const MetaStruct& getMetaStruct<" + clazz + ">()",
      exp = be_global->export_macro().c_str();
    be_global->header_ << "template<>\n" << exp << (exp.length() ? "\n" : "")
      << decl << ";\n";

    size_t key_count = 0;
    IDL_GlobalData::DCPS_Data_Type_Info* info = 0;
    const bool is_topic_type = be_global->is_topic_type(node);
    TopicKeys keys;
    if (struct_node) {
      info = idl_global->is_dcps_type(name);
      if (is_topic_type) {
        keys = TopicKeys(struct_node);
        key_count = keys.count();
      } else if (info) {
        key_count = info->key_list_.size();
      }
    } else { // Union
      key_count = be_global->has_key(union_node) ? 1 : 0;
    }

    be_global->impl_ <<
      "template<>\n"
      "struct MetaStructImpl<" << clazz << "> : MetaStruct {\n"
      "  typedef " << clazz << " T;\n\n"
      "#ifndef OPENDDS_NO_MULTI_TOPIC\n"
      "  void* allocate() const { return new T; }\n\n"
      "  void deallocate(void* stru) const { delete static_cast<T*>(stru); }\n\n"
      "  size_t numDcpsKeys() const { return " << key_count << "; }\n\n"
      "#endif /* OPENDDS_NO_MULTI_TOPIC */\n\n"
      "  bool isDcpsKey(const char* field) const\n"
      "  {\n";
    {
      /* TODO: Unions are not handled here because we don't know how queries
       * should work with unions or how this would work if they did. Maybe
       * create a separate has_key method like be_global has, since the key
       * would always be the union discriminator?
       */
      if (struct_node && key_count) {
        if (info) {
          gen_isDcpsKey(info);
        } else {
          gen_isDcpsKey(keys);
        }
      } else {
        be_global->impl_ << "    ACE_UNUSED_ARG(field);\n";
      }
    }
    be_global->impl_ <<
      "    return false;\n"
      "  }\n\n"
      "  Value getValue(const void* stru, const char* field) const\n"
      "  {\n"
      "    const " << clazz << "& typed = *static_cast<const " << clazz << "*>(stru);\n"
      "    ACE_UNUSED_ARG(typed);\n";
    std::for_each(fields.begin(), fields.end(), gen_field_getValue);
    const std::string exception =
      "    throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \" not "
      "found or its type is not supported (in struct " + clazz + ")\");\n";
    be_global->impl_ <<
      exception <<
      "  }\n\n"
      "  Value getValue(Serializer& ser, const char* field) const\n"
      "  {\n";
    if (struct_node && fields.size()) {
      std::for_each(fields.begin(), fields.end(), gen_field_getValueFromSerialized);
    } else {
      be_global->impl_ << "    ACE_UNUSED_ARG(ser);\n";
    }
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
    if (struct_node) {
      std::for_each(fields.begin(), fields.end(), gen_field_createQC);
    }
    be_global->impl_ <<
      exception <<
      "  }\n\n"
      "#ifndef OPENDDS_NO_MULTI_TOPIC\n"
      "  const char** getFieldNames() const\n"
      "  {\n"
      "    static const char* names[] = {";
    if (struct_node) {
      std::for_each(fields.begin(), fields.end(), print_field_name);
    }
    be_global->impl_ <<
      "0};\n"
      "    return names;\n"
      "  }\n\n"
      "  const void* getRawField(const void* stru, const char* field) const\n"
      "  {\n";
    if (struct_node && fields.size()) {
      std::for_each(fields.begin(), fields.end(), get_raw_field);
    } else {
      be_global->impl_ << "    ACE_UNUSED_ARG(stru);\n";
    }
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
    if (struct_node) {
      std::for_each(fields.begin(), fields.end(), assign_field);
    }
    be_global->impl_ <<
      exception <<
      "  }\n"
      "#endif /* OPENDDS_NO_MULTI_TOPIC */\n\n"
      "  bool compare(const void* lhs, const void* rhs, const char* field) "
      "const\n"
      "  {\n"
      "    ACE_UNUSED_ARG(lhs);\n"
      "    ACE_UNUSED_ARG(field);\n"
      "    ACE_UNUSED_ARG(rhs);\n";
    if (struct_node) {
      std::for_each(fields.begin(), fields.end(), compare_field);
    }
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
}

bool
metaclass_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE, const char*)
{
  const std::string clazz = scoped(name);
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/PoolAllocator.h",
    BE_GlobalData::STREAM_CPP);

  if (!generate_metaclass(node, name, fields, first_struct_, clazz)) {
    return false;
  }

  {
    Function f("gen_skip_over", "bool");
    f.addArg("ser", "Serializer&");
    f.addArg("", clazz + "*");
    f.endArgs();
    be_global->impl_ <<
      "  MetaStructImpl<" << clazz << ">().getValue(ser, \"\");\n"
      "  return true;\n";
  }
  return true;
}

bool
metaclass_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name,
  AST_Type* type, const char*)
{
  AST_Array* arr = AST_Array::narrow_from_decl(type);
  AST_Sequence* seq = 0;
  if (!arr && !(seq = AST_Sequence::narrow_from_decl(type))) {
    return true;
  }
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

  const std::string clazz = scoped(name), clazz_underscores = scoped_helper(name, "_");
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  Function f("gen_skip_over", "bool");
  f.addArg("ser", "Serializer&");
  if (use_cxx11) {
    f.addArg("", "IDL::DistinctType<" + clazz + ", " + clazz_underscores + "_tag>*");
  } else {
    f.addArg("", clazz + (arr ? "_forany*" : "*"));
  }
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
      "  if (!(ser >> length)) return false;\n";
    len = "length";
  }

  const std::string cxx_elem = scoped(elem->name());
  AST_Type* elem_orig = elem;
  elem = resolveActualType(elem);
  const Classification elem_cls = classify(elem);

  if ((elem_cls & (CL_PRIMITIVE | CL_ENUM)) && !(elem_cls & CL_WIDE)) {
    // fixed-length sequence/array element -> skip all elements at once
    int sz = 1;
    to_cxx_type(elem, sz);
    be_global->impl_ <<
      "  return ser.skip(static_cast<ACE_UINT16>(" << len << "), " << sz << ");\n";
  } else {
    be_global->impl_ <<
      "  for (ACE_CDR::ULong i = 0; i < " << len << "; ++i) {\n";
    if ((elem_cls & CL_PRIMITIVE) && (elem_cls & CL_WIDE)) {
      be_global->impl_ <<
        "    ACE_CDR::Octet o;\n"
        "    if (!(ser >> ACE_InputCDR::to_octet(o))) return false;\n"
        "    if (!ser.skip(o)) return false;\n";
    } else if (elem_cls & CL_STRING) {
      be_global->impl_ <<
        "    ACE_CDR::ULong strlength;\n"
        "    if (!(ser >> strlength)) return false;\n"
        "    if (!ser.skip(static_cast<ACE_UINT16>(strlength))) return false;\n";
    } else if (elem_cls & (CL_ARRAY | CL_SEQUENCE | CL_STRUCTURE)) {
      std::string pre, post;
      if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
        post = "_forany";
      } else if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
        pre = "IDL::DistinctType<";
        post = ", " + dds_generator::scoped_helper(elem_orig->name(), "_") + "_tag>";
      }
      be_global->impl_ <<
        "    if (!gen_skip_over(ser, static_cast<" << pre << cxx_elem << post
        << "*>(0))) return false;\n";
    }
    be_global->impl_ <<
      "  }\n";
    be_global->impl_ <<
      "  return true;\n";
  }

  return true;
}

static std::string
func(const std::string&, AST_Type* br_type, const std::string&,
  std::string&, const std::string&)
{
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  std::stringstream ss;
  const Classification br_cls = classify(br_type);
  if (br_cls & CL_STRING) {
    ss <<
      "    ACE_CDR::ULong len;\n"
      "    if (!(ser >> len)) return false;\n"
      "    if (!ser.skip(static_cast<ACE_UINT16>(len))) return false;\n";
  } else if (br_cls & CL_WIDE) {
    ss <<
      "    ACE_CDR::Octet len;\n"
      "    if (!(ser >> ACE_InputCDR::to_octet(len))) return false;\n"
      "    if (!ser.skip(len)) return false;\n";
  } else if (br_cls & CL_SCALAR) {
    int sz = 1;
    to_cxx_type(br_type, sz);
    ss <<
      "    if (!ser.skip(1, " << sz << ")) return false;\n";
  } else {
    std::string pre, post;
    if (!use_cxx11 && (br_cls & CL_ARRAY)) {
      post = "_forany";
    } else if (use_cxx11 && (br_cls & (CL_ARRAY | CL_SEQUENCE))) {
      pre = "IDL::DistinctType<";
      post = ", " + dds_generator::scoped_helper(br_type->name(), "_") + "_tag>";
    }
    ss <<
      "    if (!gen_skip_over(ser, static_cast<" << pre
      << scoped(br_type->name()) << post << "*>(0))) return false;\n";
  }

  ss <<
    "    return true;\n";
  return ss.str();
}

bool
metaclass_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
  const char*)
{
  const std::string clazz = scoped(name);
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;

  std::vector<AST_Field*> dummy_field_list;
  if (!generate_metaclass(node, name, dummy_field_list, first_struct_, clazz)) {
    return false;
  }

  {
    Function f("gen_skip_over", "bool");
    f.addArg("ser", "Serializer&");
    f.addArg("", clazz + "*");
    f.endArgs();
    be_global->impl_ <<
      "  " << scoped(discriminator->name()) << " disc;\n"
      "  if (!(ser >> " << getWrapper("disc", discriminator, WD_INPUT) << ")) {\n"
      "    return false;\n"
      "  }\n";
    if (generateSwitchForUnion("disc", func, branches, discriminator, "", "", "",
                               false, true, false)) {
      be_global->impl_ <<
        "  return true;\n";
    }
  }
  return true;
}
