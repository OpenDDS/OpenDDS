/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ts_generator.h"

#include "be_extern.h"
#include "be_util.h"
#include "topic_keys.h"
#include "typeobject_generator.h"

#include <utl_identifier.h>

#include <ace/OS_NS_sys_stat.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <map>
#include <iostream>

using namespace AstTypeClassification;

namespace {
  std::string read_template(const char* prefix)
  {
    std::string path = be_util::dds_root();
    path.append("/dds/idl/");
    path.append(prefix);
    path.append("Template.txt");
    std::ifstream ifs(path.c_str());
    if (!ifs) {
      ACE_ERROR((LM_ERROR, "Error - Couldn't open %C\n", path.c_str()));
      return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  void replaceAll(std::string& s,
                  const std::map<std::string, std::string>& rep) {
    typedef std::map<std::string, std::string>::const_iterator mapiter_t;
    for (size_t i = s.find("<%"); i < s.length(); i = s.find("<%", i + 1)) {
      size_t n = s.find("%>", i) - i + 2;
      mapiter_t iter = rep.find(s.substr(i + 2, n - 4));
      if (iter != rep.end()) {
        s.replace(i, n, iter->second);
      }
    }
  }

  template<size_t N>
  void add_includes(const char* (&includes)[N],
                    BE_GlobalData::stream_enum_t whichStream) {
    for (size_t i = 0; i < N; ++i) {
      be_global->add_include(includes[i], whichStream);
    }
  }
}

ts_generator::ts_generator()
  : idl_template_(read_template("IDL"))
{
}

namespace {
  void gen_isDcpsKey_i(const char* key)
  {
    be_global->impl_ <<
      "  if (!ACE_OS::strcmp(field, \"" << key << "\")) {\n"
      "    return true;\n"
      "  }\n";
  }

  void gen_isDcpsKey(IDL_GlobalData::DCPS_Data_Type_Info* info)
  {
    IDL_GlobalData::DCPS_Key_List::CONST_ITERATOR i(info->key_list_);
    for (ACE_TString* key = 0; i.next(key); i.advance()) {
      gen_isDcpsKey_i(ACE_TEXT_ALWAYS_CHAR(key->c_str()));
    }
  }

  void gen_isDcpsKey(TopicKeys& keys)
  {
    TopicKeys::Iterator finished = keys.end();
    for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
      gen_isDcpsKey_i(i.canonical_path().c_str());
    }
  }
}

bool ts_generator::generate_ts(AST_Type* node, UTL_ScopedName* name)
{
  if (idl_template_.empty()) {
    // error reported in read_template
    return false;
  }
  if (!node || !name) {
    return false;
  }

  AST_Structure* struct_node = 0;
  AST_Union* union_node = 0;
  AST_Type::SIZE_TYPE size_type;
  if (node->node_type() == AST_Decl::NT_struct) {
    struct_node = dynamic_cast<AST_Structure*>(node);
    size_type = struct_node->size_type();
  } else if (node->node_type() == AST_Decl::NT_union) {
    union_node = dynamic_cast<AST_Union*>(node);
    size_type = union_node->size_type();
  } else {
    idl_global->err()->misc_error(
      "Could not cast AST Nodes to valid types", node);
    return false;
  }

  size_t key_count = 0;
  IDL_GlobalData::DCPS_Data_Type_Info* info = 0;
  TopicKeys keys;
  if (struct_node) {
    info = idl_global->is_dcps_type(name);
    if (be_global->is_topic_type(struct_node)) {
      keys = TopicKeys(struct_node);
      key_count = keys.count();
    } else if (info) {
      key_count = info->key_list_.size();
    } else {
      return true;
    }
  } else if (be_global->is_topic_type(union_node)) {
    key_count = be_global->union_discriminator_is_key(union_node) ? 1 : 0;
  } else {
    return true;
  }

  const std::string full_cxx_name = scoped(name);
  const std::string short_cxx_name = name->last_component()->get_string();
  const std::string ts_base = "OpenDDS::DCPS::TypeSupportImpl_T<" + short_cxx_name + ">";
  const std::string full_name_from_tsch = scoped(name, EscapeContext_FromGenIdl);
  const std::string short_name_from_tsch = to_string(
    name->last_component(), EscapeContext_FromGenIdl);
  const std::string full_ts_name = full_name_from_tsch + "TypeSupport";
  const std::string short_ts_name = short_name_from_tsch + "TypeSupport";
  const std::string full_tsi_name = full_ts_name + "Impl";
  const std::string short_tsi_name = short_ts_name + "Impl";
  const std::string unescaped_name =
    dds_generator::scoped_helper(name, "::", EscapeContext_StripEscapes);
  const std::string name_underscores = dds_generator::scoped_helper(name, "_");
  static const std::string ns("OpenDDS::DCPS::");
  static const std::string dds_ns = std::string(be_global->versioning_name().c_str()) + "::DDS::";
  const std::string xtag = ns + get_xtag_name(name);
  const bool op_ret_is_ptr = node->size_type() == AST_Type::VARIABLE;
  const std::string op_ret_name = short_cxx_name + (op_ret_is_ptr ? "*" : "");

  static const char* idl_includes[] = {
    "dds/DdsDcpsInfrastructure.idl",
    "dds/DdsDcpsTopic.idl",
    "dds/DdsDcpsPublication.idl",
    "dds/DdsDcpsSubscriptionExt.idl",
    "dds/DdsDcpsTypeSupportExt.idl",
    "dds/DdsDynamicData.idl"
  };
  add_includes(idl_includes, BE_GlobalData::STREAM_IDL);

  std::string dc = be_global->header_name_.c_str();
  dc.replace(dc.end() - 6, dc.end() - 2, "C"); // s/Impl.h$/C.h/
  be_global->add_include(dc.c_str());

  static const char* h_includes[] = {
    "dds/DCPS/TypeSupportImpl.h", "dds/DCPS/ValueDispatcher.h"
  };
  add_includes(h_includes, BE_GlobalData::STREAM_H);

  static const char* cpp_includes[] = {
    "dds/DCPS/debug.h", "dds/DCPS/Registered_Data_Types.h",
    "dds/DdsDcpsDomainC.h", "dds/DCPS/Service_Participant.h",
    "dds/DCPS/Qos_Helper.h", "dds/DCPS/PublicationInstance.h",
    "dds/DCPS/PublisherImpl.h", "dds/DCPS/SubscriberImpl.h",
    "dds/DCPS/ReceivedDataElementList.h", "dds/DCPS/RakeResults_T.h",
    "dds/DCPS/BuiltInTopicUtils.h", "dds/DCPS/Util.h",
    "dds/DCPS/ContentFilteredTopicImpl.h", "dds/DCPS/RakeData.h",
    "dds/DCPS/MultiTopicDataReader_T.h", "dds/DCPS/DataWriterImpl_T.h",
    "dds/DCPS/DataReaderImpl_T.h", "dds/DCPS/XTypes/TypeObject.h"
  };
  add_includes(cpp_includes, BE_GlobalData::STREAM_CPP);

  std::map<std::string, std::string> replacements;
  replacements["SCOPED"] = scoped(name, EscapeContext_ForGenIdl);
  // SCOPED_NOT_GLOBAL is EscapeContext_FromGenIdl, because
  // DCPS_DATA_SEQUENCE_TYPE is strange.
  replacements["SCOPED_NOT_GLOBAL"] =
    dds_generator::scoped_helper(name, "::", EscapeContext_FromGenIdl);
  replacements["TYPE"] = to_string(name->last_component(), EscapeContext_ForGenIdl);
  replacements["EXPORT"] = be_global->export_macro().c_str();
  replacements["SEQ"] = be_global->sequence_suffix().c_str();

  ScopedNamespaceGuard idlGuard(name, be_global->idl_, "module");
  std::string idl = idl_template_;
  replaceAll(idl, replacements);
  be_global->idl_ << idl;

  {
    ScopedNamespaceGuard hGuard(name, be_global->header_);

    be_global->header_ <<
      "class " << short_tsi_name << ";\n";
  }

  be_global->header_ <<
    "OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL\n"
    "namespace OpenDDS {\n"
    "namespace DCPS {\n"
    "template <>\n"
    "struct DDSTraits<" << full_cxx_name << "> {\n"
    "  typedef " << full_cxx_name << " MessageType;\n"
    "  typedef " << full_name_from_tsch << be_global->sequence_suffix() << " MessageSequenceType;\n"
    "  typedef " << full_name_from_tsch << be_global->sequence_suffix() << "::PrivateMemberAccess MessageSequenceAdapterType;\n"
    "  typedef " << full_ts_name << " TypeSupportType;\n"
    "  typedef " << full_tsi_name << " TypeSupportImplType;\n"
    "  typedef " << full_name_from_tsch << "DataWriter DataWriterType;\n"
    "  typedef " << full_name_from_tsch << "DataReader DataReaderType;\n"
    "  typedef " << full_cxx_name << "_OpenDDS_KeyLessThan LessThanType;\n"
    "  typedef OpenDDS::DCPS::KeyOnly<const " << full_cxx_name << "> KeyOnlyType;\n"
    "  typedef " << xtag << " XtagType;\n"
    "\n"
    "  static const char* type_name() { return \"" << unescaped_name << "\"; }\n"
    "  static size_t key_count() { return " << key_count << "; }\n"
    "  static bool is_key(const char*);\n"
    "};\n"
    "} // namespace DCPS\n"
    "} // namespace OpenDDS\n"
    "OPENDDS_END_VERSIONED_NAMESPACE_DECL\n\n";

  be_global->impl_ <<
    "OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL\n"
    "namespace OpenDDS {\n"
    "namespace DCPS {\n"
    "bool DDSTraits<" << full_cxx_name << ">::is_key(const char* field)\n"
    "{\n"
    "  ACE_UNUSED_ARG(field);\n";
  if (struct_node && key_count) {
    if (info) {
      gen_isDcpsKey(info);
    } else {
      gen_isDcpsKey(keys);
    }
  }
  be_global->impl_ <<
    "  return false;\n"
    "}\n"
    "} // namespace DCPS\n"
    "} // namespace OpenDDS\n"
    "OPENDDS_END_VERSIONED_NAMESPACE_DECL\n\n";

  {
    ScopedNamespaceGuard hGuard(name, be_global->header_);
    ScopedNamespaceGuard cppGuard(name, be_global->impl_);

    be_global->header_ <<
      "class " << be_global->export_macro() << " " << short_tsi_name << "\n"
      "  : public virtual " << ts_base << "\n"
      "  , public virtual OpenDDS::DCPS::ValueDispatcher_T<" << short_cxx_name << ">\n"
      "{\n"
      "public:\n"
      "  typedef " << short_ts_name << " TypeSupportType;\n"
      "  typedef " << short_ts_name << "::_var_type _var_type;\n"
      "  typedef " << short_ts_name << "::_ptr_type _ptr_type;\n"
      "\n"
      "  " << short_tsi_name << "() {}\n"
      "  virtual ~" << short_tsi_name << "() {}\n"
      "\n"
      "  virtual " << dds_ns << "DataWriter_ptr create_datawriter();\n"
      "  virtual " << dds_ns << "DataReader_ptr create_datareader();\n"
      "#ifndef OPENDDS_NO_MULTI_TOPIC\n"
      "  virtual " << dds_ns << "DataReader_ptr create_multitopic_datareader();\n"
      "#endif /* !OPENDDS_NO_MULTI_TOPIC */\n"
      "#ifndef OPENDDS_SAFETY_PROFILE\n"
      "  " << op_ret_name << " create_sample(" << dds_ns << "DynamicData_ptr src);\n"
      "#endif\n"
      "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n"
      "  virtual const OpenDDS::DCPS::MetaStruct& getMetaStructForType() const;\n"
      "#endif /* !OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE */\n"
      "\n"
      "  virtual const OpenDDS::XTypes::TypeIdentifier& getMinimalTypeIdentifier() const;\n"
      "  virtual const OpenDDS::XTypes::TypeMap& getMinimalTypeMap() const;\n"
      "\n"
      "  virtual const OpenDDS::XTypes::TypeIdentifier& getCompleteTypeIdentifier() const;\n"
      "  virtual const OpenDDS::XTypes::TypeMap& getCompleteTypeMap() const;\n"
      "\n"
      "  ::DDS::ReturnCode_t encode_to_string(const " << short_cxx_name << "& in, CORBA::String_out out, OpenDDS::DCPS::RepresentationFormat* format);\n"
      "  ::DDS::ReturnCode_t encode_to_bytes(const " << short_cxx_name << "& in, ::DDS::OctetSeq_out out, OpenDDS::DCPS::RepresentationFormat* format);\n"
      "  ::DDS::ReturnCode_t decode_from_string(const char* in, " << short_cxx_name << "_out out, OpenDDS::DCPS::RepresentationFormat* format);\n"
      "  ::DDS::ReturnCode_t decode_from_bytes(const ::DDS::OctetSeq& in, " << short_cxx_name << "_out out, OpenDDS::DCPS::RepresentationFormat* format);\n"
      "\n"
      "  static " << short_ts_name << "::_ptr_type _narrow(CORBA::Object_ptr obj);\n"
      "};\n\n";
    be_global->impl_ <<
      "::DDS::DataWriter_ptr " << short_tsi_name << "::create_datawriter()\n"
      "{\n"
      "  typedef OpenDDS::DCPS::DataWriterImpl_T<" << short_cxx_name << "> DataWriterImplType;\n"
      "  ::DDS::DataWriter_ptr writer_impl = ::DDS::DataWriter::_nil();\n"
      "  ACE_NEW_NORETURN(writer_impl,\n"
      "                   DataWriterImplType());\n"
      "  return writer_impl;\n"
      "}\n\n"
      "::DDS::DataReader_ptr " << short_tsi_name << "::create_datareader()\n"
      "{\n"
      "  typedef OpenDDS::DCPS::DataReaderImpl_T<" << short_cxx_name << "> DataReaderImplType;\n"
      "  ::DDS::DataReader_ptr reader_impl = ::DDS::DataReader::_nil();\n"
      "  ACE_NEW_NORETURN(reader_impl,\n"
      "                   DataReaderImplType());\n"
      "  return reader_impl;\n"
      "}\n\n"
      "#ifndef OPENDDS_NO_MULTI_TOPIC\n"
      "::DDS::DataReader_ptr " << short_tsi_name << "::create_multitopic_datareader()\n"
      "{\n"
      "  typedef OpenDDS::DCPS::DataReaderImpl_T<" << short_cxx_name << "> DataReaderImplType;\n"
      "  typedef OpenDDS::DCPS::MultiTopicDataReader_T<" << short_cxx_name << ", DataReaderImplType> MultiTopicDataReaderImplType;\n"
      "  ::DDS::DataReader_ptr multitopic_reader_impl = ::DDS::DataReader::_nil();\n"
      "  ACE_NEW_NORETURN(multitopic_reader_impl,\n"
      "                   MultiTopicDataReaderImplType());\n"
      "  return multitopic_reader_impl;\n"
      "}\n"
      "#endif /* !OPENDDS_NO_MULTI_TOPIC */\n"
      "\n"
      "#ifndef OPENDDS_SAFETY_PROFILE\n" <<
      op_ret_name << " " << short_tsi_name << "::create_sample(::DDS::DynamicData_ptr src)\n"
      "{\n"
      "  ";
    if (op_ret_is_ptr) {
      be_global->impl_ << short_cxx_name << "* value = new " << short_cxx_name << ";\n"
        "  if (" << ts_base << "::create_sample(value, src)) {\n"
        "    return value;\n"
        "  }\n"
        "  delete value;\n"
        "  return 0;\n";
    } else {
      be_global->impl_ << short_cxx_name << " value;\n"
        "  " << ts_base << "::create_sample(&value, src);\n"
        "  return value;\n";
    }
    be_global->impl_ <<
      "}\n"
      "#endif\n"
      "\n"
      "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n"
      "const OpenDDS::DCPS::MetaStruct& " << short_tsi_name << "::getMetaStructForType() const\n"
      "{\n"
      "  return OpenDDS::DCPS::getMetaStruct<" << short_cxx_name << ">();\n"
      "}\n"
      "#endif /* !OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE */\n\n"
      "namespace {\n"
      "  OpenDDS::DCPS::TypeSupportInitializer<" << short_tsi_name << "> ts_init_" << name_underscores << ";\n"
      "}\n"
      "\n"
      "const OpenDDS::XTypes::TypeIdentifier& " << short_tsi_name << "::getMinimalTypeIdentifier() const\n"
      "{\n";

    const bool java_ts_only = be_global->java_arg().length() > 0;
    const bool generate_xtypes = !be_global->suppress_xtypes() && !java_ts_only;
    if (generate_xtypes) {
      be_global->impl_ <<
        "  return OpenDDS::DCPS::getMinimalTypeIdentifier<" << xtag << ">();\n";
    } else {
      be_global->impl_ <<
        "  static OpenDDS::XTypes::TypeIdentifier ti;\n"
        "  return ti;\n";
    }
    be_global->impl_ <<
      "}\n\n"
      "const OpenDDS::XTypes::TypeMap& " << short_tsi_name << "::getMinimalTypeMap() const\n"
      "{\n";

    if (generate_xtypes) {
      be_global->impl_ <<
        "  return OpenDDS::DCPS::getMinimalTypeMap<" << xtag << ">();\n";
    } else {
      be_global->impl_ <<
        "  static OpenDDS::XTypes::TypeMap tm;\n"
        "  return tm;\n";
    }
    be_global->impl_ <<
      "}\n\n"
      "const OpenDDS::XTypes::TypeIdentifier& " << short_tsi_name << "::getCompleteTypeIdentifier() const\n"
      "{\n";

    const bool generate_xtypes_complete = generate_xtypes && be_global->xtypes_complete();
    if (generate_xtypes_complete) {
      be_global->impl_ <<
        "  return OpenDDS::DCPS::getCompleteTypeIdentifier<" << xtag << ">();\n";
    } else {
      be_global->impl_ <<
        "  static OpenDDS::XTypes::TypeIdentifier ti;\n"
        "  return ti;\n";
    }
    be_global->impl_ <<
      "}\n\n"
      "const OpenDDS::XTypes::TypeMap& " << short_tsi_name << "::getCompleteTypeMap() const\n"
      "{\n";

    if (generate_xtypes_complete) {
      be_global->impl_ <<
        "  return OpenDDS::DCPS::getCompleteTypeMap<" << xtag << ">();\n";
    } else {
      be_global->impl_ <<
        "  static OpenDDS::XTypes::TypeMap tm;\n"
        "  return tm;\n";
    }
    be_global->add_cpp_include("dds/DCPS/JsonValueReader.h");
    be_global->add_cpp_include("dds/DCPS/JsonValueWriter.h");
    const bool alloc_out = be_global->language_mapping() != BE_GlobalData::LANGMAP_CXX11 && size_type == AST_Type::VARIABLE;
    be_global->impl_ <<
      "}\n\n"
      "::DDS::ReturnCode_t " << short_tsi_name << "::encode_to_string(const " << short_cxx_name << "& in, CORBA::String_out out, OpenDDS::DCPS::RepresentationFormat* format)\n"
      "{\n"
      "#if OPENDDS_HAS_JSON_VALUE_WRITER\n"
      "  OpenDDS::DCPS::JsonRepresentationFormat_var jrf = OpenDDS::DCPS::JsonRepresentationFormat::_narrow(format);\n"
      "  if (jrf) {\n"
      "    rapidjson::StringBuffer buffer;\n"
      "    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);\n"
      "    OpenDDS::DCPS::JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);\n"
      "    if (!vwrite(jvw, in)) {\n"
      "      return ::DDS::RETCODE_ERROR;\n"
      "    }\n"
      "    out = buffer.GetString();\n"
      "    return ::DDS::RETCODE_OK;\n"
      "  }\n"
      "#else\n"
      "  ACE_UNUSED_ARG(in);\n"
      "  ACE_UNUSED_ARG(format);\n"
      "#endif\n"
      "  out = \"\";\n"
      "  return ::DDS::RETCODE_UNSUPPORTED;\n"
      "}\n\n"
      "::DDS::ReturnCode_t " << short_tsi_name << "::encode_to_bytes(const " << short_cxx_name << "& in, ::DDS::OctetSeq_out out, OpenDDS::DCPS::RepresentationFormat* format)\n"
      "{\n"
      "#if OPENDDS_HAS_JSON_VALUE_WRITER\n"
      "  OpenDDS::DCPS::JsonRepresentationFormat_var jrf = OpenDDS::DCPS::JsonRepresentationFormat::_narrow(format);\n"
      "  if (jrf) {\n"
      "    CORBA::String_var buffer;\n"
      "    const ::DDS::ReturnCode_t ret = encode_to_string(in, buffer, format);\n"
      "    if (ret == ::DDS::RETCODE_OK) {\n"
      "      const ::DDS::UInt32 len = static_cast< ::DDS::UInt32>(std::strlen(buffer));\n"
      "      out = new ::DDS::OctetSeq(len);\n"
      "      out->length(len);\n"
      "      std::memcpy(out->get_buffer(), buffer, len);\n"
      "      return ::DDS::RETCODE_OK;\n"
      "    } else {\n"
      "      out = new ::DDS::OctetSeq();\n"
      "      return ret;\n"
      "    }\n"
      "  }\n"
      "#else\n"
      "  ACE_UNUSED_ARG(in);\n"
      "  ACE_UNUSED_ARG(format);\n"
      "#endif\n"
      "  out = new ::DDS::OctetSeq();\n"
      "  return ::DDS::RETCODE_UNSUPPORTED;\n"
      "}\n\n"
      "::DDS::ReturnCode_t " << short_tsi_name << "::decode_from_string(const char* in, " << short_cxx_name << "_out " <<
      (alloc_out ? "out" : "param") << ", OpenDDS::DCPS::RepresentationFormat* format)\n"
      "{\n";

    if (alloc_out) {
      be_global->impl_ << "  out = new " << short_cxx_name << ";\n";
    } else {
      be_global->impl_ <<  "  " << short_cxx_name << "* out = &param;\n";
    }

    be_global->impl_ <<
      "  OpenDDS::DCPS::set_default(*out);\n"
      "#if OPENDDS_HAS_JSON_VALUE_READER\n"
      "  OpenDDS::DCPS::JsonRepresentationFormat_var jrf = OpenDDS::DCPS::JsonRepresentationFormat::_narrow(format);\n"
      "  if (jrf) {\n"
      "    rapidjson::StringStream buffer(in);\n"
      "    OpenDDS::DCPS::JsonValueReader<> jvr(buffer);\n" <<
      "    return vread(jvr, *out) ? ::DDS::RETCODE_OK : ::DDS::RETCODE_ERROR;\n"
      "  }\n"
      "#else\n"
      "  ACE_UNUSED_ARG(in);\n"
      "  ACE_UNUSED_ARG(format);\n"
      "#endif\n"
      "  return ::DDS::RETCODE_UNSUPPORTED;\n"
      "}\n\n"
      "::DDS::ReturnCode_t " << short_tsi_name << "::decode_from_bytes(const ::DDS::OctetSeq& in, " << short_cxx_name << "_out out, OpenDDS::DCPS::RepresentationFormat* format)\n"
      "{\n"
      "#if OPENDDS_HAS_JSON_VALUE_READER\n"
      "  OpenDDS::DCPS::JsonRepresentationFormat_var jrf = OpenDDS::DCPS::JsonRepresentationFormat::_narrow(format);\n"
      "  if (jrf) {\n"
      "    return decode_from_string(reinterpret_cast<const char*>(in.get_buffer()), out, format);\n"
      "  }\n"
      "#else\n"
      "  ACE_UNUSED_ARG(in);\n"
      "  ACE_UNUSED_ARG(format);\n"
      "#endif\n"
      "  out = " << (alloc_out ? "new " : "") << short_cxx_name << "();\n"
      "  return ::DDS::RETCODE_UNSUPPORTED;\n"
      "}\n\n"
      << short_ts_name << "::_ptr_type " << short_tsi_name << "::_narrow(CORBA::Object_ptr obj)\n"
      "{\n"
      "  return TypeSupportType::_narrow(obj);\n"
      "}\n";
  }

  if (be_global->face_ts()) {
    if (node->node_type() == AST_Decl::NT_struct) {
      face_ts_generator::generate(name);
    } else {
      idl_global->err()->misc_error(
        "Generating FACE type support for Union topic types is not supported", node);
      return false;
    }
  }

  return true;
}

bool ts_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE, const char*)
{
  if (be_global->generate_equality() && be_global->language_mapping() == BE_GlobalData::LANGMAP_NONE) {
    // == and != are generated as class members in other language mappings.

    const char* const nm = name->last_component()->get_string();

    ScopedNamespaceGuard hGuard(name, be_global->header_);
    ScopedNamespaceGuard cppGuard(name, be_global->impl_);

    be_global->header_
      << be_global->export_macro() << " bool operator==(const " << nm << "&, const " << nm << "&);\n"
      << be_global->export_macro() << " bool operator!=(const " << nm << "&, const " << nm << "&);\n";

    be_global->impl_ << "bool operator==(const " << nm << "& lhs, const " << nm << "& rhs)\n"
                      << "{\n";
    for (size_t i = 0; i < fields.size(); ++i) {
      const std::string field_name = fields[i]->local_name()->get_string();
      AST_Type* field_type = resolveActualType(fields[i]->field_type());
      const Classification cls = classify(field_type);
      if (cls & CL_ARRAY) {
        std::string indent("  ");
        NestedForLoops nfl("int", "i",
          dynamic_cast<AST_Array*>(field_type), indent, true);
        be_global->impl_ <<
          indent << "if (lhs." << field_name << nfl.index_ << " != rhs."
          << field_name << nfl.index_ << ") {\n" <<
          indent << "  return false;\n" <<
          indent << "}\n";
      } else if (cls & CL_SEQUENCE) {
        be_global->impl_ <<
          "  if (!OpenDDS::DCPS::sequence_equal(lhs." << field_name << ", rhs." << field_name << ")) return false;\n";
      } else {
        be_global->impl_ <<
          "  if (lhs." << field_name << " != rhs." << field_name << ") {\n"
          "    return false;\n"
          "  }\n";
      }
    }
    be_global->impl_ <<
      "  return true;\n"
      "}\n\n"
      "bool operator!=(const " << nm << "& lhs, const " << nm << "& rhs) { return !(lhs == rhs); }\n";
  }

  return generate_ts(node, name);
}

