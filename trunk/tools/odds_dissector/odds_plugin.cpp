/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "odds_Export.h"

#ifndef ACE_AS_STATIC_LIBS
odds_Export extern "C"
const char version[] = "$Revision$";

odds_Export extern "C" void
plugin_register()
{
  extern void proto_register_odds();
  proto_register_odds();
}

odds_Export extern "C" void
plugin_reg_handoff()
{
  extern void proto_reg_handoff_odds();
  proto_reg_handoff_odds();
}
#endif  /* ACE_AS_STATIC_LIBS */
