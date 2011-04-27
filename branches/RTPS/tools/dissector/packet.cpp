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

#include <epan/value_string.h>
#include <epan/ipproto.h>
#include <epan/packet.h>

} // extern "C"

#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/Message_Block.h>

#include <dds/DCPS/DataSampleHeader.h>
#include <dds/DCPS/RepoIdConverter.h>
#include <dds/DdsDcpsGuidTypeSupportImpl.h>
#include <dds/DCPS/transport/framework/TransportHeader.h>

#include <cstring>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include "dissector_export.h"

using namespace OpenDDS::DCPS;

// value ABSOLUTE_TIME_LOCAL, 1.2.x does not.  This technique uses
// the ABSOLUTE_TIME_LOCAL value if it is present (1.3.x),
// and uses BASE_NONE if it is not (1.2.x).  This must be in
// the same scope as the Wireshark 1.2.x declaration of
// the ABSOLUTE_TIME_LOCAL enum value, which is why it is in the
// global namespace.
struct ABSOLUTE_TIME_LOCAL {
  static const int value = BASE_NONE;
};

namespace {

// These two functions are the rest of the
// Wireshark 1.2.x / 1.3.x compatibility solution.
template <int V> int enum_value() { return V; }
template <typename T> int enum_value() { return T::value; }

int proto_opendds    = -1;

int hf_version    = -1;
int hf_length     = -1;
int hf_sequence   = -1;
int hf_source     = -1;

int hf_flags               = -1;
int hf_flags_byte_order    = -1;
int hf_flags_first_fragment= -1;
int hf_flags_last_fragment = -1;

const int flags_bits = 8;
const int* flags_fields[] = {
  &hf_flags_byte_order,
  &hf_flags_first_fragment,
  &hf_flags_last_fragment,
  NULL
};

int hf_sample             = -1;
int hf_sample_id          = -1;
int hf_sample_sub_id      = -1;
int hf_sample_length      = -1;
int hf_sample_sequence    = -1;
int hf_sample_timestamp   = -1;
int hf_sample_lifespan    = -1;
int hf_sample_publication = -1;
int hf_sample_publisher   = -1;
int hf_sample_content_filt= -1;

int hf_sample_flags             = -1;
int hf_sample_flags_byte_order  = -1;
int hf_sample_flags_coherent    = -1;
int hf_sample_flags_historic    = -1;
int hf_sample_flags_lifespan    = -1;
int hf_sample_flags_group_coh   = -1;
int hf_sample_flags_content_filt= -1;
int hf_sample_flags_seq_repair  = -1;
int hf_sample_flags_more_frags  = -1;

const int sample_flags_bits = 8;

const int* sample_flags_fields[] = {
  &hf_sample_flags_byte_order,
  &hf_sample_flags_coherent,
  &hf_sample_flags_historic,
  &hf_sample_flags_lifespan,
  &hf_sample_flags_group_coh,
  &hf_sample_flags_content_filt,
  &hf_sample_flags_seq_repair,
  &hf_sample_flags_more_frags,
  NULL
};

gint ett_header       = -1;
gint ett_flags        = -1;
gint ett_sample       = -1;
gint ett_sample_flags = -1;

const value_string byte_order_vals[] = {
  { 0x0,  "Big Endian"    },
  { 0x1,  "Little Endian" },
  { 0,    NULL            }
};

const true_false_string byte_order_tfs = {
  "Little Endian",
  "Big Endian"
};

const value_string sample_id_vals[] = {
  { SAMPLE_DATA,            "SAMPLE_DATA"           },
  { DATAWRITER_LIVELINESS,  "DATAWRITER_LIVELINESS" },
  { INSTANCE_REGISTRATION,  "INSTANCE_REGISTRATION" },
  { UNREGISTER_INSTANCE,    "UNREGISTER_INSTANCE"   },
  { DISPOSE_INSTANCE,       "DISPOSE_INSTANCE"      },
  { GRACEFUL_DISCONNECT,    "GRACEFUL_DISCONNECT"   },
  { FULLY_ASSOCIATED,       "FULLY_ASSOCIATED"      },
  { REQUEST_ACK,            "REQUEST_ACK"           },
  { SAMPLE_ACK,             "SAMPLE_ACK"            },
  { END_COHERENT_CHANGES,   "END_COHERENT_CHANGES"  },
  { TRANSPORT_CONTROL,      "TRANSPORT_CONTROL"     },
  { 0,                      NULL                    }
};

const value_string sample_sub_id_vals[] = {
  { SUBMESSAGE_NONE,        "SUBMESSAGE_NONE"       },
  { MULTICAST_SYN,          "MULTICAST_SYN"         },
  { MULTICAST_SYNACK,       "MULTICAST_SYNACK"      },
  { MULTICAST_NAK,          "MULTICAST_NAK"         },
  { MULTICAST_NAKACK,       "MULTICAST_NAKACK"      },
  { 0,                      NULL                    }
};

template<typename T>
T
demarshal_data(tvbuff_t* tvb, gint offset)
{
  T t;

  guint len = std::min(tvb->length - offset,
    static_cast<guint>(t.max_marshaled_size()));
  const guint8* data = tvb_get_ptr(tvb, offset, len);

  ACE_Message_Block mb(reinterpret_cast<const char*>(data));
  mb.wr_ptr(len);

  t.init(&mb);

  return t;
}

std::string
format_header(const TransportHeader& header)
{
  std::ostringstream os;

  os << "Length: " << std::dec << header.length_;
  if (header.first_fragment_) os << ", First Fragment";
  if (header.last_fragment_) os << ", Last Fragment";
  os << ", Sequence: 0x" << std::hex << std::setw(8) << std::setfill('0')
     << header.sequence_.getValue();
  os << ", Source: 0x" << std::hex << std::setw(8) << std::setfill('0')
     << ACE_UINT32(header.source_);

  return os.str();
}

void
dissect_header(tvbuff* tvb, packet_info* pinfo, proto_tree* tree,
               const TransportHeader& header, gint& offset)
{
  ACE_UNUSED_ARG(pinfo);

  gint len;

  offset += sizeof(header.protocol_) - 2; // skip preamble

  // hf_version
  len = sizeof(header.protocol_) - offset;
  proto_tree_add_bytes_format_value(tree, hf_version, tvb, offset, len,
    reinterpret_cast<const guint8*>(header.protocol_ + offset),
    "%d.%d", header.protocol_[4], header.protocol_[5]);
  offset += len;

  // hf_flags
  len = sizeof(ACE_CDR::Octet);
  proto_tree_add_bitmask(tree, tvb, offset, hf_flags, ett_flags,
    flags_fields, FALSE);
  offset += len;

  offset += sizeof(header.reserved_);     // skip reserved

  // hf_length
  len = sizeof(header.length_);
  proto_tree_add_uint_format_value(tree, hf_length, tvb, offset, len,
    header.length_, "%d octets", header.length_);
  offset += len;

  // hf_sequence
  len = static_cast<gint>(gen_find_size(header.sequence_));
  proto_tree_add_uint64(tree, hf_sequence, tvb, offset, len,
    gint64(header.sequence_.getValue()));
  offset += len;

  // hf_source
  len = sizeof(header.source_);
  proto_tree_add_uint(tree, hf_source, tvb, offset, len,
    guint32(header.source_));
  offset += len;
}

std::string
format_sample(const DataSampleHeader& sample)
{
  std::ostringstream os;

  if (sample.submessage_id_ != SUBMESSAGE_NONE) {
    os << SubMessageId(sample.submessage_id_)
       << " (0x" << std::hex << std::setw(2) << std::setfill('0')
       << unsigned(sample.submessage_id_) << ")";
  } else {
    os << MessageId(sample.message_id_)
       << " (0x" << std::hex << std::setw(2) << std::setfill('0')
       << unsigned(sample.message_id_) << ")";
  }

  os << ", Length: " << std::dec << sample.message_length_;

  if (sample.message_id_ != TRANSPORT_CONTROL) {
    if (sample.message_id_ == SAMPLE_DATA) {
      os << ", Sequence: 0x" << std::hex << std::setw(8) << std::setfill('0')
         << sample.sequence_.getValue();
    }
    os << ", Publication: " << RepoIdConverter(sample.publication_id_);
  }

  return os.str();
}

void
dissect_sample(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree,
               const DataSampleHeader& sample, gint& offset)
{
  ACE_UNUSED_ARG(pinfo);

  gint len;

  // hf_sample_id
  len = sizeof(sample.message_id_);
  proto_tree_add_item(tree, hf_sample_id, tvb, offset, len, FALSE);
  offset += len;

  // hf_sample_sub_id
  len = sizeof(sample.submessage_id_);
  if (sample.submessage_id_ != SUBMESSAGE_NONE) {
    proto_tree_add_item(tree, hf_sample_sub_id, tvb, offset, len, FALSE);
  }
  offset += len;

  // hf_sample_flags
  len = sizeof(ACE_CDR::Octet);
  proto_tree_add_bitmask(tree, tvb, offset,
    hf_sample_flags, ett_sample_flags, sample_flags_fields, FALSE);
  offset += len;

  // hf_sample_length
  len = sizeof(sample.message_length_);
  proto_tree_add_uint_format_value(tree, hf_sample_length, tvb, offset, len,
    sample.message_length_, "%d octets", sample.message_length_);
  offset += len;

  // hf_sample_sequence
  len = static_cast<gint>(gen_find_size(sample.sequence_));
  if (sample.message_id_ == SAMPLE_DATA) {
    proto_tree_add_uint64(tree, hf_sample_sequence, tvb, offset, len,
      gint64(sample.sequence_.getValue()));
  }
  offset += len;

  // hf_sample_timestamp
  len = sizeof(sample.source_timestamp_sec_) +
        sizeof(sample.source_timestamp_nanosec_);
  if (sample.message_id_ != TRANSPORT_CONTROL) {
    nstime_t ns = {
      sample.source_timestamp_sec_,
      sample.source_timestamp_nanosec_
    };
    proto_tree_add_time(tree, hf_sample_timestamp, tvb, offset, len, &ns);
  }
  offset += len;

  // hf_sample_lifespan
  if (sample.lifespan_duration_) {
    len = sizeof(sample.lifespan_duration_sec_) +
          sizeof(sample.lifespan_duration_nanosec_);
    if (sample.message_id_ != TRANSPORT_CONTROL) {
      nstime_t ns = {
        sample.lifespan_duration_sec_,
        sample.lifespan_duration_nanosec_
      };
      proto_tree_add_time(tree, hf_sample_lifespan, tvb, offset, len, &ns);
    }
    offset += len;
  }

  // hf_sample_publication
  len = static_cast<gint>(gen_find_size(sample.publication_id_));
  if (sample.message_id_ != TRANSPORT_CONTROL) {
    RepoIdConverter converter(sample.publication_id_);
    proto_tree_add_bytes_format_value(tree, hf_sample_publication, tvb, offset, len,
      reinterpret_cast<const guint8*>(&sample.publication_id_),
      std::string(converter).c_str());
  }
  offset += len;

  // hf_sample_publisher
  if (sample.group_coherent_) {
    len = static_cast<gint>(gen_find_size(sample.publisher_id_));
    if (sample.message_id_ != TRANSPORT_CONTROL) {
      RepoIdConverter converter(sample.publisher_id_);
      proto_tree_add_bytes_format_value(tree, hf_sample_publisher, tvb, offset,
        len, reinterpret_cast<const guint8*>(&sample.publisher_id_),
        std::string(converter).c_str());
    }
    offset += len;
  }

  // hf_sample_content_filt
  if (sample.content_filter_) {
    gint total_len =
      static_cast<gint>(gen_find_size(sample.content_filter_entries_));
    len = sizeof(CORBA::ULong);
    if (sample.message_id_ != TRANSPORT_CONTROL) {
      proto_tree_add_uint_format_value(tree, hf_sample_content_filt, tvb,
        offset, len, sample.content_filter_entries_.length(), "%d entries",
        sample.content_filter_entries_.length());
    }
    offset += total_len;
    //TODO: represent the content_filter entries in wireshark, for now skip
  }

  offset += sample.message_length_; // skip marshaled data
}

} // namespace