namespace {
  std::string generateEqual(const std::string&, AST_Decl*, const std::string& name, AST_Type* field_type,
                            const std::string&, bool, Intro&,
                            const std::string&)
  {
    std::stringstream ss;

    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      ss <<
        "    return lhs." << name << "() == rhs." << name << "();\n";
    } else if (cls & CL_STRING) {
      ss <<
        "    return std::strcmp (lhs." << name << "(), rhs." << name << "()) == 0 ;\n";
    } else if (cls & CL_ARRAY) {
      std::string indent("  ");
      NestedForLoops nfl("int", "i",
                         dynamic_cast<AST_Array*>(field_type), indent, true);
      ss <<
        indent << "if (lhs." << name << nfl.index_ << " != rhs."
               << name << nfl.index_ << ") {\n" <<
        indent << "  return false;\n" <<
        indent << "}\n";
      ss <<
        "    return true;\n";
    } else if (cls & CL_SEQUENCE) {
      ss <<
        "    return OpenDDS::DCPS::sequence_equal(lhs." << name << "(), rhs." << name << "());\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_FIXED)) {
      ss <<
        "    return lhs." << name << "() == rhs." << name << "();\n";
    } else {
      idl_global->err()->misc_warning("Unsupported type for union element", field_type);
    }

    return ss.str();
  }
}

