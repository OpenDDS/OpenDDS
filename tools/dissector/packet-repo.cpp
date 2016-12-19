/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tools/dissector/packet-repo.h"
#include "dds/DdsDcpsInfoUtilsC.h"

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

} // namespace


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {

    // static member values
    InfoRepo_Dissector InfoRepo_Dissector::instance_;
    bool InfoRepo_Dissector::initialized_ = false;

    int InfoRepo_Dissector::ett_ior_ = -1;
    int InfoRepo_Dissector::ett_topic_qos_ = -1;
    int InfoRepo_Dissector::ett_domain_qos_ = -1;
    int InfoRepo_Dissector::ett_writer_qos_ = -1;
    int InfoRepo_Dissector::ett_trans_info_ = -1;
    int InfoRepo_Dissector::ett_publisher_qos_ = -1;

    int InfoRepo_Dissector::hf_topicId = -1;
    int InfoRepo_Dissector::hf_topicStatus = -1;

    int InfoRepo_Dissector::hf_domainId = -1;
    int InfoRepo_Dissector::hf_participantId = -1;
    int InfoRepo_Dissector::hf_topicName = -1;
    int InfoRepo_Dissector::hf_transInfo = -1;
    int InfoRepo_Dissector::hf_dataTypeName = -1;
    int InfoRepo_Dissector::hf_topicQos = -1;
    int InfoRepo_Dissector::hf_qos = -1;

    int InfoRepo_Dissector::hf_addEntityRetn = -1;
    int InfoRepo_Dissector::hf_exception = -1;



    void
    InfoRepo_Dissector::init ()
    {

      GIOP_Base::init ();

      this->init_proto_label ("InfoRepo");
      this->init_repo_id ("OpenDDS/DCPS/DCPSInfo");

      this->proto_id_ = proto_register_protocol
        ("OpenDDS InfoRepo using GIOP",
         this->proto_label_,
         "inforepo-giop");

      if (initialized_)
        return;

      initialized_ = true;

      static gint *ett[] = {
        &ett_ior_,
        &ett_topic_qos_,
        &ett_domain_qos_,
        &ett_writer_qos_,
        &ett_trans_info_,
        &ett_publisher_qos_
      };

      proto_register_subtree_array(ett, array_length(ett));

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

        { &hf_topicName,
          { "topicName", "OpenDDS.DCPS.topicName",
            FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL
          }
        },

        { &hf_dataTypeName,
          { "dataType", "OpenDDS.DCPS.dataTypeName",
            FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL
          }
        },

        { &hf_topicQos,
          { "generic qos", "OpenDDS.DCPS.QOS",
            FT_BYTES, BASE_NONE, NULL, 0, NULL, HFILL
          }
        },

        { &hf_transInfo,
          { "trans info", "OpenDDS.DDS.TransportInfo",
            FT_BYTES, BASE_NONE, NULL, 0, NULL, HFILL
          }
        },

        { &hf_qos,
          { "generic qos", "OpenDDS.DCPS.QOS",
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

    InfoRepo_Dissector&
    InfoRepo_Dissector::instance()
    {
      return instance_;
    }

    const char *
    InfoRepo_Dissector::topic_for_pub (const RepoId *pub)
    {
      gulong key = ACE::hash_pjw(reinterpret_cast<const char *>(pub),
                                 sizeof (RepoId));
      const RepoId *topicId = 0;
      if (publications_.find (key,topicId) != 0)
        {
          return 0;
        }
      const char *data_name = 0;
      key = ACE::hash_pjw(reinterpret_cast<const char *>(topicId),
                          sizeof (RepoId));
      topics_.find (key, data_name);
      return data_name;
    }

    void
    InfoRepo_Dissector::add_pending (int request_id, const char *dataName)
    {
      Pending *pt = new Pending;
      pt->conv_ = this->find_conversation ();
      pt->request_ = request_id;
      pt->data_name_ = new char[ACE_OS::strlen(dataName)+1];
      pt->next_ = 0;

      ACE_OS::strcpy (pt->data_name_, dataName);

      if (this->pending_ == 0)
        {
          this->pending_ = pt;
          return;
        }
      Pending *node = this->pending_;
      while (node->next_ != 0)
        node = node->next_;
      node->next_ = pt;
    }

    void
    InfoRepo_Dissector::add_pending (int request_id, const RepoId *topic)
    {
      Pending *pt = new Pending;
      pt->conv_ = this->find_conversation ();
      pt->request_ = request_id;
      pt->topic_id_ = new RepoId;
      pt->next_ = 0;

      ACE_OS::memcpy (pt->topic_id_, topic, sizeof(RepoId));

      if (this->pending_ == 0)
        {
          this->pending_ = pt;
          return;
        }
      Pending *node = this->pending_;
      while (node->next_ != 0)
        node = node->next_;
      node->next_ = pt;
    }

    void
    InfoRepo_Dissector::map_pending (Pending &pt, const RepoId *rid)
    {
      Pending *prev_node = 0;
      for (Pending *node = this->pending_;
           node != 0;
           node = node->next_)
        {
          if (pt.conv_->index == node->conv_->index &&
              pt.request_ == node->request_)
            {
              gulong hash_rid = ACE::hash_pjw(reinterpret_cast<const char *>(rid), sizeof (RepoId));
              if (node->data_name_ != 0)
                {
                  this->topics_.bind (hash_rid, node->data_name_);
                  node->data_name_ = 0;
                }
              else
                {
                  this->publications_.bind (hash_rid, node->topic_id_);
                  node->topic_id_ = 0;
                }
              if (prev_node == 0)
                {
                  this->pending_ = node->next_;
                }
              else
                {
                  prev_node->next_ = node->next_;
                }
              delete node;
              return;
            }
        }
      ACE_DEBUG ((LM_DEBUG,"map_pending could not find pending topic\n"));
    }

    void
    InfoRepo_Dissector::discard_pending (Pending &pt)
    {
      Pending *prev_node = 0;
      for (Pending *node = this->pending_;
           node != 0;
           node = node->next_)
        {
          if (pt.conv_->index == node->conv_->index &&
              pt.request_ == node->request_)
            {
              if (prev_node == 0)
                {
                  this->pending_ = node->next_;
                }
              else
                {
                  prev_node->next_ = node->next_;
                }
              delete node;
              return;
            }
        }
    }


    /*
      // Domain participant calls to notify of a new topic
      TopicStatus assert_topic (out RepoId topicId,
                                in ::DDS::DomainId_t domainId,
                                in RepoId participantId,
                                in string topicName,
                                in string DataTypeName,
                                in ::DDS::TopicQos qos)
        raises (Invalid_Domain,
                Invalid_Participant);
     */

    bool
    InfoRepo_Dissector::assert_topic(::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          Pending pt;
          pt.conv_ = instance().find_conversation();
          pt.request_ = header->req_id;
          switch (header->rep_status) {
          case NO_EXCEPTION:
            {
              TopicStatus status = instance().add_topic_status (hf_topicStatus);
              const RepoId *rid = instance().add_repo_id (hf_topicId);

              if (instance().pinfo_->fd->flags.visited)
                {
                  return true;
                }

              if (status == CREATED || status == ENABLED)
                {
                  instance().map_pending (pt,rid);
                  return true;
                }
            }
            break;
          case USER_EXCEPTION:
            instance().add_exception (hf_exception,
                                      header->exception_id);
            break;
          case SYSTEM_EXCEPTION:
            break;
          default:;
          }

          instance().discard_pending (pt);
          break;
        }
      case Request:
        {
          instance().add_ulong (hf_domainId);
          instance().add_repo_id (hf_participantId);
          instance().add_string (hf_topicName);
          const char * dname = instance().add_string (hf_dataTypeName);
//           instance().add_topic_qos (hf_qos, instance().ett_topic_qos_);
          if (instance().pinfo_->fd->flags.visited)
            {
              return true;
            }

          instance().add_pending (header->req_id, dname);

          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Assert_Topic, msg type = %d\n",
                      header->message_type));
        }
      }

      return true;
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

    bool
    InfoRepo_Dissector::add_publication (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          Pending pp;
          pp.conv_ = instance().find_conversation();
          pp.request_ = header->req_id;
          const RepoId *rid = instance().add_repo_id (hf_addEntityRetn);
          instance().map_pending (pp, rid);
          break;
        }
      case Request:
        {
          instance().add_ulong (hf_domainId);
          instance().add_repo_id (hf_participantId);
          const RepoId *topic_id = instance().add_repo_id (hf_topicId);
          // this is necessary to get the objectId registered
          ::get_CDR_object(instance().tvb_,
                           instance().pinfo_,
                           instance().tree_,
                           instance().offset_,
                           instance().is_big_endian_, 4);
          //instance().add_writer_qos (hf_qos,
          //                           instance().ett_writer_qos_);
          //instance().add_trans_info (hf_transInfo,
          //                           instance().ett_trans_info_);
          //instance().add_publisher_qos (hf_qos,
          //                              instance().ett_publisher_qos_);

          instance().add_pending (header->req_id, topic_id);

          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Publication, msg type = %d\n",
                      header->message_type));
        }
      }

      return true;
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
    bool
    InfoRepo_Dissector::add_domain_participant (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          break;
        }
      case Request:
        {
          instance().add_ulong (hf_domainId);
          instance().add_domain_qos (hf_qos,
                                     instance().ett_domain_qos_);

          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Domain_Participant, msg type = %d\n",
                      header->message_type));
        }

      }

      return true;
    }

    bool
    InfoRepo_Dissector::add_subscription (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          break;
        }
      case Request:
        {
          // parse reply
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Add_Subscription, msg type = %d\n",
                      header->message_type));
        }
      }

      return true;
   }

    bool
    InfoRepo_Dissector::remove_publication (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          break;
        }
      case Request:
        {
          // parse reply
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Publication, msg type = %d\n",
                      header->message_type));
        }
      }

      return true;
    }

    bool
    InfoRepo_Dissector::remove_subscription (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          break;
        }
      case Request:
        {
          // parse reply
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Subscription, msg type = %d\n",
                      header->message_type));
        }
      }

      return true;
    }

    bool
    InfoRepo_Dissector::remove_domain_participant (::MessageHeader *header)
    {
      instance().start_decoding ();
      switch (header->message_type) {
      case Reply:
        {
          // parse reply
          break;
        }
      case Request:
        {
          // parse reply
          break;
        }
      default:
        {
          ACE_DEBUG ((LM_DEBUG,"Decoding Remove_Domain_Participant, msg type = %d\n",
                      header->message_type));
        }
      }

      return true;
    }

    extern "C"
    gboolean
    explicit_inforepo_callback
    (tvbuff_t *tvb, packet_info *pinfo,
     proto_tree *ptree, int *offset,
     ::MessageHeader *header, WS_CONST gchar *operation,
     gchar *idlname)
    {
      InfoRepo_Dissector &dissector =
        InfoRepo_Dissector::instance();

      if (idlname == 0)
        return FALSE;
      dissector.setPacket (tvb, pinfo, ptree, offset);
      dissector.fix_reqid(header);
      if (dissector.dissect_giop (header, operation, idlname) == -1)
        return FALSE;
      return TRUE;
    }

    extern "C"
    gboolean
    heuristic_inforepo_callback
    (tvbuff_t *tvb, packet_info *pinfo,
     proto_tree *ptree, int *offset,
     ::MessageHeader *header, WS_CONST gchar *operation, gchar *)
    {
      InfoRepo_Dissector &dissector =
        InfoRepo_Dissector::instance();

      dissector.setPacket (tvb, pinfo, ptree, offset);
      dissector.fix_reqid(header);

      return dissector.dissect_heur (header, operation);
    }

    void
    InfoRepo_Dissector::register_handoff ()
    {
      register_giop_user_module(explicit_inforepo_callback,
                                this->proto_label_,
                                this->repo_id_,
                                this->proto_id_);

      register_giop_user(heuristic_inforepo_callback,
                         this->proto_label_,
                         this->proto_id_);

    }

  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
