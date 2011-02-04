/*
 * $Id$
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

#ifndef ACE_AS_STATIC_LIBS
extern "C"
dissector_Export const gchar version[] = DDS_VERSION;

extern "C"
dissector_Export void
plugin_register()
{
  extern void proto_register_opendds();
  proto_register_opendds();
}

extern "C"
dissector_Export void
plugin_reg_handoff()
{
  extern void proto_reg_handoff_opendds();
  proto_reg_handoff_opendds();
}
#endif  /* ACE_AS_STATIC_LIBS */