bool ts_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator, const char*)
{
  if (be_global->generate_equality() && be_global->language_mapping() == BE_GlobalData::LANGMAP_NONE) {
    // == and != are generated as class members in other language mappings.

    const char* const nm = name->last_component()->get_string();

    ScopedNamespaceGuard hGuard(name, be_global->header_);
    ScopedNamespaceGuard cppGuard(name, be_global->impl_);

    be_global->header_
      << be_global->export_macro() << " bool operator==(const " << nm << "&, const " << nm << "&);\n"
      << be_global->export_macro() << " bool operator!=(const " << nm << "&, const " << nm << "&);\n";

    be_global->impl_ << "bool operator==(const " << nm << "& lhs, const " << nm << "& rhs)\n"
                      << "{\n"
                      << "  if (lhs._d() != rhs._d()) return false;\n";
    if (generateSwitchForUnion(node, "lhs._d()", generateEqual, branches, discriminator, "", "", "", false, false)) {
      be_global->impl_ <<
        "  return false;\n";
    }
    be_global->impl_ << "}\n";
    be_global->impl_ << "bool operator!=(const " << nm << "& lhs, const " << nm << "& rhs) { return !(lhs == rhs); }\n";
  }

  return generate_ts(node, name);
}

namespace java_ts_generator {

