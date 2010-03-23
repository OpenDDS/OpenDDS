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

#include "odds_export.h"

#ifndef ACE_AS_STATIC_LIBS
extern "C"
odds_Export const gchar version[] = DDS_VERSION;

extern "C"
odds_Export void
plugin_register()
{
  extern void proto_register_odds();
  proto_register_odds();
}

extern "C"
odds_Export void
plugin_reg_handoff()
{
  extern void proto_reg_handoff_odds();
  proto_reg_handoff_odds();
}
#endif  /* ACE_AS_STATIC_LIBS */
