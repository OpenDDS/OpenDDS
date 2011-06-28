/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tools/dissector/packet-repo.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/RepoIdConverter.h"

#include "ace/Basic_Types.h"
#include "ace/CDR_Base.h"
#include "ace/Message_Block.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/ACE.h"

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

  const value_string byte_order_vals[] = {
    { 0x0,  "Big Endian"    },
    { 0x1,  "Little Endian" },
    { 0,    NULL            }
  };

  const true_false_string byte_order_tfs = {
    "Little Endian",
    "Big Endian"
  };

  const value_string topic_status_vals[] = {
    { OpenDDS::DCPS::CREATED, "CREATED" },
    { OpenDDS::DCPS::ENABLED, "ENABLED" },
    { OpenDDS::DCPS::FOUND, "FOUND" },
    { OpenDDS::DCPS::NOT_FOUND, "NOT_FOUND" },
    { OpenDDS::DCPS::REMOVED, "REMOVED" },
    { OpenDDS::DCPS::CONFLICTING_TYPENAME, "CONFLICTING_TYPENAME" },
    { OpenDDS::DCPS::INTERNAL_ERROR, "INTERNAL_ERROR" }
  };

  void
  proto_register_repo()
  {
    OpenDDS::DCPS::InfoRepo_Dissector::instance().init ();
    OpenDDS::DCPS::DataWriterRemote_Dissector::instance().init ();
  }

  void
  proto_reg_handoff_repo ()
  {
    OpenDDS::DCPS::InfoRepo_Dissector::instance().register_handoff ();
    OpenDDS::DCPS::DataWriterRemote_Dissector::instance().register_handoff ();
  }

} // namespace


namespace OpenDDS
{
  namespace DCPS
  {

    // static member values
    InfoRepo_Dissector InfoRepo_Dissector::instance_;
    int InfoRepo_Dissector::hf_topicId = -1;
    int InfoRepo_Dissector::hf_topicStatus = -1;

    int InfoRepo_Dissector::hf_domainId = -1;
    int InfoRepo_Dissector::hf_participantId = -1;
    int InfoRepo_Dissector::hf_topicName = -1;
    int InfoRepo_Dissector::hf_dataTypeName = -1;
    int InfoRepo_Dissector::hf_dds_topicQos = -1;

    int InfoRepo_Dissector::hf_addEntityRetn = -1;


    DataWriterRemote_Dissector DataWriterRemote_Dissector::instance_;

    InfoRepo_Dissector&
    InfoRepo_Dissector::instance()
    {
      return instance_;
    }

    DataWriterRemote_Dissector&
    DataWriterRemote_Dissector::instance()
    {
      return instance_;
    }

    int
    Dissector_Base::dissect_giop (::MessageHeader *header,
                                  gchar *operation,
                                  gchar *idlname)
    {
      ACE_DEBUG ((LM_DEBUG,"dissect_giop called for %s::%s\n",
                  idlname, operation));

      if (idlname != 0 &&
          ACE_OS::strcmp (idlname, this->repo_id_) != 0)
        return -1;

      this->is_big_endian_ = ::is_big_endian(header);

      //GIOP_Decoder *decoder = find_giop_decoder (operation);
      DecodeFN *decoder = find_giop_decoder (operation);

      if (decoder == 0)
        return -1;

      (*decoder)(header);

      //    decoder->decode (header);
      return 0;
    }

    void
    Dissector_Base::start_decoding()
    {
      proto_item *ti = 0;
      proto_tree *subtree = 0;
      col_set_str (pinfo->cinfo, COL_PROTOCOL, this->proto_label_);
      if (tree) {
        ti = proto_tree_add_item(tree,
                                 this->proto_id_,
                                 tvb, *offset, -1, 0);
        subtree = proto_item_add_subtree (ti, this->ett_);
        this->tree = subtree;
      }
    }

    DecodeFN*
    Dissector_Base::find_giop_decoder (gchar *opname)
    {
      DecodeFN *func = 0;
      if (op_functions_.find(opname, func) == 0)
        {
          return func;
        }
      ACE_DEBUG ((LM_DEBUG,"Unknown operation %s\n", opname));
      return 0;
    }

    void
    Dissector_Base::add_giop_decoder (const char *opname, DecodeFN decoder)
    {
      op_functions_.bind (opname, decoder);
    }

    void
    Dissector_Base::init_proto_label (const char *proto)
    {
      this->proto_label_ = new char [ACE_OS::strlen(proto)+1];
      ACE_OS::strcpy (this->proto_label_,proto);
    }

    void
    Dissector_Base::init_repo_id (const char *repo)
    {
      this->repo_id_ = new char [ACE_OS::strlen(repo)+1];
      ACE_OS::strcpy (this->repo_id_,repo);
    }


