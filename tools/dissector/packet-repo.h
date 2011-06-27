// -*- C++ -*-

#ifndef _PACKET_REPO_H_
#define _PACKET_REPO_H_

extern "C" {

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include <epan/packet.h>
#include <epan/dissectors/packet-giop.h>
} // extern "C"

#include "tools/dissector/dissector_export.h"

#include "ace/Synch.h"
#include "ace/Hash_Map_Manager.h"

namespace OpenDDS
{
  namespace DCPS
  {
    class Dissector_Base;

    typedef void (DecodeFN) (::MessageHeader *);
    typedef ACE_Hash_Map_Manager <const char *, DecodeFN*, ACE_Null_Mutex> DecodeFN_Map;

    class dissector_Export Dissector_Base
    {
    public:
      int dissect_giop (MessageHeader *header,
                        gchar *operation,
                        gchar *idlname);
      bool dissect_heur ();

      void setPacket (tvbuff_t* buf, packet_info* pi, proto_tree* pt, int *poff)
      {
        tvb = buf; pinfo = pi; tree = pt; offset = poff;
      }

      void start_decoding ();
      virtual void init () = 0;

    protected:
      char *proto_label_;
      char *repo_id_;

      int proto_id_;
      int ett_;

      tvbuff_t *tvb;
      packet_info *pinfo;
      proto_tree *tree;
      int *offset;

      DecodeFN_Map op_functions_;
      DecodeFN *find_giop_decoder (gchar *opname);
      void add_giop_decoder (const char *opname, DecodeFN decoder);
      void init_proto_label (const char *proto);
      void init_repo_id (const char *repoid);

      char * current_op_;
      bool is_big_endian_;

    };

    class dissector_Export InfoRepo_Dissector : public Dissector_Base
    {
    public:
      static InfoRepo_Dissector& instance ();

      static gboolean explicit_giop_callback (tvbuff_t *, packet_info *,
                                              proto_tree *,int *,
                                              ::MessageHeader *, gchar *,
                                              gchar *);
      static gboolean heuristic_giop_callback (tvbuff_t *, packet_info *,
                                              proto_tree *,int *,
                                              ::MessageHeader *, gchar *,
                                              gchar *);

      virtual void init ();
      void register_handoff ();

      static void assert_topic            (::MessageHeader *);
      static void add_publication         (::MessageHeader *);
      static void add_domain_participant  (::MessageHeader *);
      static void add_subscription        (::MessageHeader *);
      static void remove_publication      (::MessageHeader *);
      static void remove_subscription     (::MessageHeader *);
      static void remove_domain_participant (::MessageHeader *);

#if 0
      static void attach_participant             (::MessageHeader *);
      static void find_topic                     (::MessageHeader *);
      static void remove_topic                   (::MessageHeader *);
      static void enable_topic                   (::MessageHeader *);
      static void remove_domain_participant      (::MessageHeader *);
      static void disassociate_participant       (::MessageHeader *);
      static void disassociate_subscription      (::MessageHeader *);
      static void disassociate_publication       (::MessageHeader *);
      static void ignore_domain_participant      (::MessageHeader *);
      static void ignore_topic                   (::MessageHeader *);
      static void ignore_subscription            (::MessageHeader *);
      static void ignore_publication             (::MessageHeader *);
      static void update_domain_participant_qos  (::MessageHeader *);
      static void update_topic_qos               (::MessageHeader *);
      static void update_publication_qos         (::MessageHeader *);
      static void update_subscription_qos        (::MessageHeader *);
      static void update_subscription_params     (::MessageHeader *);
      static void shutdown                       (::MessageHeader *);
#endif
    private:
      static InfoRepo_Dissector instance_;
      static int hf_topicId;
      static int hf_topicStatus;

      static int hf_domainId;
      static int hf_participantId;
      static int hf_topicName;
      static int hf_dataTypeName;
      static int hf_dds_topicQos;

      static int hf_addEntityRetn;
    };

    class dissector_Export DataWriterRemote_Dissector : public Dissector_Base
    {
    public:
      static DataWriterRemote_Dissector& instance ();

      static gboolean explicit_giop_callback (tvbuff_t *, packet_info *,
                                              proto_tree *,int *,
                                              ::MessageHeader *, gchar *,
                                              gchar *);
      static gboolean heuristic_giop_callback (tvbuff_t *, packet_info *,
                                              proto_tree *,int *,
                                              ::MessageHeader *, gchar *,
                                              gchar *);



      virtual void init ();
      void register_handoff ();

      static void add_associations      (::MessageHeader *);

#if 0
      static void remove_associations   (::MessageHeader *);
      static void update_incompatible_qos (::MessageHeader *);
      static void update_subscription_params (::MessageHeader *);
#endif
    private:
      static DataWriterRemote_Dissector instance_;
    };

  } // namespace DCPS
} // namespace OpenDDS

extern "C"
void proto_register_repo();

extern "C"
void proto_reg_handoff_repo();


//----



#endif //  _PACKET_REPO_H_