  /// called directly by dds_visitor::visit_structure() if -Wb,java
  void generate(AST_Structure* node) {
    UTL_ScopedName* name = node->name();

    if (!(idl_global->is_dcps_type(name) || be_global->is_topic_type(node))) {
      return;
    }

    ACE_CString output_file = be_global->java_arg();
    if (output_file.length()) {
      be_global->impl_name_ = output_file;
    }
    be_global->add_include("idl2jni_jni.h", BE_GlobalData::STREAM_CPP);

    std::string type = scoped(name);

    std::string file, jniclass, jpackage;
    for (UTL_ScopedName* sn = name; sn;
        sn = static_cast<UTL_ScopedName*>(sn->tail())) {
      std::string tmp = sn->head()->get_string();
      if (!tmp.empty() && sn->tail()) {
        jpackage += tmp;
        file += tmp;
        if (ACE_OS::mkdir(file.c_str()) != 0 && errno != EEXIST) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: java_ts_generator::generate - ")
            ACE_TEXT("unable to create specified directory: %C"), file.c_str()));
        }
      }
      for (size_t i = tmp.find('_'); i < tmp.length();
          i = tmp.find('_', i + 1)) {
        tmp.insert(++i, 1, '1');
      }
      jniclass += tmp;
      if (!jniclass.empty() && sn->tail()) {
        jniclass += '_';
        jpackage += '.';
        file += '/';
      }
    }

    if (jpackage.size() && jpackage[jpackage.size() - 1] == '.') {
      jpackage.resize(jpackage.size() - 1);
    }

    std::string clazz = name->last_component()->get_string();
    file += clazz + "TypeSupportImpl.java";

    std::ofstream java(file.c_str());
    java << (jpackage.size() ? "package " : "") << jpackage
      << (jpackage.size() ? ";\n" :"") <<
      "public class " << clazz << "TypeSupportImpl extends _" << clazz
      << "TypeSupportTAOPeer {\n"
      "    public " << clazz << "TypeSupportImpl() {\n"
      "        super(_jni_init());\n"
      "    }\n"
      "    private static native long _jni_init();\n"
      "}\n";
    be_global->impl_ <<
      "extern \"C\" JNIEXPORT jlong JNICALL\n"
      "Java_" << jniclass << "TypeSupportImpl__1jni_1init(JNIEnv*, jclass) {\n"
      "  return reinterpret_cast<jlong>(static_cast<CORBA::Object_ptr>(new "
      << type << "TypeSupportImpl));\n"
      "}\n\n";
  }

}