    //----------------------------------------------------------------------

    /*
      // Domain participant calls to notify of a new topic
      TopicStatus assert_topic (out RepoId topicId,
                                in ::DDS::DomainId_t domainId,
                                in RepoId  participantId,
                                in string          topicName,
                                in string          DataTypeName,
                                in ::DDS::TopicQos   qos)
        raises (Invalid_Domain,
                Invalid_Participant);
     */

    void
    InfoRepo_Dissector::assert_topic(::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          proto_tree_add_item (instance().tree,
                               hf_topicStatus,
                               instance().tvb, *instance().offset,
                               4, FALSE);
          *instance().offset += 4;

          guint len = 16; // size of RepoId
          if (len > instance().tvb->length - *instance().offset)
            {
              ACE_DEBUG ((LM_DEBUG, "buffer len should be 16, is (%d - %d)\n",
                          instance().tvb->length,*instance().offset));
              return;
            }
          const RepoId *rid =
            reinterpret_cast<const RepoId *>(tvb_get_ptr(instance().tvb,
                                                   *instance().offset,
                                                   len));
          RepoIdConverter converter (*rid);
          proto_tree_add_bytes_format_value
            (instance().tree,
             hf_topicId,
             instance().tvb, *instance().offset,
             len,
             reinterpret_cast<const guint8*>(rid),
             "%s",
             std::string(converter).c_str()
             );

          *instance().offset += len;
          break;
        }

