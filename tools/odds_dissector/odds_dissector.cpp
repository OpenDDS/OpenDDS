/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "config.h"

#include <epan/packet.h>

extern "C" void
dissect_odds(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree)
{
}

//

static int proto_odds = -1;

extern "C" void
proto_register_odds()
{
  proto_odds = proto_register_protocol(
    "OpenDDS DCPS Protocol",  // name
    "OpenDDS",                // short_name
    "odds");                  // filter_name
}

extern "C" void
proto_reg_handoff_odds()
{
  static dissector_handle_t odds_handle;

  odds_handle = create_dissector_handle(dissect_odds, proto_odds);
}
