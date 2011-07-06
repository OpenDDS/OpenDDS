/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tools/dissector/giop_base.h"


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

namespace OpenDDS
{
  namespace DCPS
  {

    bool GIOP_Base::initialized_ = false;
    int GIOP_Base::ett_base_ = -1;
    int GIOP_Base::hf_ = -1;

    void
    GIOP_Base::init ()
    {
      if (initialized_)
        return;

      static gint *ett[] = {
        &ett_base_
      };

      proto_register_subtree_array(ett, array_length(ett));

      initialized_ = true;
    }

    int
    GIOP_Base::dissect_giop (::MessageHeader *header,
                             gchar *operation,
                             gchar *idlname)
    {
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
    GIOP_Base::start_decoding()
    {
      proto_item *ti = 0;
      proto_tree *subtree = 0;
      col_set_str (pinfo_->cinfo, COL_PROTOCOL, this->proto_label_);
      if (tree_) {
        ti = proto_tree_add_item(tree_,
                                 this->proto_id_,
                                 tvb_, *offset_, -1, 0);
        subtree = proto_item_add_subtree (ti, this->ett_base_);
        this->tree_ = subtree;
      }
    }

    DecodeFN*
    GIOP_Base::find_giop_decoder (gchar *opname)
    {
      DecodeFN *func = 0;
      if (op_functions_.find(opname, func) == 0)
        {
          return func;
        }
      ACE_DEBUG ((LM_DEBUG,
                  "GIOP_Base::find_giop_decoder: Unknown operation %s\n", 
                  opname));
      return 0;
    }

    void
    GIOP_Base::add_giop_decoder (const char *opname, DecodeFN decoder)
    {
      op_functions_.bind (opname, decoder);
    }

    void
    GIOP_Base::init_proto_label (const char *proto)
    {
      this->proto_label_ = new char [ACE_OS::strlen(proto)+1];
      ACE_OS::strcpy (this->proto_label_,proto);
    }

    void
    GIOP_Base::init_repo_id (const char *repo)
    {
      this->repo_id_ = new char [ACE_OS::strlen(repo)+1];
      ACE_OS::strcpy (this->repo_id_,repo);
    }


    // dissection helpers

    conversation_t *
    GIOP_Base::find_conversation()
    {
      return ::find_conversation (pinfo_->fd->num,
                                  &pinfo_->src, &pinfo_->dst,
                                  pinfo_->ptype, pinfo_->srcport,
                                  pinfo_->destport, 0);
    }

    void
    GIOP_Base::add_ulong (int fieldId, proto_tree *subtree)
    {
      proto_tree *tree = subtree != 0 ? subtree : this->tree_;
      int ofs = *offset_;
      int len = sizeof (CORBA::ULong);
      int boundary = 4;
      gulong domain_id = ::get_CDR_ulong(tvb_, offset_, is_big_endian_, boundary);
      proto_tree_add_uint (tree, fieldId, tvb_, ofs, len, domain_id);
    }

    const RepoId *
    GIOP_Base::add_repo_id (int fieldId, proto_tree *subtree)
    {
      proto_tree *tree = subtree != 0 ? subtree : this->tree_;
      guint len = 16; // size of RepoId
      const RepoId *rid =
        reinterpret_cast<const RepoId *>(tvb_get_ptr(tvb_, *offset_, len));
      RepoIdConverter converter (*rid);
      proto_tree_add_bytes_format_value
        ( tree, fieldId, tvb_, *offset_, len,
          reinterpret_cast<const guint8*>(rid),
          "%s", std::string(converter).c_str() );
      *offset_ += len;
      return rid;
    }

    const char *
    GIOP_Base::add_string (int fieldId, proto_tree *subtree)
    {
      proto_tree *tree = subtree != 0 ? subtree : this->tree_;
      gchar *strbuf = 0;
      int slen = ::get_CDR_string(tvb_,&strbuf, offset_, is_big_endian_, 4);
      proto_tree_add_string (tree, fieldId, tvb_, *offset_-slen, slen, strbuf);
      return strbuf;
    }


    TopicStatus
    GIOP_Base::add_topic_status (int fieldId, proto_tree *subtree)
    {
      proto_tree *tree = subtree != 0 ? subtree : this->tree_;
      int len = sizeof (TopicStatus);
      guint32 ts = ::get_CDR_enum (tvb_,offset_, is_big_endian_, 4);
      proto_tree_add_item (tree, fieldId, tvb_, *offset_-len, len, ts);
      return static_cast<TopicStatus>(ts);
    }

    void
    GIOP_Base::add_exception (int fieldId, const char *exceptionId)
    {
      proto_tree_add_string_format_value(tree_, fieldId, tvb_, *offset_, -1,
                                         "", "Exception %s",exceptionId );
    }

    void
    GIOP_Base::add_trans_info (int fieldId, int ett, proto_tree *subtree)
    {
      proto_tree *tree = subtree != 0 ? subtree : this->tree_;
      proto_item *tf = proto_tree_add_text (tree, tvb_, *offset_, -1,
                                            "TransInfo");
      tree = proto_item_add_subtree (tf, ett);
      /*
        struct TransportInterfaceInfo {
        /// The transport type (.e.g SimpleTcp or udp)
        TransportInterfaceId   transport_id;
        /// Informationn about the transport instance that is opaque to all but
        /// code specific to that transport implementation.
        TransportInterfaceBLOB data;

        /// This encodes the TRANSPORT_PRIORITY DataWriter Qos policy value
        /// for publications.  The value is set to 0 for subscriptions.
        long publication_transport_priority;
        };
      */

      add_ulong (fieldId,tree);

      //add_octetseq ();
      //add_ulong ();
    }

    void
    GIOP_Base::add_topic_qos (int fieldId, int ett, proto_tree *subtree)
    {
      /*
    struct TopicQos {
                TopicDataQosPolicy topic_data;
                DurabilityQosPolicy durability;
                DurabilityServiceQosPolicy durability_service;
                DeadlineQosPolicy deadline;
                LatencyBudgetQosPolicy latency_budget;
                LivelinessQosPolicy liveliness;
                ReliabilityQosPolicy reliability;
                DestinationOrderQosPolicy destination_order;
                HistoryQosPolicy history;
                ResourceLimitsQosPolicy resource_limits;
                TransportPriorityQosPolicy transport_priority;
                LifespanQosPolicy lifespan;
                OwnershipQosPolicy ownership;
                };

       */
      proto_tree *tree = subtree != 0 ? subtree : this->tree_;
      proto_item *tf = proto_tree_add_text (tree, tvb_, *offset_, -1,
                                            "TopicQos");
      tree = proto_item_add_subtree (tf, ett);

      int len = sizeof (TopicStatus);
      proto_tree_add_item (tree, fieldId, tvb_, *offset_, len, FALSE);
      *offset_ += len;
    }

    void
    GIOP_Base::add_domain_qos (int fieldId, int ett, proto_tree *subtree)
    {
//       struct DomainParticipantQos {
//         UserDataQosPolicy user_data;
//         EntityFactoryQosPolicy entity_factory;
//       };

//       struct UserDataQosPolicy {
//         octetseq value;
//       };

//       struct EntityFactoryQosPolicy {
//         boolean autoenable_created_entities;
//       };

      proto_tree *tree = subtree != 0 ? subtree : this->tree_;

      proto_item *ti = proto_tree_add_item(tree, fieldId,
                                           tvb_, *offset_, -1, 0);
      tree = proto_item_add_subtree (ti, ett);

    }

    void
    GIOP_Base::add_publisher_qos (int fieldId, int ett, proto_tree *subtree)
    {
//       struct PublisherQos {
//         PresentationQosPolicy presentation;
//         PartitionQosPolicy partition;
//         GroupDataQosPolicy group_data;
//         EntityFactoryQosPolicy entity_factory;
//       };

//       enum PresentationQosPolicyAccessScopeKind {
//         INSTANCE_PRESENTATION_QOS,
//         TOPIC_PRESENTATION_QOS,
//         GROUP_PRESENTATION_QOS
//       };

//       struct PresentationQosPolicy {
//         PresentationQosPolicyAccessScopeKind access_scope;
//         boolean coherent_access;
//         boolean ordered_access;
//       };

//       struct GroupDataQosPolicy {
//         OctetSeq value;
//       };

//       struct EntityFactoryQosPolicy {
//         boolean autoenable_created_entities;
//       };


      proto_tree *tree = subtree != 0 ? subtree : this->tree_;

      proto_item *ti = 0;
      ti = proto_tree_add_item(tree, fieldId, tvb_, *offset_, -1, 0);
      tree = proto_item_add_subtree (ti, ett);
    }

    void
    GIOP_Base::add_writer_qos (int fieldId, int ett, proto_tree *subtree)
    {
      proto_tree *tree = subtree != 0 ? subtree : this->tree_;

      proto_item *ti = 0;
      ti = proto_tree_add_item(tree, fieldId, tvb_, *offset_, -1, 0);
      tree = proto_item_add_subtree (ti, ett);

    }

  } // namespace DCPS
} // namespace OpenDDS