      case Request:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Assert_Topic request\n"));
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Assert_Topic, msg type = %d\n",
                      header->message_type));
        }
      }

    }

    /*
    // publisher calls to create new publication
    // returns the id of the added publication
    // 0 is an invalid id
    RepoId add_publication (in ::DDS::DomainId_t domainId,
                            in RepoId participantId,
                            in RepoId topicId,
                            in DataWriterRemote publication,
                            in ::DDS::DataWriterQos qos,
                            in TransportInterfaceInfo transInfo,
                            in ::DDS::PublisherQos publisherQos)
    raises (Invalid_Domain,
            Invalid_Participant,
            Invalid_Topic);
    */

    void
    InfoRepo_Dissector::add_publication (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          guint len = 16; // size of RepoId
          if (len > instance().tvb->length - *instance().offset)
            {
              ACE_DEBUG ((LM_DEBUG, "buffer len should be 16, is (%d - %d)\n",
                          instance().tvb->length,*instance().offset));
              return;
            }
          const RepoId *rid =
            reinterpret_cast<const RepoId *>(tvb_get_ptr(instance().tvb,
                                                         *instance().offset,
                                                         len));
          RepoIdConverter converter (*rid);
          proto_tree_add_bytes_format_value
            (instance().tree,
             hf_addEntityRetn,
             instance().tvb, *instance().offset,
             len,
             reinterpret_cast<const guint8*>(rid),
             "%s",
             std::string(converter).c_str()
             );

          *instance().offset += len;

          break;
        }
      case Request:
        {
          int ofs = *instance().offset;
          gulong domain_id = ::get_CDR_ulong(instance().tvb, instance().offset,
                                             instance().is_big_endian_, 4);
          proto_tree_add_uint (instance().tree,
                               hf_domainId,
                               instance().tvb, ofs,
                               4, domain_id);

          guint len = 16; // size of RepoId
          {
            const RepoId *rid =
              reinterpret_cast<const RepoId *>(tvb_get_ptr(instance().tvb,
                                                           *instance().offset,
                                                           len));
            RepoIdConverter converter (*rid);
            proto_tree_add_bytes_format_value
              (instance().tree,
               hf_participantId,
               instance().tvb, *instance().offset,
               len,
               reinterpret_cast<const guint8*>(rid),
               "%s",
               std::string(converter).c_str()
               );
          }
          *instance().offset += len;

          {
            const RepoId *rid =
              reinterpret_cast<const RepoId *>(tvb_get_ptr(instance().tvb,
                                                           *instance().offset,
                                                           len));
            RepoIdConverter converter (*rid);
            proto_tree_add_bytes_format_value
              (instance().tree,
               hf_topicId,
               instance().tvb, *instance().offset,
               len,
               reinterpret_cast<const guint8*>(rid),
               "%s",
               std::string(converter).c_str()
               );
          }
          *instance().offset += len;


          ::get_CDR_object(instance().tvb,
                           instance().pinfo,
                           instance().tree,
                           instance().offset,
                           instance().is_big_endian_, 4);


          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Publication, msg type = %d\n",
                      header->message_type));
        }
      }
    }

    /*
      // DomainParticipantFactory calls to add a new domain participant
      // returns the id of the added participant and indication that the
      // repository is federated.
      // 0 is an invalid id
      AddDomainStatus add_domain_participant (in ::DDS::DomainId_t domain,
                                              in ::DDS::DomainParticipantQos qos)
        raises (Invalid_Domain);
    */
    void
    InfoRepo_Dissector::add_domain_participant (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Assert_Topic reply\n"));
          break;
        }
      case Request:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Assert_Topic request\n"));
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Assert_Topic, msg type = %d\n",
                      header->message_type));
        }
      }
    }

    void
    InfoRepo_Dissector::add_subscription (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Subscription reply\n"));
          break;
        }
      case Request:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Subscription request\n"));
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Subscription, msg type = %d\n",
                      header->message_type));
        }
      }
    }

    void
    InfoRepo_Dissector::remove_publication (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Publication reply\n"));
          break;
        }
      case Request:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Publication request\n"));
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Publication, msg type = %d\n",
                      header->message_type));
        }
      }
    }

    void
    InfoRepo_Dissector::remove_subscription (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Subscription reply\n"));
          break;
        }
      case Request:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Subscription request\n"));
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Subscription, msg type = %d\n",
                      header->message_type));
        }
      }
    }

    void
    InfoRepo_Dissector::remove_domain_participant (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Domain_Participant reply\n"));
          break;
        }
      case Request:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Domain_Participant request\n"));
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Domain_Participant, msg type = %d\n",
                      header->message_type));
        }
      }
    }

    extern "C"
    gboolean
    InfoRepo_Dissector::explicit_giop_callback
    (tvbuff_t *tvb, packet_info *pinfo,
     proto_tree *ptree, int *offset,
     ::MessageHeader *header, gchar *operation,
     gchar *idlname)
    {
      ACE_DEBUG ((LM_DEBUG,"pinfo.protocol = %s\n", pinfo->current_proto));
      if (idlname == 0)
        return FALSE;
      instance().setPacket (tvb, pinfo, ptree, offset);
      if (instance().dissect_giop (header, operation, idlname) == -1)
        return FALSE;
      return TRUE;
    }

    extern "C"
    gboolean
    InfoRepo_Dissector::heuristic_giop_callback
    (tvbuff_t *tvb, packet_info *pinfo,
     proto_tree *ptree, int *offset,
     ::MessageHeader *header, gchar *operation, gchar *)
    {

      ACE_UNUSED_ARG (tvb);
      //ACE_UNUSED_ARG (pinfo);
      ACE_UNUSED_ARG (ptree);
      ACE_UNUSED_ARG (offset);
      ACE_UNUSED_ARG (header);
      ACE_UNUSED_ARG (operation);

      ACE_DEBUG ((LM_DEBUG,"inforepo_heur pinfo.protocol = %s\n",
                  pinfo->current_proto));
      return FALSE;

    }

    void
    InfoRepo_Dissector::init ()
    {
      this->init_proto_label ("InfoRepo");
      this->init_repo_id ("OpenDDS/DCPS/DCPSInfo");

      this->proto_id_ = proto_register_protocol
        ("OpenDDS InfoRepo using GIOP",
         this->proto_label_,
         "inforepo-giop");
      this->ett_ = -1;
      static gint *ett[] = {
        &ett_
#if 0
        &ett_assert_topic,
        &ett_add_publication,
        &ett_remove_publication,
        &ett_add_subscription,
        &ett_remove_subscription,
        &ett_add_domain_participant,
        &ett_remove_domain_participant
#endif
      };

      static hf_register_info hf[] = {
        { &hf_topicId,
          { "topicId", "OpenDDS.DCPS.topicId",
            FT_BYTES, BASE_NONE, NULL, 0, NULL, HFILL
          }
        },
        { &hf_addEntityRetn,
          { "repoId_retn", "OpenDDS.DCPS.repoId.retn",
            FT_BYTES, BASE_NONE, NULL, 0, NULL, HFILL
          }
        },
        { &hf_domainId,
          { "domainId", "OpenDDS.DCPS.domain",
            FT_UINT32, BASE_DEC, NULL, 0, NULL, HFILL
          }
        },
        { &hf_participantId,
          { "participantId", "OpenDDS.DCPS.particiant",
            FT_BYTES, BASE_NONE, NULL, 0, NULL, HFILL
          }
        },

        {
          &hf_topicStatus,
          { "topicStatus", "OpenDDS.topicStatus",
            FT_UINT32, BASE_HEX, VALS(topic_status_vals), 0, NULL, HFILL
          }
        }
      };

      proto_register_field_array(this->proto_id_, hf, array_length(hf));
      proto_register_subtree_array(ett, array_length(ett));

      add_giop_decoder ("assert_topic", assert_topic);
      add_giop_decoder ("add_publication", add_publication);
      add_giop_decoder ("remove_publication", remove_publication);
      add_giop_decoder ("add_subscription", add_subscription);
      add_giop_decoder ("remove_subscription", remove_subscription);
      add_giop_decoder ("add_domain_participant", add_domain_participant);
      add_giop_decoder ("remove_domain_participant", remove_domain_participant);

#if 0
      add_giop_decoder ("attach_participant", attach_participant);
      add_giop_decoder ("find_topic", find_topic);
      add_giop_decoder ("remove_topic", remove_topic);
      add_giop_decoder ("enable_topic", enable_topic);
      add_giop_decoder ("disassociate_participant", disassociate_participant);
      add_giop_decoder ("disassociate_subscription", disassociate_subscription);
      add_giop_decoder ("disassociate_publication", disassociate_publication);
      add_giop_decoder ("ignore_domain_participant", ignore_domain_participant);
      add_giop_decoder ("ignore_topic", ignore_topic);
      add_giop_decoder ("ignore_subscription", ignore_subscription);
      add_giop_decoder ("ignore_publication", ignore_publication);
      add_giop_decoder ("update_domain_participant_qos", update_domain_participant_qos);
      add_giop_decoder ("update_topic_qos", update_topic_qos);
      add_giop_decoder ("update_publication_qos", update_publication_qos);
      add_giop_decoder ("update_subscription_qos", update_subscription_qos);
      add_giop_decoder ("update_subscription_params", update_subscription_params);
      add_giop_decoder ("shutdown", shutdown);
#endif
    }

    void
    InfoRepo_Dissector::register_handoff ()
    {
      register_giop_user_module(explicit_giop_callback,
                                this->proto_label_,
                                this->repo_id_,
                                this->proto_id_);

      register_giop_user(heuristic_giop_callback,
                         this->proto_label_,
                         this->proto_id_);

    }

    //---------------------------------------------------------------

    void
    DataWriterRemote_Dissector::add_associations (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Associations reply\n"));
          break;
        }
      case Request:
        {
          // parse reply
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Associations request\n"));
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Associations, msg type = %d\n",
                      header->message_type));
        }
      }

    }

    void
    DataWriterRemote_Dissector::init ()
    {
      this->init_proto_label ("DataWriter");
      this->init_repo_id ("OpenDDS/DCPS/DataWriterRemote");
      this->proto_id_ = proto_register_protocol
        ("OpenDDS DataWriterRemote using GIOP",
         this->proto_label_,
         "datawriter-giop");
      this->ett_ = -1;
      static gint *ett[] = {
        &ett_
      };

      proto_register_subtree_array(ett, array_length(ett));

      add_giop_decoder ("add_associations", add_associations);

#if 0
      add_giop_decoder ("remove_associations", remove_associations);
      add_giop_decoder ("update_incompatible_qos", update_incompatible_qos);
      add_giop_decoder ("update_subscription_params", update_subscription_params);
#endif
    }


    extern "C"
    gboolean
    DataWriterRemote_Dissector::explicit_giop_callback
    (tvbuff_t *tvb, packet_info *pinfo,
     proto_tree *ptree, int *offset,
     ::MessageHeader *header, gchar *operation,
     gchar *idlname)
    {
      ACE_DEBUG ((LM_DEBUG,"pinfo.protocol = %s\n", pinfo->current_proto));
      if (idlname == 0)
        return FALSE;
      instance().setPacket (tvb, pinfo, ptree, offset);
      if (instance().dissect_giop (header, operation, idlname) == -1)
        return FALSE;
      return TRUE;
    }

    extern "C"
    gboolean
    DataWriterRemote_Dissector::heuristic_giop_callback
    (tvbuff_t *tvb, packet_info *pinfo,
     proto_tree *ptree, int *offset,
     ::MessageHeader *header, gchar *operation, gchar *)
    {
      ACE_UNUSED_ARG (tvb);
      //ACE_UNUSED_ARG (pinfo);
      ACE_UNUSED_ARG (ptree);
      ACE_UNUSED_ARG (offset);
      ACE_UNUSED_ARG (header);
      ACE_UNUSED_ARG (operation);

      ACE_DEBUG ((LM_DEBUG,"DWR_heur pinfo.protocol = %s\n",
                  pinfo->current_proto));
      return FALSE;

    }

    void
    DataWriterRemote_Dissector::register_handoff ()
    {
      register_giop_user_module(explicit_giop_callback,
                                proto_label_,
                                repo_id_,
                                proto_id_);

      register_giop_user(heuristic_giop_callback,
                         proto_label_, proto_id_);


    }


  }
}
