/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ws-wrapper-headers/config.h"
#include "dissector_export.h"
#include "packet-opendds.h"
#include "packet-repo.h"
#include "packet-datawriter.h"

#include <dds/Version.h>

#include <glib.h>
#include <gmodule.h>

#include <fstream>

#ifndef ACE_AS_STATIC_LIBS

/*
 * In Wireshark 2.5 (the future version 2.6), the method of reporting plugins
 * to Wireshark changed.
 */
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 5, 0)
  extern "C" dissector_Export const gchar plugin_version[] = OPENDDS_VERSION;

#  if WIRESHARK_VERSION < WIRESHARK_VERSION_NUMBER(3, 0, 0)
  extern "C" dissector_Export const gchar plugin_release[] = VERSION_RELEASE;
#  else
  extern "C" dissector_Export const int plugin_want_major = VERSION_MAJOR;
  extern "C" dissector_Export const int plugin_want_minor = VERSION_MINOR;
#  endif
#else
  extern "C" dissector_Export const gchar version[] = OPENDDS_VERSION;
#endif

extern "C"
dissector_Export void
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 5, 0)
register_opendds()
#else
plugin_register()
#endif
{
  if (ACE_OS::getenv("OPENDDS_DISSECTOR_LOG")) {
    ACE_LOG_MSG->msg_ostream(new std::ofstream("OpenDDS_wireshark.log"), 1);
    ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
  }
  OpenDDS::DCPS::DDS_Dissector::instance().init ();
  OpenDDS::DCPS::InfoRepo_Dissector::instance().init ();
  OpenDDS::DCPS::DataWriterRemote_Dissector::instance().init ();
}

extern "C"
dissector_Export void
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 5, 0)
reg_handoff_opendds()
#else
plugin_reg_handoff()
#endif
{
  OpenDDS::DCPS::DDS_Dissector::instance().register_handoff ();
  OpenDDS::DCPS::InfoRepo_Dissector::instance().register_handoff ();
  OpenDDS::DCPS::DataWriterRemote_Dissector::instance().register_handoff ();
}

#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 5, 0)
extern "C" dissector_Export void plugin_register()
{
  static proto_plugin opendds_plugin;
  opendds_plugin.register_protoinfo = register_opendds;
  opendds_plugin.register_handoff = reg_handoff_opendds;
  proto_register_plugin(&opendds_plugin);
}
#endif

#endif  /* ACE_AS_STATIC_LIBS */
