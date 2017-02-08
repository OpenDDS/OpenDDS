/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ts_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include "ace/OS_NS_sys_stat.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <map>
#include <iostream>

namespace {
  std::string read_template(const char* prefix)
  {
    const char* dds_root = ACE_OS::getenv("DDS_ROOT");
    if (!dds_root) {
      ACE_DEBUG((LM_ERROR, "The environment variable DDS_ROOT must be set.\n"));
      BE_abort();
    }
    std::string path = dds_root;
    path.append("/dds/idl/");
    path.append(prefix);
    path.append("Template.txt");
    std::ifstream ifs(path.c_str());
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

bool ts_generator::gen_struct(AST_Structure*, UTL_ScopedName* name,
  const std::vector<AST_Field*>&, AST_Type::SIZE_TYPE, const char*)
{
  IDL_GlobalData::DCPS_Data_Type_Info* info = idl_global->is_dcps_type(name);

  if (!info) {
    // no #pragma DCPS_DATA_TYPE, so nothing to generate
    return true;
  }

  const std::string cxxName = scoped(name);
  const std::string short_name = name->last_component()->get_string();

  static const char* idl_includes[] = {
    "dds/DdsDcpsInfrastructure.idl", "dds/DdsDcpsTopic.idl",
    "dds/DdsDcpsPublication.idl", "dds/DdsDcpsSubscriptionExt.idl",
    "dds/DdsDcpsTypeSupportExt.idl"
  };
  add_includes(idl_includes, BE_GlobalData::STREAM_IDL);

  std::string dc = be_global->header_name_.c_str();
  dc.replace(dc.end() - 6, dc.end() - 2, "C"); // s/Impl.h$/C.h/
  be_global->add_include(dc.c_str());

  static const char* h_includes[] = {
    "dds/DCPS/TypeSupportImpl.h"
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
    "dds/DCPS/DataReaderImpl_T.h"
  };
  add_includes(cpp_includes, BE_GlobalData::STREAM_CPP);

  std::map<std::string, std::string> replacements;
  replacements["SCOPED"] = cxxName;
  replacements["TYPE"] = short_name;
  replacements["EXPORT"] = be_global->export_macro().c_str();
  replacements["SEQ"] = be_global->sequence_suffix().c_str();

  ScopedNamespaceGuard idlGuard(name, be_global->idl_, "module");
  std::string idl = idl_template_;
  replaceAll(idl, replacements);
  be_global->idl_ << idl;

  be_global->header_ << be_global->versioning_begin() << "\n";
  {
    ScopedNamespaceGuard hGuard(name, be_global->header_);

    be_global->header_ <<
      "class " << short_name << "TypeSupportImpl;\n";
  }
  be_global->header_ << be_global->versioning_end() << "\n";

  const bool has_keys = info->key_list_.is_empty();

  be_global->header_ <<
    "OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL\n"
    "namespace OpenDDS { namespace DCPS {\n"
    "template <>\n"
    "struct DDSTraits<" << cxxName << "> {\n"
    "  typedef " << cxxName << " MessageType;\n"
    "  typedef " << cxxName << "Seq MessageSequenceType;\n"
    "  typedef " << cxxName << "TypeSupport TypeSupportType;\n"
    "  typedef " << cxxName << "TypeSupportImpl TypeSupportTypeImpl;\n"
    "  typedef " << cxxName << "DataWriter DataWriterType;\n"
    "  typedef " << cxxName << "DataReader DataReaderType;\n"
    "  typedef " << cxxName << "_OpenDDS_KeyLessThan LessThanType;\n"
    "\n"
    "  static const char* type_name () { return \"" << cxxName << "\"; }\n"
    "  static bool gen_has_key () { return " << (has_keys ? "false" : "true") << "; }\n"
    "\n"
    "  static size_t gen_max_marshaled_size(const MessageType& x, bool align) { return ::OpenDDS::DCPS::gen_max_marshaled_size(x, align); }\n"
    "  static void gen_find_size(const MessageType& arr, size_t& size, size_t& padding) { ::OpenDDS::DCPS::gen_find_size(arr, size, padding); }\n"
    "\n"
    "  static size_t gen_max_marshaled_size(const OpenDDS::DCPS::KeyOnly<const MessageType>& x, bool align) { return ::OpenDDS::DCPS::gen_max_marshaled_size(x, align); }\n"
    "  static void gen_find_size(const OpenDDS::DCPS::KeyOnly<const MessageType>& arr, size_t& size, size_t& padding) { ::OpenDDS::DCPS::gen_find_size(arr, size, padding); }\n"
    "};\n}  }\nOPENDDS_END_VERSIONED_NAMESPACE_DECL\n\n";

  be_global->header_ << be_global->versioning_begin() << "\n";
  {
    ScopedNamespaceGuard hGuard(name, be_global->header_);

    be_global->header_ <<
      "class " << be_global->export_macro() << " " << short_name << "TypeSupportImpl\n"
      "  : public virtual OpenDDS::DCPS::LocalObject<" << short_name << "TypeSupport>\n"
      "  , public virtual OpenDDS::DCPS::TypeSupportImpl\n"
      "{\n"
      "public:\n"
      "  typedef OpenDDS::DCPS::DDSTraits<" << short_name << "> TraitsType;\n"
      "  typedef " << short_name << "TypeSupport TypeSupportType;\n"
      "  typedef " << short_name << "TypeSupport::_var_type _var_type;\n"
      "  typedef " << short_name << "TypeSupport::_ptr_type _ptr_type;\n"
      "\n"
      "  " << short_name << "TypeSupportImpl() {}\n"
      "  virtual ~" << short_name << "TypeSupportImpl() {}\n"
      "\n"
      "  virtual " << be_global->versioning_name() << "::DDS::DataWriter_ptr create_datawriter();\n"
      "  virtual " << be_global->versioning_name() << "::DDS::DataReader_ptr create_datareader();\n"
      "#ifndef OPENDDS_NO_MULTI_TOPIC\n"
      "  virtual " << be_global->versioning_name() << "::DDS::DataReader_ptr create_multitopic_datareader();\n"
      "#endif /* !OPENDDS_NO_MULTI_TOPIC */\n"
      "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n"
      "  virtual const OpenDDS::DCPS::MetaStruct& getMetaStructForType();\n"
      "#endif /* !OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE */\n"
      "  virtual bool has_dcps_key();\n"
      "  const char* default_type_name() const;\n"
      "  static " << short_name << "TypeSupport::_ptr_type _narrow(CORBA::Object_ptr obj);\n"
      "};\n";
  }
  be_global->header_ << be_global->versioning_end() << "\n";

  be_global->impl_ << be_global->versioning_begin() << "\n";
  {
    ScopedNamespaceGuard cppGuard(name, be_global->impl_);
    be_global->impl_ <<
      "::DDS::DataWriter_ptr " << short_name << "TypeSupportImpl::create_datawriter()\n"
      "{\n"
      "  typedef OpenDDS::DCPS::DataWriterImpl_T<" << short_name << "> DataWriterImplType;\n"
      "  ::DDS::DataWriter_ptr writer_impl = ::DDS::DataWriter::_nil();\n"
      "  ACE_NEW_NORETURN(writer_impl,\n"
      "                   DataWriterImplType());\n"
      "  return writer_impl;\n"
      "}\n"
      "::DDS::DataReader_ptr " << short_name << "TypeSupportImpl::create_datareader()\n"
      "{\n"
      "  typedef OpenDDS::DCPS::DataReaderImpl_T<" << short_name << "> DataReaderImplType;\n"
      "  ::DDS::DataReader_ptr reader_impl = ::DDS::DataReader::_nil();\n"
      "  ACE_NEW_NORETURN(reader_impl,\n"
      "                   DataReaderImplType());\n"
      "  return reader_impl;\n"
      "}\n"
      "#ifndef OPENDDS_NO_MULTI_TOPIC\n"
      "::DDS::DataReader_ptr " << short_name << "TypeSupportImpl::create_multitopic_datareader()\n"
      "{\n"
      "  typedef OpenDDS::DCPS::DataReaderImpl_T<" << short_name << "> DataReaderImplType;\n"
      "  typedef OpenDDS::DCPS::MultiTopicDataReader_T<" << short_name << ", DataReaderImplType> MultiTopicDataReaderImplType;\n"
      "  ::DDS::DataReader_ptr multitopic_reader_impl = ::DDS::DataReader::_nil();\n"
      "  ACE_NEW_NORETURN(multitopic_reader_impl,\n"
      "                   MultiTopicDataReaderImplType());\n"
      "  return multitopic_reader_impl;\n"
      "}\n"
      "#endif /* !OPENDDS_NO_MULTI_TOPIC */\n"
      "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n"
      "const OpenDDS::DCPS::MetaStruct& " << short_name << "TypeSupportImpl::getMetaStructForType()\n"
      "{\n"
      "  return OpenDDS::DCPS::getMetaStruct<" << short_name << ">();\n"
      "}\n"
      "#endif /* !OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE */\n"
      "bool " << short_name << "TypeSupportImpl::has_dcps_key()\n"
      "{\n"
      "  return TraitsType::gen_has_key ();\n"
      "}\n"
      "const char* " << short_name << "TypeSupportImpl::default_type_name() const\n"
      "{\n"
      "  return TraitsType::type_name();\n"
      "}\n"
      << short_name << "TypeSupport::_ptr_type " << short_name << "TypeSupportImpl::_narrow(CORBA::Object_ptr obj)\n"
      "{\n"
      "  return TypeSupportType::_narrow(obj);\n"
      "}\n";
  }
  be_global->impl_ << be_global->versioning_end() << "\n";

  if (be_global->face_ts()) {
    face_ts_generator::generate(name);
  }

  return true;
}

bool ts_generator::gen_union(AST_Union*, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>&, AST_Type*, const char*)
{
  if (idl_global->is_dcps_type(name)) {
    std::cerr << "ERROR: union " << scoped(name) << " can not be used as a "
      "DCPS_DATA_TYPE (only structs can be Topic types)" << std::endl;
    return false;
  }
  return true;
}

namespace java_ts_generator {

  /// called directly by dds_visitor::visit_structure() if -Wb,java
  void generate(UTL_ScopedName* name) {
    if (idl_global->is_dcps_type(name) == 0) {
      // no #pragma DCPS_DATA_TYPE, so nothing to generate
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
      if (tmp != "" && sn->tail()) {
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
      if (jniclass != "" && sn->tail()) {
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
    const std::string name_cxx = scoped(name),
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
      "      /* inout */ " << name_cxx << "& message,\n"
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
      "      /* out */ " << name_cxx << "& message,\n"
      "      /* in */ MESSAGE_SIZE_TYPE message_size,\n"
      "      /* out */ RETURN_CODE_TYPE& return_code);\n\n" << exporter <<
      "    void Send_Message(\n"
      "      /* in */ CONNECTION_ID_TYPE connection_id,\n"
      "      /* in */ TIMEOUT_TYPE timeout,\n"
      "      /* inout */ TRANSACTION_ID_TYPE& transaction_id,\n"
      "      /* inout */ " << name_cxx << "& message,\n"
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
      "                     " << name_cxx << "& message,\n"
      "                     MESSAGE_SIZE_TYPE message_size,\n"
      "                     RETURN_CODE_TYPE& return_code) {\n"
      "  OpenDDS::FaceTSS::receive_message(connection_id, timeout,\n"
      "                                    transaction_id, message,\n"
      "                                    message_size, return_code);\n"
      "}\n\n"
      "void Send_Message(CONNECTION_ID_TYPE connection_id,\n"
      "                  TIMEOUT_TYPE timeout,\n"
      "                  TRANSACTION_ID_TYPE& transaction_id,\n"
      "                  " << name_cxx << "& message,\n"
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
      "struct " << name_underscores << "_Initializer {\n"
      "  " << name_underscores << "_Initializer()\n"
      "  {\n"
      "    " << name_cxx << "TypeSupport_var ts = new " << name_cxx
                          << "TypeSupportImpl;\n"
      "    ts->register_type(0, \"\");\n"
      "  }\n"
      "} init_" << name_underscores << ";\n\n";
  }
}
