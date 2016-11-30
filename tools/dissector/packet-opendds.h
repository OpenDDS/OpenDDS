// -*- C++ -*-
/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _PACKET_OPENDDS_H_
#define _PACKET_OPENDDS_H_


extern "C" {

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include <epan/value_string.h>
#include <epan/ipproto.h>
#include <epan/packet.h>
#include <epan/dissectors/packet-tcp.h>
  //#include <epan/dissectors/packet-giop.h>
} // extern "C"

#include "tools/dissector/dissector_export.h"
#include "tools/dissector/ws_common.h"

#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportHeader.h"
#include "ace/Hash_Map_Manager.h"

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
      dissector_Export guint get_pdu_len (packet_info *, tvbuff_t *, int);
      dissector_Export WS_DISSECTOR_T_RETURN_TYPE dissect_common (
                             tvbuff_t*, packet_info*,
                             proto_tree* WS_DISSECTOR_T_EXTRA_PARAM);
      dissector_Export void dissect_dds (tvbuff_t*, packet_info*, proto_tree*);
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

#endif //  _PACKET_OPENDDS_H_
