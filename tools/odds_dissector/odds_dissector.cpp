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

#include <epan/ipproto.h>
#include <epan/packet.h>

#include <glib.h>

} // extern "C"

#include <ace/Message_Block.h>
#include <ace/OS_NS_string.h>

#include <dds/DCPS/transport/framework/TransportHeader.h>

#include <algorithm>

static int proto_odds = -1;

static int hf_odds_version    = -1;
static int hf_odds_byte_order = -1;
static int hf_odds_length     = -1;
static int hf_odds_sequence   = -1;
static int hf_odds_source     = -1;

static const value_string odds_byte_order_vals[] = {
  { 0x0,  "Big Endian"    },
  { 0x1,  "Little Endian" },
  { 0,    NULL            }
};

static gint ett_odds = -1;

OpenDDS::DCPS::TransportHeader
dissect_odds_header(tvbuff_t* tvb, gint offset)
{
  OpenDDS::DCPS::TransportHeader header;

  size_t len = std::min(tvb->length, header.max_marshaled_size());
  const guint8* data = tvb_get_ptr(tvb, offset, len);

  ACE_Message_Block mb(reinterpret_cast<const char*>(data));
  mb.wr_ptr(len);

  header.init(&mb); // demarshal

  return header;
}

extern "C" void
dissect_odds(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree)
{
  OpenDDS::DCPS::TransportHeader header(dissect_odds_header(tvb, 0));

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "OpenDDS");
  col_clear(pinfo->cinfo,COL_INFO);

  if (tree != NULL) {
    proto_item* item = NULL;
    proto_tree* odds_tree = NULL;

    item = proto_tree_add_item(tree, proto_odds, tvb, 0, -1, FALSE);
    odds_tree = proto_item_add_subtree(item, ett_odds);

    size_t len = 0;
    size_t offset = 0;

    // Skip protocol preamble (DCPS):
    offset += sizeof(header.protocol_) - 2;

    // hf_odds_version
    len = sizeof(header.protocol_) - offset;
    proto_tree_add_protocol_format(odds_tree, hf_odds_version, tvb, offset, len,
      "Version: %d.%d", header.protocol_[4], header.protocol_[5]);
    offset += len;

    // hf_odds_byte_order
    len = sizeof(header.byte_order_);
    proto_tree_add_item(odds_tree, hf_odds_byte_order, tvb, offset, len, FALSE);
    offset += len;

    // Skip reserved octets:
    offset += sizeof(header.reserved_);

    // hf_odds_length
    len = sizeof(header.length_);
    proto_tree_add_uint_format_value(odds_tree, hf_odds_length, tvb, offset, len,
      header.length_, "%d octets", header.length_);
    offset += len;

    // hf_odds_sequence
    len = sizeof(header.sequence_);
    proto_tree_add_uint(odds_tree, hf_odds_sequence, tvb, offset, len,
      guint16(header.sequence_));
    offset += len;

    // hf_odds_sequence
    len = sizeof(header.source_);
    proto_tree_add_uint(odds_tree, hf_odds_source, tvb, offset, len,
      guint32(header.source_));
    offset += len;
  }
}

extern "C" gboolean
dissect_odds_heur(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree)
{
  size_t len = sizeof(OpenDDS::DCPS::TransportHeader::DCPS_PROTOCOL);
  guint8* data = tvb_get_ephemeral_string(tvb, 0, len);

  // OpenDDS only supports the current DCPS protocol revision; if
  // this changes, we should do something about it here.
  if (ACE_OS::memcmp(data, OpenDDS::DCPS::TransportHeader::DCPS_PROTOCOL, len) == 0) {
    dissect_odds(tvb, pinfo, tree);
    return TRUE;
  }

  return FALSE;
}

extern "C" void
proto_register_odds()
{
  static hf_register_info hf[] = {
    { &hf_odds_version,
      { "Version",
        "odds.version",
        FT_PROTOCOL,
        HFILL
      },
    },

    { &hf_odds_byte_order,
      { "Byte order",
        "odds.byte_order",
        FT_UINT8,
        BASE_HEX,
        VALS(odds_byte_order_vals),
        HFILL
      },
    },

    { &hf_odds_length,
      { "Length",
        "odds.length",
        FT_UINT16,
        BASE_HEX,
        HFILL
      },
    },

    { &hf_odds_sequence,
      { "Sequence",
        "odds.sequence",
        FT_UINT16,
        BASE_HEX,
        HFILL
      },
    },

    { &hf_odds_source,
      { "Source",
        "odds.source",
        FT_UINT32,
        BASE_HEX,
        HFILL
      },
    }
  };

  static gint *ett[] = {
    &ett_odds
  };

  proto_odds = proto_register_protocol(
    "OpenDDS DCPS Protocol",  // name
    "OpenDDS",                // short_name
    "odds");                  // filter_name

  proto_register_field_array(proto_odds, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

extern "C" void
proto_reg_handoff_odds()
{
  static dissector_handle_t odds_handle;
  odds_handle = create_dissector_handle(dissect_odds, proto_odds);

  // OpenDDS supports TCP/IP, UDP/IP, and IP multicast:
  heur_dissector_add("tcp", dissect_odds_heur, proto_odds);
  heur_dissector_add("udp", dissect_odds_heur, proto_odds);
}
