/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

extern "C" {

#include "config.h"

#include <glib.h>
#include <gmodule.h>

} // extern "C"

#include <dds/Version.h>
#include "dissector_export.h"

#include "tools/dissector/packet-opendds.h"
#include "tools/dissector/packet-repo.h"
#include "tools/dissector/packet-datawriter.h"

#include <fstream>

#ifndef ACE_AS_STATIC_LIBS
extern "C"
dissector_Export const gchar version[] = DDS_VERSION;

extern "C"
dissector_Export void
plugin_register()
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
plugin_reg_handoff()
{
  OpenDDS::DCPS::DDS_Dissector::instance().register_handoff ();
  OpenDDS::DCPS::InfoRepo_Dissector::instance().register_handoff ();
  OpenDDS::DCPS::DataWriterRemote_Dissector::instance().register_handoff ();
}

#endif  /* ACE_AS_STATIC_LIBS */
