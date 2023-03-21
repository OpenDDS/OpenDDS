/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DISSECTOR_PACKET_OPENDDS_H_
#define OPENDDS_DISSECTOR_PACKET_OPENDDS_H_

#include "ws_common.h"
#include "dissector_export.h"
#include "ws-wrapper-headers/packet-tcp.h"

#include <dds/DCPS/DataSampleHeader.h>
#include <dds/DCPS/transport/framework/TransportHeader.h>

#include <dds/DdsDcpsGuidTypeSupportImpl.h>

#include <ace/Hash_Map_Manager.h>

#include <epan/value_string.h>
#include <epan/ipproto.h>
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 4, 0)
#  include <wsutil/report_message.h>
#elif WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(1, 12, 0)
#  include <wsutil/report_err.h>
#else // Before 1.12
#  include <epan/report_err.h>
#endif
#ifndef NO_EXPERT
#  include <epan/expert.h>
#endif

#include <glib.h>
#include <gmodule.h>

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {
    class dissector_Export Dissector_Base;
    class dissector_Export Sample_Dissector_Manager;

    typedef ACE_Hash_Map_Manager <const char *, Dissector_Base *, ACE_Null_Mutex> DissectorMap;

    class Dissector_Manager
    {
    public:
      void add (Dissector_Base &dissector);


    private:
      DissectorMap dissectors_;

    };

    class dissector_Export Dissector_Base
    {
    public:
      Dissector_Base ()
        :tvb_(0), pinfo_(0), tree_(0)
          {}

      virtual ~Dissector_Base() {}

      void setPacket (tvbuff_t* buf, packet_info* pi, proto_tree* pt)
      {
        tvb_ = buf; pinfo_ = pi; tree_ = pt;
      }

      virtual int dissect () = 0;

      virtual bool dissect_heur () = 0;

    protected:
      tvbuff_t *tvb_;
      packet_info *pinfo_;
      proto_tree *tree_;
    };


    extern "C" {
      dissector_Export guint get_pdu_len (packet_info *, tvbuff_t *, int WS_GET_PDU_LEN_EXTRA_PARAM);
      dissector_Export WS_DISSECTOR_T_RETURN_TYPE dissect_common (
                             tvbuff_t*, packet_info*,
                             proto_tree* WS_DISSECTOR_T_EXTRA_PARAM);
      dissector_Export WS_DISSECTOR_RETURN_TYPE dissect_dds (tvbuff_t*, packet_info*, proto_tree* WS_DISSECTOR_EXTRA_PARAM);
      dissector_Export gboolean dissect_dds_heur (tvbuff_t*, packet_info*,
                                                  proto_tree* WS_HEUR_DISSECTOR_T_EXTRA_PARAM);
    }


    class dissector_Export DDS_Dissector : public Dissector_Base
    {
    public:
      static DDS_Dissector &instance();

      virtual ~DDS_Dissector() {}

      int dissect ();
      bool dissect_heur ();

      virtual void init ();
      void register_handoff ();

    private:
      DDS_Dissector ();

      static DDS_Dissector instance_;
      static bool initialized_;
      //----

      std::string format(const DCPS::TransportHeader&);
      std::string format(const DCPS::DataSampleHeader&);

      void dissect_transport_header (proto_tree*,
                                     const DCPS::TransportHeader&,
                                     gint& );

      void dissect_sample_header (proto_tree*,
                                  const DCPS::DataSampleHeader&,
                                  gint& );

      void dissect_sample_payload (proto_tree*,
                                   const DCPS::DataSampleHeader&,
                                   gint& );

    };

  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
