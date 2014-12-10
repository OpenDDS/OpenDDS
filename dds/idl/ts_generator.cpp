/*
 * $Id$
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
  , h_template_(read_template("H"))
  , cpp_template_(read_template("CPP"))
{
}

bool ts_generator::gen_struct(UTL_ScopedName* name,
  const std::vector<AST_Field*>&, const char*)
{
  if (idl_global->is_dcps_type(name) == 0) {
    // no #pragma DCPS_DATA_TYPE, so nothing to generate
    return true;
  }

  const std::string cxxName = scoped(name);

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
    "dds/DCPS/DataWriterImpl.h", "dds/DCPS/DataReaderImpl.h",
    "dds/DCPS/TypeSupportImpl.h",
    "dds/DCPS/Dynamic_Cached_Allocator_With_Overflow_T.h",
    "dds/DCPS/DataBlockLockPool.h", "dds/DCPS/SubscriptionInstance.h"
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
    "dds/DCPS/MultiTopicDataReader_T.h"
  };
  add_includes(cpp_includes, BE_GlobalData::STREAM_CPP);

  std::map<std::string, std::string> replacements;
  replacements["SCOPED"] = cxxName;
  replacements["TYPE"] = name->last_component()->get_string();
  replacements["EXPORT"] = be_global->export_macro().c_str();
  replacements["SEQ"] = be_global->sequence_suffix().c_str();

  TS_NamespaceGuard idlGuard(name, be_global->idl_, "module");
  std::string idl = idl_template_;
  replaceAll(idl, replacements);
  be_global->idl_ << idl;

  {
    TS_NamespaceGuard hGuard(name, be_global->header_);
    std::string h = h_template_;
    replaceAll(h, replacements);
    be_global->header_ << h;
  }

  be_global->header_ <<
    "namespace OpenDDS { namespace DCPS {\n"
    "template <>\n"
    "struct DDSTraits<" << cxxName << "> {\n"
    "  typedef " << cxxName << "DataWriter DataWriter;\n"
    "  typedef " << cxxName << "DataReader DataReader;\n"
    "  typedef " << cxxName << "TypeSupport TypeSupport;\n"
    "  typedef " << cxxName << "TypeSupportImpl TypeSupportImpl;\n"
    "  typedef " << cxxName << "Seq Sequence;\n"
    "};\n}  }\n\n";

  TS_NamespaceGuard cppGuard(name, be_global->impl_);
  std::string cpp = cpp_template_;
  replaceAll(cpp, replacements);
  be_global->impl_ << cpp;

  if (be_global->face()) {
    face_ts_generator::generate(name, dc.c_str());
  }

  return true;
}

bool ts_generator::gen_union(UTL_ScopedName* name,
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
        ACE_OS::mkdir(file.c_str());
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

    if (jpackage[jpackage.size() - 1] == '.') {
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

  void generate(UTL_ScopedName* name, std::string typeSuppHeader) {
    const std::string name_cxx = scoped(name),
      name_underscores = dds_generator::scoped_helper(name, "_"),
      dataTypeHeader = typeSuppHeader.erase(typeSuppHeader.size() - 14, 11),
      exportMacro = be_global->export_macro().c_str(),
      exporter = exportMacro.empty() ? "" : ("    " + exportMacro + '\n');
    be_global->add_include("FACE/TS.hpp", BE_GlobalData::STREAM_FACE_H);
    be_global->face_header_ <<
      "#include \"" << dataTypeHeader << "\"\n\n"
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
    be_global->face_impl_ <<
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