extern "C"
dissector_Export void
dissect_opendds(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree)
{
  gint offset = 0;

  if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "OpenDDS");
  }

  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_clear(pinfo->cinfo, COL_INFO);
  }

  TransportHeader header = demarshal_data<TransportHeader>(tvb, offset);
  std::string header_str(format_header(header));

  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "DCPS %s", header_str.c_str());
  }

  if (tree != NULL) {
    proto_item* item =
      proto_tree_add_protocol_format(tree, proto_opendds, tvb, 0, -1,
        "OpenDDS DCPS Protocol, %s", header_str.c_str());

    proto_tree* header_tree = proto_item_add_subtree(item, ett_header);

    dissect_header(tvb, pinfo, header_tree, header, offset);

    while (offset < gint(tvb->length)) {
      DataSampleHeader sample = demarshal_data<DataSampleHeader>(tvb, offset);
      std::string sample_str(format_sample(sample));

      proto_item* item =
        proto_tree_add_none_format(header_tree, hf_sample, tvb, offset,
          static_cast<gint>(sample.marshaled_size()) + sample.message_length_,
          sample_str.c_str());

      proto_tree* sample_tree = proto_item_add_subtree(item, ett_sample);

      dissect_sample(tvb, pinfo, sample_tree, sample, offset);
    }
  }
}

