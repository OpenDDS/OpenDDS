// -*- C++ -*-

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

#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportHeader.h"
#include "ace/Hash_Map_Manager.h"

// add definition of callbacks and types used to parse samples
/*
using namespace OpenDDS::DCPS;

extern "C"
typedef gboolean (dds_sub_dissector_t) (tvbuff_t *, packet_info *, proto_tree *, int *, DataSampleHeader *, gchar *, gchar * );
*/

namespace OpenDDS
{
  class dissector_Export Sample_Dissector_Base
  {
  public:
    Sample_Dissector_Base (const char *type_id);
    ~Sample_Dissector_Base ();

    virtual bool dissect (tvbuff_t *, packet_info *, proto_tree *, int *,
                          DCPS::DataSampleHeader *, gchar *, gchar * ) = 0;

    const char * typeId() const;
    const DCPS::RepoId& publication () const;
    void publication (const DCPS::RepoId& );

  private:
    char *typeId_;
    DCPS::RepoId publication_;
  };



  typedef ACE_Hash_Map_Manager <const char *, Sample_Dissector_Base *, ACE_Null_Mutex> SampleDissectorMap;


  class Sample_Dissector_Manager
  {
  public:
    void add (Sample_Dissector_Base &dissector);

  private:
    SampleDissectorMap dissectors_;

  };

  class dissector_Export Dissector_Base;

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
      :tvb(0),
       pinfo (0),
       tree (0)
    {}

    void setPacket (tvbuff_t* buf, packet_info* pi, proto_tree* pt)
    {
      tvb = buf; pinfo = pi; tree = pt;
    }

    virtual void dissect () = 0;

    virtual bool dissect_heur () = 0;

  protected:
    tvbuff_t *tvb;
    packet_info *pinfo;
    proto_tree *tree;
  };

  class dissector_Export DCPS_DDS_Dissector : public Dissector_Base
  {
  public:
    DCPS_DDS_Dissector (tvbuff_t* buf, packet_info* pi, proto_tree* pt)
      :current_op_(0)

    {
      this->setPacket(buf, pi, pt);
    }

    void dissect ();
    //int dissect_giop (MessageHeader *header, gchar *operation, gchar *idlname);

    bool dissect_heur ();

    //    static void init ();

  private:
#if 0
    static class GIOP_Decoder {
    public:
      void start (DCPS_DDS_Dissector *instance);
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } default_giop_decoder;

    static class Decode_Assert_Topic : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } assert_topic;

    static class Decode_Attach_Participant : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } attach_participant;

    static class Decode_Find_Topic : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } find_topic;

    static class Decode_Remove_Topic : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } remove_topic;

    static class Decode_Enable_Topic : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } enable_topic;

    static class Decode_Add_Publication : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } add_publication;

    static class Decode_Remove_Publication : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } remove_publication;

    static class Decode_Add_Subscription : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } add_subscription;

    static class Decode_Remove_Subscription : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } remove_subscription;

    static class Decode_Add_Domain_Participant : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } add_domain_participant;

    static class Decode_Remove_Domain_Participant : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } remove_domain_participant;

    static class Decode_Disassociate_Participant : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } disassociate_participant;

    static class Decode_Disassociate_Subscription : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } disassociate_subscription;

    static class Decode_Disassociate_Publication : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } disassociate_publication;

    static class Decode_Ignore_Domain_Participant : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } ignore_domain_participant;

    static class Decode_Ignore_Topic : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } ignore_topic;

    static class Decode_Ignore_Subscription : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } ignore_subscription;

    static class Decode_Ignore_Publication : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } ignore_publication;

    static class Decode_Update_Domain_Participant_QOS : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } update_domain_participant_qos;

    static class Decode_Update_Topic_QOS : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } update_topic_qos;

    static class Decode_Update_Publication_QOS : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } update_publication_qos;

    static class Decode_Update_Subscription_QOS : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } update_subscription_qos;

    static class Decode_Update_Subscription_Params : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } update_subscription_params;

    static class Decode_Shutdown : public GIOP_Decoder {
    public:
      virtual void decode (DCPS_DDS_Dissector *instance, ::MessageHeader *header);
    } shutdown;

    typedef ACE_Hash_Map_Manager <u_long, GIOP_Decoder *, ACE_Null_Mutex> GIOP_Decoder_Map;
    static GIOP_Decoder_Map opDecoders;

    static GIOP_Decoder *find_giop_decoder (gchar *opname);
    static void add_giop_decoder (const char *opname, GIOP_Decoder &decoder);
#endif

    std::string format(const DCPS::TransportHeader&);
    std::string format(const DCPS::DataSampleHeader&);

    void dissect_transport_header (proto_tree*,
                                   const DCPS::TransportHeader&,
                                   gint& );

    void dissect_sample_header (proto_tree*,
                                const DCPS::DataSampleHeader&,
                                gint& );

    char * current_op_;
  };

}

extern "C"
void proto_register_opendds();

extern "C"
void proto_reg_handoff_opendds();


//----



#endif //  _PACKET_OPENDDS_H_