namespace face_ts_generator {

  void generate(UTL_ScopedName* name) {
    const std::string cxx_name = scoped(name),
      name_underscores = dds_generator::scoped_helper(name, "_"),
      exportMacro = be_global->export_macro().c_str(),
      exporter = exportMacro.empty() ? "" : ("    " + exportMacro + '\n');
    be_global->add_include("FACE/TS.hpp", BE_GlobalData::STREAM_FACETS_H);
    be_global->facets_header_ <<
      "namespace FACE\n"
      "{\n"
      "  namespace Read_Callback\n"
      "  {\n"
      "    typedef void (*send_event_" << name_underscores << "_Ptr) (\n"
      "      /* in */ TRANSACTION_ID_TYPE transaction_id,\n"
      "      /* inout */ " << cxx_name << "& message,\n"
      "      /* in */ MESSAGE_TYPE_GUID message_type_id,\n"
      "      /* in */ MESSAGE_SIZE_TYPE message_size,\n"
      "      /* in */ const WAITSET_TYPE waitset,\n"
      "      /* out */ RETURN_CODE_TYPE& return_code);\n"
      "  }\n\n"
      "  namespace TS\n"
      "  {\n" << exporter <<
      "    void Receive_Message(\n"
      "      /* in */ CONNECTION_ID_TYPE connection_id,\n"
      "      /* in */ TIMEOUT_TYPE timeout,\n"
      "      /* inout */ TRANSACTION_ID_TYPE& transaction_id,\n"
      "      /* out */ " << cxx_name << "& message,\n"
      "      /* in */ MESSAGE_SIZE_TYPE message_size,\n"
      "      /* out */ RETURN_CODE_TYPE& return_code);\n\n" << exporter <<
      "    void Send_Message(\n"
      "      /* in */ CONNECTION_ID_TYPE connection_id,\n"
      "      /* in */ TIMEOUT_TYPE timeout,\n"
      "      /* inout */ TRANSACTION_ID_TYPE& transaction_id,\n"
      "      /* inout */ " << cxx_name << "& message,\n"
      "      /* inout */ MESSAGE_SIZE_TYPE& message_size,\n"
      "      /* out */ RETURN_CODE_TYPE& return_code);\n\n" << exporter <<
      "    void Register_Callback(\n"
      "      /* in */ CONNECTION_ID_TYPE connection_id,\n"
      "      /* in */ const WAITSET_TYPE waitset,\n"
      "      /* in */ Read_Callback::send_event_" << name_underscores
                    << "_Ptr data_callback,\n"
      "      /* in */ MESSAGE_SIZE_TYPE max_message_size,\n"
      "      /* out */ RETURN_CODE_TYPE& return_code);\n\n"
      "  }\n"
      "}\n\n";
    be_global->facets_impl_ <<
      "void Receive_Message(CONNECTION_ID_TYPE connection_id,\n"
      "                     TIMEOUT_TYPE timeout,\n"
      "                     TRANSACTION_ID_TYPE& transaction_id,\n"
      "                     " << cxx_name << "& message,\n"
      "                     MESSAGE_SIZE_TYPE message_size,\n"
      "                     RETURN_CODE_TYPE& return_code) {\n"
      "  OpenDDS::FaceTSS::receive_message(connection_id, timeout,\n"
      "                                    transaction_id, message,\n"
      "                                    message_size, return_code);\n"
      "}\n\n"
      "void Send_Message(CONNECTION_ID_TYPE connection_id,\n"
      "                  TIMEOUT_TYPE timeout,\n"
      "                  TRANSACTION_ID_TYPE& transaction_id,\n"
      "                  " << cxx_name << "& message,\n"
      "                  MESSAGE_SIZE_TYPE& message_size,\n"
      "                  RETURN_CODE_TYPE& return_code) {\n"
      "  OpenDDS::FaceTSS::send_message(connection_id, timeout,\n"
      "                                 transaction_id, message,\n"
      "                                 message_size, return_code);\n"
      "}\n\n"
      "void Register_Callback(CONNECTION_ID_TYPE connection_id,\n"
      "                       const WAITSET_TYPE waitset,\n"
      "                       Read_Callback::send_event_" << name_underscores
                          << "_Ptr data_callback,\n"
      "                       MESSAGE_SIZE_TYPE max_message_size,\n"
      "                       RETURN_CODE_TYPE& return_code) {\n"
      "  OpenDDS::FaceTSS::register_callback(connection_id, waitset,\n"
      "                                      data_callback,\n"
      "                                      max_message_size, return_code);\n"
      "}\n\n"
      "namespace {\n"
      "  OpenDDS::DCPS::TypeSupportInitializer<" << cxx_name << "TypeSupportImpl> ts_init_" << name_underscores << ";\n"
      "}\n\n";
  }
}