extern "C"
dissector_Export gboolean
dissect_opendds_heur(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree)
{
  gint len = sizeof(TransportHeader::DCPS_PROTOCOL);
  guint8* data = tvb_get_ephemeral_string(tvb, 0, len);

  if (std::memcmp(data, TransportHeader::DCPS_PROTOCOL, len) != 0) {
    return FALSE;
  }

  dissect_opendds(tvb, pinfo, tree);
  return TRUE;
}

extern "C"
dissector_Export void
proto_register_opendds()
{
  static hf_register_info hf[] = {
    { &hf_version,
      { "Version",
        "opendds.version",
        FT_BYTES,
        BASE_NONE,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_flags,
      { "Flags",
        "opendds.flags",
        FT_UINT8,
        BASE_HEX,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_flags_byte_order,
      { "Byte order",
        "opendds.flags.byte_order",
        FT_BOOLEAN,
        flags_bits,
        TFS(&byte_order_tfs),
        1 << 0,
        NULL,
        HFILL
      }
    },
    { &hf_flags_first_fragment,
      { "First fragment",
        "opendds.flags.first_fragment",
        FT_BOOLEAN,
        flags_bits,
        NULL,
        1 << 1,
        NULL,
        HFILL
      }
    },
    { &hf_flags_last_fragment,
      { "Last fragment",
        "opendds.flags.last_fragment",
        FT_BOOLEAN,
        flags_bits,
        NULL,
        1 << 2,
        NULL,
        HFILL
      }
    },
    { &hf_length,
      { "Length",
        "opendds.length",
        FT_UINT16,
        BASE_HEX,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sequence,
      { "Sequence",
        "opendds.sequence",
        FT_UINT64, // wireshark needs an unsigned type for hex
        BASE_HEX,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_source,
      { "Source",
        "opendds.source",
        FT_UINT32,
        BASE_HEX,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample,
      { "Sample",
        "opendds.sample",
        FT_NONE,
        BASE_NONE,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_id,
      { "ID",
        "opendds.sample.id",
        FT_UINT8,
        BASE_HEX,
        VALS(sample_id_vals),
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_sub_id,
      { "Sub-ID",
        "opendds.sample.sub_id",
        FT_UINT8,
        BASE_HEX,
        VALS(sample_sub_id_vals),
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags,
      { "Flags",
        "opendds.sample.flags",
        FT_UINT8,
        BASE_HEX,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags_byte_order,
      { "Byte order",
        "opendds.sample.flags.byte_order",
        FT_BOOLEAN,
        sample_flags_bits,
        TFS(&byte_order_tfs),
        1 << 0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags_coherent,
      { "Coherent",
        "opendds.sample.flags.coherent",
        FT_BOOLEAN,
        sample_flags_bits,
        NULL,
        1 << 1,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags_historic,
      { "Historic",
        "opendds.sample.flags.historic",
        FT_BOOLEAN,
        sample_flags_bits,
        NULL,
        1 << 2,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags_lifespan,
      { "Lifespan",
        "opendds.sample.flags.lifespan",
        FT_BOOLEAN,
        sample_flags_bits,
        NULL,
        1 << 3,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags_group_coh,
      { "Group Coherent",
        "opendds.sample.flags.group_coherent",
        FT_BOOLEAN,
        sample_flags_bits,
        NULL,
        1 << 4,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags_content_filt,
      { "Content Filtering",
        "opendds.sample.flags.content_filter",
        FT_BOOLEAN,
        sample_flags_bits,
        NULL,
        1 << 5,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags_seq_repair,
      { "Sequence Repair",
        "opendds.sample.flags.sequence_repair",
        FT_BOOLEAN,
        sample_flags_bits,
        NULL,
        1 << 6,
        NULL,
        HFILL
      }
    },
    { &hf_sample_flags_more_frags,
      { "More Fragments",
        "opendds.sample.flags.more_frags",
        FT_BOOLEAN,
        sample_flags_bits,
        NULL,
        1 << 7,
        NULL,
        HFILL
      }
    },
    { &hf_sample_length,
      { "Length",
        "opendds.sample.length",
        FT_UINT32,
        BASE_HEX,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_sequence,
      { "Sequence",
        "opendds.sample.sequence",
        FT_UINT64, // wireshark needs an unsigned type for hex
        BASE_HEX,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_timestamp,
      { "Timestamp",
        "opendds.sample.timestamp",
        FT_ABSOLUTE_TIME,
        enum_value<ABSOLUTE_TIME_LOCAL>(),
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_lifespan,
      { "Lifespan",
        "opendds.sample.lifespan",
        FT_RELATIVE_TIME,
        BASE_NONE,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_publication,
      { "Publication",
        "opendds.sample.publication",
        FT_BYTES,
        BASE_NONE,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_publisher,
      { "Publisher",
        "opendds.sample.publisher",
        FT_BYTES,
        BASE_NONE,
        NULL,
        0,
        NULL,
        HFILL
      }
    },
    { &hf_sample_content_filt,
      { "Number of Content Filter Entries",
        "opendds.sample.content_filter_entries",
        FT_UINT32,
        BASE_HEX,
        NULL,
        0,
        NULL,
        HFILL
      }
    }
  };

  static gint *ett[] = {
    &ett_header,
    &ett_flags,
    &ett_sample,
    &ett_sample_flags
  };

  proto_opendds = proto_register_protocol(
    "OpenDDS DCPS Protocol",  // name
    "OpenDDS",                // short_name
    "opendds");               // filter_name

  proto_register_field_array(proto_opendds, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

extern "C"
dissector_Export void
proto_reg_handoff_opendds()
{
  static dissector_handle_t opendds_handle =
    create_dissector_handle(dissect_opendds, proto_opendds);

  ACE_UNUSED_ARG(opendds_handle);

  heur_dissector_add("tcp", dissect_opendds_heur, proto_opendds);
  heur_dissector_add("udp", dissect_opendds_heur, proto_opendds);
}
