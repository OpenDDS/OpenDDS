/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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

  struct TS_NamespaceGuard  {
    TS_NamespaceGuard(UTL_ScopedName* name, std::ostream& os,
      const char* keyword = "namespace")
      : os_(os)
    {
      for (n_ = 0; name->tail();
          name = static_cast<UTL_ScopedName*>(name->tail())) {
        const char* str = name->head()->get_string();
        if (str && str[0]) {
          ++n_;
          os << keyword << (name->head()->escaped() ? " _" : " ")
            << str << " {\n";
        }
      }
      if (std::strcmp(keyword, "module") == 0) semi_ = ";";
    }
    ~TS_NamespaceGuard()
    {
      for (int i = 0; i < n_; ++i) os_ << '}' << semi_ << '\n';
    }
    std::ostream& os_;
    std::string semi_;
    int n_;
  };

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
    "dds/DCPS/transport/framework/TransportInterface.h",
    "dds/DCPS/BuiltInTopicUtils.h", "dds/DCPS/Util.h",
    "dds/DCPS/ContentFilteredTopicImpl.h"
  };
  add_includes(cpp_includes, BE_GlobalData::STREAM_CPP);

  std::map<std::string, std::string> replacements;
  replacements["SCOPED"] = scoped(name);
  replacements["TYPE"] = name->last_component()->get_string();
  replacements["EXPORT"] = be_global->export_macro().c_str();

  TS_NamespaceGuard idlGuard(name, be_global->idl_, "module");
  std::string idl = idl_template_;
  replaceAll(idl, replacements);
  be_global->idl_ << idl;

  TS_NamespaceGuard hGuard(name, be_global->header_);
  std::string h = h_template_;
  replaceAll(h, replacements);
  be_global->header_ << h;

  TS_NamespaceGuard cppGuard(name, be_global->impl_);
  std::string cpp = cpp_template_;
  replaceAll(cpp, replacements);
  be_global->impl_ << cpp;
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
      "Java_" << jniclass << "TypeSupportImpl__1_jni_1init(JNIEnv*, jclass) {\n"
      "  return reinterpret_cast<jlong>(static_cast<CORBA::Object_ptr>(new "
      << type << "TypeSupportImpl));\n"
      "}\n\n";
  }

}
