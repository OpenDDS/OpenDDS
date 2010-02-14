/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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

#ifndef ENABLE_STATIC
G_MODULE_EXPORT extern "C"
const gchar version[] = DDS_VERSION;

G_MODULE_EXPORT extern "C" void
plugin_register()
{
  extern void proto_register_odds();
  proto_register_odds();
}

G_MODULE_EXPORT extern "C" void
plugin_reg_handoff()
{
  extern void proto_reg_handoff_odds();
  proto_reg_handoff_odds();
}
#endif  /* ENABLE_STATIC */
