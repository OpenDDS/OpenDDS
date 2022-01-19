/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "packet-opendds.h"

#include "packet-repo.h"
#include "sample_manager.h"

#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/Serializer.h>

#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/Message_Block.h>
#include <ace/Log_Msg.h>
#include <ace/ACE.h>
#include <ace/Signal.h>

#include <cstring>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

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

gboolean opendds_desegment = TRUE;
const char * DCPS_MAGIC = "DCPS";
dissector_handle_t dcps_tcp_handle;

int hf_version    = -1;
int hf_length     = -1;
int hf_sequence   = -1;
int hf_source     = -1;

int hf_flags               = -1;
int hf_flags_byte_order    = -1;
int hf_flags_first_fragment= -1;
int hf_flags_last_fragment = -1;

#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(3, 4, 0)
#  define OPENDDS_WIRESHARK_FLAGS_ARRAY_TYPE int* const
#else
#  define OPENDDS_WIRESHARK_FLAGS_ARRAY_TYPE const int*
#endif

const int flags_bits = 8;
static OPENDDS_WIRESHARK_FLAGS_ARRAY_TYPE flags_fields[] = {
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
int hf_sample_content_filt_entries = -1;

int hf_sample_flags             = -1;
int hf_sample_flags_byte_order  = -1;
int hf_sample_flags_coherent    = -1;
int hf_sample_flags_historic    = -1;
int hf_sample_flags_lifespan    = -1;
int hf_sample_flags_group_coh   = -1;
int hf_sample_flags_content_filt= -1;
int hf_sample_flags_seq_repair  = -1;
int hf_sample_flags_more_frags  = -1;
int hf_sample_flags2            = -1;
int hf_sample_flags2_cdr_encap  = -1;
int hf_sample_flags2_key_only   = -1;

// Payload IDL Type, ex "Messenger::Message"
int hf_sample_payload = -1;

#ifndef NO_EXPERT
// This signifies a problem with the structure of the payload
expert_field ei_sample_payload_error = EI_INIT;
// All other problems that are considered less serious, like missing
// dissector ITL file or problems with individual fields of payload.
expert_field ei_sample_payload_warning = EI_INIT;
#endif

const int sample_flags_bits = 8;
static OPENDDS_WIRESHARK_FLAGS_ARRAY_TYPE sample_flags_fields[] = {
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

const int sample_flags2_bits = 8;
static OPENDDS_WIRESHARK_FLAGS_ARRAY_TYPE sample_flags2_fields[] = {
  &hf_sample_flags2_cdr_encap,
  &hf_sample_flags2_key_only,
  NULL
};

gint ett_trans_header  = -1;
gint ett_trans_flags   = -1;
gint ett_sample_header = -1;
gint ett_sample_flags  = -1;
gint ett_sample_flags2 = -1;
gint ett_filters = -1;
gint ett_sample_payload = -1;

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
  { OpenDDS::DCPS::SAMPLE_DATA,            "SAMPLE_DATA"           },
  { OpenDDS::DCPS::DATAWRITER_LIVELINESS,  "DATAWRITER_LIVELINESS" },
  { OpenDDS::DCPS::INSTANCE_REGISTRATION,  "INSTANCE_REGISTRATION" },
  { OpenDDS::DCPS::UNREGISTER_INSTANCE,    "UNREGISTER_INSTANCE"   },
  { OpenDDS::DCPS::DISPOSE_INSTANCE,       "DISPOSE_INSTANCE"      },
  { OpenDDS::DCPS::GRACEFUL_DISCONNECT,    "GRACEFUL_DISCONNECT"   },
  { OpenDDS::DCPS::REQUEST_ACK,            "REQUEST_ACK"           },
  { OpenDDS::DCPS::SAMPLE_ACK,             "SAMPLE_ACK"            },
  { OpenDDS::DCPS::END_COHERENT_CHANGES,   "END_COHERENT_CHANGES"  },
  { OpenDDS::DCPS::TRANSPORT_CONTROL,      "TRANSPORT_CONTROL"     },
  { OpenDDS::DCPS::DISPOSE_UNREGISTER_INSTANCE, "DISPOSE_UNREGISTER_INSTANCE" },
  { OpenDDS::DCPS::END_HISTORIC_SAMPLES,   "END_HISTORIC_SAMPLES"  },
  { 0,                                     NULL                    }
};

const value_string sample_sub_id_vals[] = {
  { OpenDDS::DCPS::SUBMESSAGE_NONE,        "SUBMESSAGE_NONE"       },
  { OpenDDS::DCPS::MULTICAST_SYN,          "MULTICAST_SYN"         },
  { OpenDDS::DCPS::MULTICAST_SYNACK,       "MULTICAST_SYNACK"      },
  { OpenDDS::DCPS::MULTICAST_NAK,          "MULTICAST_NAK"         },
  { OpenDDS::DCPS::MULTICAST_NAKACK,       "MULTICAST_NAKACK"      },
  { 0,                                     NULL                    }
};

} // namespace

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {
    // static member values
    DDS_Dissector DDS_Dissector::instance_;
    bool DDS_Dissector::initialized_ = false;

    DDS_Dissector&
    DDS_Dissector::instance()
    {
      return instance_;
    }

    const Encoding encoding(Encoding::KIND_UNALIGNED_CDR);

    template<typename T>
    T
    demarshal_data(tvbuff_t* tvb, gint offset)
    {
      T t;

      guint len = std::min(ws_tvb_length(tvb) - offset,
                           static_cast<guint>(t.get_max_serialized_size()));
      const guint8* data = tvb_get_ptr(tvb, offset, len);

      ACE_Message_Block mb(reinterpret_cast<const char*>(data));
      mb.wr_ptr(len);

      t.init(&mb);

      return t;
    }

    DDS_Dissector::DDS_Dissector ()
    {
    }

    std::string
    DDS_Dissector::format (const DCPS::TransportHeader& header)
    {
      std::ostringstream os;

      os << "Length: " << std::dec << header.length_;
      if (header.first_fragment_) os << ", First Fragment";
      if (header.last_fragment_) os << ", Last Fragment";
      os << ", Sequence: 0x" << std::hex << std::setw(8) << std::setfill('0')
         << header.sequence_.getValue();
      os << ", Source: 0x" << std::hex << std::setw(16) << std::setfill('0')
         << ACE_UINT64(header.source_);

      return os.str();
    }

    std::string
    DDS_Dissector::format (const DCPS::DataSampleHeader& sample)
    {
      std::ostringstream os;

      if (sample.submessage_id_ != DCPS::SUBMESSAGE_NONE) {
        os << DCPS::SubMessageId(sample.submessage_id_)
           << " (0x" << std::hex << std::setw(2) << std::setfill('0')
           << unsigned(sample.submessage_id_) << ")";
      } else {
        os << DCPS::MessageId(sample.message_id_)
           << " (0x" << std::hex << std::setw(2) << std::setfill('0')
           << unsigned(sample.message_id_) << ")";
      }

      os << ", Length: " << std::dec << sample.message_length_;

      if (sample.message_id_ != DCPS::TRANSPORT_CONTROL) {
        if (sample.message_id_ == DCPS::SAMPLE_DATA) {
          os << ", Sequence: 0x" << std::hex << std::setw(8) << std::setfill('0')
             << sample.sequence_.getValue();
        }
        os << ", Publication: " << DCPS::GuidConverter(sample.publication_id_);
      }

      return os.str();
    }


    void
    DDS_Dissector::dissect_transport_header (proto_tree* ltree,
                                             const DCPS::TransportHeader& header,
                                             gint& offset)
    {
      gint len;

      offset += sizeof(header.protocol_) - 2; // skip preamble

      // hf_version
      len = sizeof(header.protocol_) - offset;
      const guint8 *data_ptr =
        reinterpret_cast<const guint8*>(header.protocol_ + offset);
      proto_tree_add_bytes_format_value (ltree, hf_version, tvb_,
                                         offset, len, data_ptr,
                                         "%d.%d",
                                         header.protocol_[4],
                                         header.protocol_[5]);
      offset += len;

      // hf_flags
      len = sizeof(ACE_CDR::Octet);
      proto_tree_add_bitmask(ltree, tvb_, offset, hf_flags, ett_trans_flags,
                             flags_fields, FALSE);
      offset += len;

      offset += sizeof(header.reserved_);     // skip reserved

      // hf_length
      len = sizeof(header.length_);
      proto_tree_add_uint_format_value(ltree, hf_length, tvb_, offset, len,
                                       header.length_, "%d octets",
                                       header.length_);
      offset += len;

      // hf_sequence
      size_t size = serialized_size(encoding, header.sequence_);
      len = static_cast<gint>(size);
      proto_tree_add_uint64(ltree, hf_sequence, tvb_, offset, len,
                            gint64(header.sequence_.getValue()));
      offset += len;

      // hf_source
      len = sizeof(header.source_);
      proto_tree_add_uint(ltree, hf_source, tvb_, offset, len,
                          guint32(header.source_));
      offset += len;
    }

    void
    DDS_Dissector::dissect_sample_header (proto_tree* ltree,
                                          const DCPS::DataSampleHeader& sample,
                                          gint& offset)
    {
      gint len;

      // hf_sample_id
      len = sizeof(sample.message_id_);
      proto_tree_add_item(ltree, hf_sample_id, tvb_, offset, len, FALSE);
      offset += len;

      // hf_sample_sub_id
      len = sizeof(sample.submessage_id_);
      if (sample.submessage_id_ != SUBMESSAGE_NONE)
        {
          proto_tree_add_item(ltree, hf_sample_sub_id, tvb_, offset, len, FALSE);
        }
      offset += len;

      // hf_sample_flags
      len = sizeof(ACE_CDR::Octet);
      proto_tree_add_bitmask(ltree, tvb_, offset,
                             hf_sample_flags,
                             ett_sample_flags, sample_flags_fields, FALSE);
      offset += len;

      // hf_sample_flags2
      len = sizeof(ACE_CDR::Octet);
      proto_tree_add_bitmask(ltree, tvb_, offset,
                             hf_sample_flags2,
                             ett_sample_flags2, sample_flags2_fields, FALSE);
      offset += len;

      // hf_sample_length
      len = sizeof(sample.message_length_);
      proto_tree_add_uint_format_value(ltree, hf_sample_length,
                                       tvb_, offset, len,
                                       sample.message_length_,
                                       "%d octets", sample.message_length_);
      offset += len;

      // hf_sample_sequence
      size_t size = serialized_size(encoding, sample.sequence_);
      len = static_cast<gint>(size);
      if (sample.message_id_ == SAMPLE_DATA) {
        proto_tree_add_uint64(ltree, hf_sample_sequence, tvb_, offset, len,
                              gint64(sample.sequence_.getValue()));
      }
      offset += len;

      // hf_sample_timestamp
      len = sizeof(sample.source_timestamp_sec_) +
        sizeof(sample.source_timestamp_nanosec_);
      if (sample.message_id_ != TRANSPORT_CONTROL)
        {
          nstime_t ns =
            {
              sample.source_timestamp_sec_,
              int(sample.source_timestamp_nanosec_)
            };
          proto_tree_add_time(ltree, hf_sample_timestamp, tvb_, offset, len, &ns);
        }
      offset += len;

      // hf_sample_lifespan
      if (sample.lifespan_duration_)
        {
          len = sizeof(sample.lifespan_duration_sec_) +
            sizeof(sample.lifespan_duration_nanosec_);
          if (sample.message_id_ != TRANSPORT_CONTROL)
            {
              nstime_t ns =
                {
                  sample.lifespan_duration_sec_,
                  int(sample.lifespan_duration_nanosec_)
                };
              proto_tree_add_time(ltree, hf_sample_lifespan, tvb_,
                                  offset, len, &ns);
            }
          offset += len;
        }

      // hf_sample_publication
      size = serialized_size(encoding, sample.publication_id_);
      len = static_cast<gint>(size);
      const guint8 *data_ptr =
        reinterpret_cast<const guint8*>(&sample.publication_id_);
      if (sample.message_id_ != TRANSPORT_CONTROL)
        {
          GuidConverter converter (sample.publication_id_);
          proto_tree_add_bytes_format_value (ltree, hf_sample_publication,
                                             tvb_, offset, len, data_ptr, "%s",
                                             std::string(converter).c_str());
        }
      offset += len;

      // hf_sample_publisher
      if (sample.group_coherent_)
        {
          size = serialized_size(encoding, sample.publication_id_);
          len = static_cast<gint>(size);
          data_ptr = reinterpret_cast<const guint8*>(&sample.publisher_id_);
          if (sample.message_id_ != DCPS::TRANSPORT_CONTROL)
            {
              DCPS::GuidConverter converter(sample.publisher_id_);
              proto_tree_add_bytes_format_value (ltree, hf_sample_publisher,
                                                 tvb_, offset, len,
                                                 data_ptr, "%s",
                                                 std::string(converter).c_str()
                                                 );
            }
          offset += len;
        }

      // hf_sample_content_filt
      if (sample.content_filter_)
        {
          size = serialized_size(encoding, sample.content_filter_entries_);
          gint total_len = static_cast<gint>(size);
          len = sizeof(CORBA::ULong);
          if (sample.message_id_ != DCPS::TRANSPORT_CONTROL)
            {
              proto_item *item =
                proto_tree_add_uint
                (ltree, hf_sample_content_filt, tvb_,
                 offset, len,
                 sample.content_filter_entries_.length());
              offset += len;
              proto_tree *subtree = proto_item_add_subtree (item, ett_filters);
              for (CORBA::ULong i = 0;
                   i < sample.content_filter_entries_.length();
                   i++)
                {
                  // Get Entry Value
                  const GUID_t &filter = sample.content_filter_entries_[i];
                  DCPS::GuidConverter converter(filter);
                  std::stringstream strm;
                  strm << converter;
                  std::string guid = strm.str();

                  // Get Entry Size
                  size = serialized_size(encoding, filter);
                  len = static_cast<gint>(size);

                  // Add to Wireshark
                  proto_tree_add_string_format(
                    subtree, hf_sample_content_filt_entries,
                    tvb_, offset, len,
                    guid.c_str(),
                    "Content Filter [%u]: %s", i, guid.c_str()
                  );
                  offset += len;
                }

            }
          else
            {
              offset += total_len;
            }
        }
    }

    // Try to gracefully handle a Seg Fault, give Wireshark a chance to
    // finish its business, and tell inform the user that their ITL file
    // is invalid
    void sample_dissect_handle_sigsegv(int /* signal */) {
      ACE_DEBUG ((LM_DEBUG,
        "DDS_Dissector::dissect_sample_payload: "
        "Encountered SIGSEGV (Segmentation Fault) signal during "
        "sample payload dissection. "
        "There is a pointer bug in the sample dissector or the ITL "
        "file provided matched the type name but does not actually have the "
        "same type."
        "\n"
      ));
      report_failure(
        "There has been a fatal error in the OpenDDS Dissector:\n"
        "There is a pointer bug in the sample dissector or the ITL "
        "file provided matched the type name but does not actually have the "
        "same type."
      );
      exit(EXIT_FAILURE);
    }

    void
    DDS_Dissector::dissect_sample_payload (proto_tree* ltree,
                                           const DCPS::DataSampleHeader& header,
                                           gint& offset)
    {
      if (header.message_id_ != SAMPLE_DATA)
        {
          offset += header.message_length_;
          return;
        }

      GuidConverter converter(header.publication_id_);

      if (header.cdr_encapsulation_) {
        ACE_DEBUG ((LM_DEBUG,
                    "DDS_Dissector::dissect_sample_payload: "
                    "dissection of CDR-encapsulated data is not currently "
                    "supported\n"));
        offset += header.message_length_;

        // Mark Packet
#ifndef NO_EXPERT
        proto_tree_add_expert_format(
          ltree,
          pinfo_,
          &ei_sample_payload_warning,
          tvb_,
          offset,
          (gint) header.message_length_,
          "Dissecting CDR-encapsulated data is not currently supported.\n"
        );
#endif

        return;
      }

      const char * data_name =
        InfoRepo_Dissector::instance().topic_for_pub(&header.publication_id_);

      if (data_name == 0) {
        ACE_DEBUG ((LM_DEBUG,
                    "DDS_Dissector::dissect_sample_payload: "
                    "couldn't dissect payload: no topic for %C\n",
                    std::string(converter).c_str()));

        // Mark Packet
#ifndef NO_EXPERT
        proto_tree_add_expert_format(
          ltree,
          pinfo_,
          &ei_sample_payload_warning,
          tvb_,
          offset,
          (gint) header.message_length_,
          "Couldn't Dissect Payload: No Topic Found for %s\n",
          std::string(converter).c_str()
        );
#endif

        offset += header.message_length_; // skip marshaled data

        return;
      }

      // Set Payload Field/Tree
      proto_item * payload_item = proto_tree_add_string(
        ltree, hf_sample_payload, tvb_, offset,
        (gint) header.message_length_,
        data_name);
      proto_tree * contents_tree = proto_item_add_subtree(
          payload_item, ett_sample_payload);

      // Try to Dissect Payload
      Sample_Dissector* data_dissector =
        Sample_Manager::instance().find(data_name);
      if (!data_dissector) {
        ACE_DEBUG ((LM_DEBUG,
                    "DDS_Dissector::dissect_sample_payload: "
                    "couldn't dissect payload: "
                    "no dissector found for %C\n",
                    data_name));

        // Mark Packet
#ifndef NO_EXPERT
        proto_tree_add_expert_format(
          ltree,
          pinfo_,
          &ei_sample_payload_warning,
          tvb_,
          offset,
          (gint) header.message_length_,
          "Couldn't Dissect Payload: "
          "No Dissector Found for %s\n",
          data_name
        );
#endif

        offset += header.message_length_; // skip marshaled data
      } else {
        // Push DCPS Primary Data Type Namespace
        // Convert, for example, "1::2::3" to "1.2.3"
        Sample_Base::clear_ns();
        std::string ns;
        char c = 'a';
        bool delimiter = false;
        for (size_t i = 0; c; i++) {
          c = data_name[i];
          if (c == ':') {
            if (delimiter) {
              Sample_Base::push_ns(ns);
              ns = "";
              delimiter = false;
            } else {
              delimiter = true;
            }
          } else if (c) { // Any value that's not : or NULL
            ns.push_back(c);
            delimiter = false;
          }
        }
        Sample_Base::push_ns(ns);

        // Parameters to pass between the recursive dissect calls
        const guint8 * const_data = tvb_get_ptr(
          tvb_, offset, header.message_length_
        );
        guint8 * data = const_cast<guint8 *>(const_data);
        size_t size = static_cast<size_t>(ws_tvb_length(tvb_) - offset);
        Wireshark_Bundle params(
          (char *) data, size,
          header.byte_order_ != ACE_CDR_BYTE_ORDER,
          Encoding::ALIGN_NONE // For now alignment is not supported
        );
        params.tvb = tvb_;
        params.info = pinfo_;
        params.tree = contents_tree;
        params.offset = offset;
#ifndef NO_EXPERT
        params.warning_ef = &ei_sample_payload_warning;
#endif

        /*
         * Handle Seg Faults that might be caused by incorrect ITL file
         *
         * Check for environment variable $OPENDDS_DISSECTORS_SIGSEGV,
         * If it exists don't handle seg faults.
         * Either way Wireshark will crash if the ITL file is the wrong type,
         * or technically inconsistent size.
         */
        bool handle = ACE_OS::getenv(ACE_TEXT("OPENDDS_DISSECTOR_SIGSEGV")) == 0;
        ACE_Sig_Action sig_act(sample_dissect_handle_sigsegv);
        if (handle) sig_act.register_action(SIGSEGV); // Handle Segfaults

        try {
          offset = data_dissector->dissect(params); // Dissect Sample Payload
        } catch (const Sample_Dissector_Error& e) {
#ifndef NO_EXPERT
          proto_tree_add_expert_format(
            contents_tree, pinfo_, &ei_sample_payload_error,
            tvb_, offset, -1, "%s", e.what()
          );
#endif
          ACE_DEBUG ((LM_DEBUG,
            "DDS_Dissector::dissect_sample_payload: %C\n", e.what()
          ));
          offset = static_cast<gint>(params.offset + header.message_length_);
        }

        // Clean Up
        if (handle) sig_act.handler(SIG_DFL); // Restore default segfault behavior
        Sample_Base::clear_ns();
      }
    }

    int
    DDS_Dissector::dissect ()
    {

      gint offset = 0;

      col_set_str(pinfo_->cinfo, COL_PROTOCOL, "OpenDDS");

      col_clear(pinfo_->cinfo, COL_INFO);

      TransportHeader trans =
        demarshal_data<TransportHeader>(tvb_, offset);
      std::string trans_str(format(trans));

      col_add_fstr(pinfo_->cinfo, COL_INFO, "DCPS %s", trans_str.c_str());

      if (tree_ != NULL) {
        proto_item* item =
          proto_tree_add_protocol_format(tree_,
                                         proto_opendds, tvb_, 0, -1,
                                         "OpenDDS DCPS Protocol, %s",
                                         trans_str.c_str());

        proto_tree* trans_tree =
          proto_item_add_subtree(item, ett_trans_header);

        this->dissect_transport_header (trans_tree, trans, offset);

        while (offset < gint(ws_tvb_length(tvb_)))
          {
            DataSampleHeader sample =
              demarshal_data<DataSampleHeader>(tvb_, offset);

            std::string sample_str(format(sample));

            proto_item* item =
              proto_tree_add_none_format
              (trans_tree, hf_sample, tvb_, offset,
               static_cast<gint>(sample.get_serialized_size()) +
               sample.message_length_,
               "%s",
               sample_str.c_str()
               );

            proto_tree* sample_tree =
              proto_item_add_subtree(item, ett_sample_header);

            this->dissect_sample_header (sample_tree, sample, offset);

            sample_tree = proto_item_add_subtree (item, ett_sample_header);
            this->dissect_sample_payload (sample_tree, sample, offset);
          }
      }
      return offset;
    }

    bool
    DDS_Dissector::dissect_heur()
    {
      // Check for Magic Number in Header
      gint len = sizeof(DCPS::TransportHeader::DCPS_PROTOCOL);
      guint8* data = ws_tvb_get_ephemeral_string(tvb_, 0, len);
      if (std::memcmp(data, DCPS::TransportHeader::DCPS_PROTOCOL, len) != 0) {
        return false;
      }

      if ( pinfo_->ptype == PT_TCP ) {
        conversation_set_dissector(ws_find_or_create_conversation(pinfo_), dcps_tcp_handle);
      } else {
        this->dissect();
      }

      return true;
    }

    // function passed to tcp pdu parser for computing packet length
    extern "C"
    guint
    get_pdu_len(packet_info *, tvbuff_t *tvb, int offset WS_GET_PDU_LEN_EXTRA_PARAM)
    {
      if ( tvb_memeql(tvb, 0, reinterpret_cast<const guint8*>(DCPS_MAGIC) ,4) != 0)
        return 0;

      TransportHeader header =
        demarshal_data<TransportHeader>(tvb, offset);

      return header.length_ + static_cast<guint>(header.get_max_serialized_size());
    }

    // function passed to tcp pdu parser to do actual work.
    extern "C"
    WS_DISSECTOR_T_RETURN_TYPE
    dissect_common(tvbuff_t* tvb, packet_info* pinfo,
                   proto_tree* tree WS_DISSECTOR_T_EXTRA_PARAM)
    {
      DDS_Dissector::instance().setPacket(tvb, pinfo, tree);
#ifdef WS_DISSECTOR_T_RETURN_TYPE_INT
      return
#endif
      DDS_Dissector::instance().dissect();
    }

    extern "C"
    WS_DISSECTOR_RETURN_TYPE
    dissect_dds (tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree WS_DISSECTOR_EXTRA_PARAM)
    {
      // this can filter between DCPS and RTPS as needed.
      tcp_dissect_pdus(tvb,
                       pinfo,
                       tree,
                       opendds_desegment,
                       sizeof (OpenDDS::DCPS::TransportHeader),
                       get_pdu_len,
                       dissect_common
                       WS_TCP_DISSECT_PDUS_EXTRA_ARG);

#ifdef WS_DISSECTOR_RETURN_INT
      return tvb_captured_length(tvb);
#endif
    }

    extern "C"
    dissector_Export gboolean
    dissect_dds_heur(tvbuff_t* tvb, packet_info* pinfo,
                     proto_tree* tree WS_HEUR_DISSECTOR_T_EXTRA_PARAM)
    {
      DDS_Dissector::instance().setPacket(tvb, pinfo, tree);

      return DDS_Dissector::instance().dissect_heur();
    }

    void
    DDS_Dissector::init()
    {

      if (initialized_)
        return;
      initialized_ = true;

      //#define HFILL 0, 0, HF_REF_TYPE_NONE, 0, NULL, NULL
#define NULL_HFILL NULL, 0, NULL, HFILL
#define BF_HFILL(X) NULL, 1 << X, NULL, HFILL

      static hf_register_info hf[] = {
        { &hf_version,
            { "Version", "opendds.version", FT_BYTES, BASE_NONE, NULL_HFILL }
        },
        { &hf_flags,
            { "Flags", "opendds.flags", FT_UINT8, BASE_HEX, NULL_HFILL }
        },
        { &hf_flags_byte_order,
            { "Byte order", "opendds.flags.byte_order",
                FT_BOOLEAN, flags_bits, TFS(&byte_order_tfs),
                1 << 0, NULL, HFILL }
        },
        { &hf_flags_first_fragment,
            { "First fragment", "opendds.flags.first_fragment",
                FT_BOOLEAN, flags_bits, BF_HFILL(1) }
        },
        { &hf_flags_last_fragment,
            { "Last fragment", "opendds.flags.last_fragment",
                FT_BOOLEAN, flags_bits, BF_HFILL(2) }
        },
        { &hf_length,
            { "Length", "opendds.length",
                FT_UINT16, BASE_HEX, NULL_HFILL
                }
        },
        { &hf_sequence,
            { "Sequence", "opendds.sequence",
                FT_UINT64, BASE_HEX, NULL_HFILL
                }
        },
        { &hf_source,
            { "Source", "opendds.source",
                FT_UINT32, BASE_HEX, NULL_HFILL
                }
        },
        { &hf_sample,
            { "Sample", "opendds.sample",
                FT_NONE, BASE_NONE, NULL_HFILL
                }
        },
        { &hf_sample_id,
            { "ID", "opendds.sample.id",
                FT_UINT8, BASE_HEX, VALS(sample_id_vals), 0, NULL, HFILL
                }
        },
        { &hf_sample_sub_id,
            { "Sub-ID", "opendds.sample.sub_id",
                FT_UINT8, BASE_HEX, VALS(sample_sub_id_vals), 0, NULL, HFILL
                }
        },
        {
          &hf_sample_flags,
            { "Flags", "opendds.sample.flags",
                FT_UINT8, BASE_HEX, NULL_HFILL
                }
        },
        { &hf_sample_flags_byte_order,
            { "Byte order", "opendds.sample.flags.byte_order",
                FT_BOOLEAN, sample_flags_bits,
                TFS(&byte_order_tfs), 1 << 0, NULL, HFILL
                }
        },
        { &hf_sample_flags_coherent,
            { "Coherent", "opendds.sample.flags.coherent",
                FT_BOOLEAN, sample_flags_bits, BF_HFILL(1)
                }
        },
        { &hf_sample_flags_historic,
            { "Historic", "opendds.sample.flags.historic",
                FT_BOOLEAN, sample_flags_bits, BF_HFILL(2)
                }
        },
        { &hf_sample_flags_lifespan,
            { "Lifespan", "opendds.sample.flags.lifespan",
                FT_BOOLEAN, sample_flags_bits, BF_HFILL(3)
                }
        },
        { &hf_sample_flags_group_coh,
            { "Group Coherent", "opendds.sample.flags.group_coherent",
                FT_BOOLEAN, sample_flags_bits, BF_HFILL(4)
                }
        },
        { &hf_sample_flags_content_filt,
            { "Content Filtering", "opendds.sample.flags.content_filter",
                FT_BOOLEAN, sample_flags_bits, BF_HFILL(5)
                }
        },
        { &hf_sample_flags_seq_repair,
            { "Sequence Repair", "opendds.sample.flags.sequence_repair",
                FT_BOOLEAN, sample_flags_bits, BF_HFILL(6)
                }
        },
        { &hf_sample_flags_more_frags,
            { "More Fragments", "opendds.sample.flags.more_frags",
                FT_BOOLEAN, sample_flags_bits, BF_HFILL(7)
                }
        },
        { &hf_sample_flags2,
            { "Flags2", "opendds.sample.flags2",
                FT_UINT8, BASE_HEX, NULL_HFILL
            }
        },
        { &hf_sample_flags2_cdr_encap,
            { "CDR Encapsulation", "opendds.sample.flags2.cdr_encap",
              FT_BOOLEAN, sample_flags2_bits, BF_HFILL(0)
            }
        },
        { &hf_sample_flags2_key_only,
            { "Key Fields Only", "opendds.sample.flags2.key_only",
              FT_BOOLEAN, sample_flags2_bits, BF_HFILL(1)
            }
        },
        { &hf_sample_length,
            { "Length", "opendds.sample.length",
                FT_UINT32, BASE_HEX, NULL_HFILL
                }
        },
        { &hf_sample_sequence,
            { "Sequence", "opendds.sample.sequence",
                FT_UINT64, BASE_HEX, NULL_HFILL
                }
        },
        { &hf_sample_timestamp,
            { "Timestamp", "opendds.sample.timestamp",
                FT_ABSOLUTE_TIME, enum_value<ABSOLUTE_TIME_LOCAL>(), NULL_HFILL
                }
        },
        { &hf_sample_lifespan,
            { "Lifespan", "opendds.sample.lifespan",
                FT_RELATIVE_TIME, BASE_NONE, NULL_HFILL
                }
        },
        { &hf_sample_publication,
            { "Publication", "opendds.sample.publication",
                FT_BYTES, BASE_NONE, NULL_HFILL
                }
        },
        { &hf_sample_publisher,
            { "Publisher", "opendds.sample.publisher",
                FT_BYTES, BASE_NONE, NULL_HFILL
                }
        },
        { &hf_sample_content_filt,
            { "Number of Content Filters",
                "opendds.sample.content_filter",
                FT_UINT32, BASE_DEC, NULL_HFILL
                }
        },
        { &hf_sample_content_filt_entries,
            { "Content Filter",
                "opendds.sample.content_filter_entries",
                FT_STRING, BASE_NONE, NULL_HFILL
                }
        },
        { &hf_sample_payload,
          {"Payload",
            payload_namespace.c_str(),
            FT_STRING, BASE_NONE, NULL_HFILL
          }
        }
      };

      for (unsigned i = 0; i < array_length(hf); i++) {
        Sample_Manager::instance().add_protocol_field(hf[i]);
      }

      Sample_Manager::instance().init();

      static gint *ett[] = {
        &ett_trans_header,
        &ett_trans_flags,
        &ett_sample_header,
        &ett_sample_flags,
        &ett_sample_flags2,
        &ett_filters,
        &ett_sample_payload
      };

      // Expert Information
#ifndef NO_EXPERT
      static ei_register_info ei[] = {
        {&ei_sample_payload_warning, {
          "opendds.sample.dissect_warning",
          PI_UNDECODED, PI_WARN,
          "Unable dissect a field in the sample payload successfully.",
          EXPFILL
        }},
        {&ei_sample_payload_error, {
          "opendds.sample.dissect_error",
          PI_MALFORMED, PI_ERROR,
          "Unable to dissect sample payload because data not match ITL file.",
          EXPFILL
        }}
      };
#endif

      // Register Protocol
      proto_opendds =
        proto_register_protocol
        ("OpenDDS DCPS Protocol",  // name
         "OpenDDS",                // short_name
         "opendds");               // filter_name

      proto_register_field_array(proto_opendds,
        Sample_Manager::instance().fields_array(),
        static_cast<int>(Sample_Manager::instance().number_of_fields())
      );
      proto_register_subtree_array(ett, array_length(ett));

#ifndef NO_EXPERT
      // Register Expert Information
      expert_register_field_array(
        expert_register_protocol(proto_opendds),
        ei, array_length(ei)
      );
#endif
    }

    void
    DDS_Dissector::register_handoff()
    {
      dcps_tcp_handle = create_dissector_handle(dissect_dds, proto_opendds);

      heur_dissector_add("tcp", dissect_dds_heur, WS_HEUR_DISSECTOR_EXTRA_ARGS1(TCP, tcp)
                         proto_opendds WS_HEUR_DISSECTOR_EXTRA_ARGS2);
      heur_dissector_add("udp", dissect_dds_heur, WS_HEUR_DISSECTOR_EXTRA_ARGS1(UDP, udp)
                         proto_opendds WS_HEUR_DISSECTOR_EXTRA_ARGS2);

      ws_dissector_add_handle("tcp.port", dcps_tcp_handle);  /* for "decode-as" */
    }

  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
