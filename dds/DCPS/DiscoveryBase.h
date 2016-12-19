/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DDS_DCPS_DISCOVERYBASE_H
#define OPENDDS_DDS_DCPS_DISCOVERYBASE_H

#include "dds/DCPS/Discovery.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/DCPS_Utils.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DCPS/DataReaderImpl_T.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include "ace/Select_Reactor.h"
#include "ace/Condition_Thread_Mutex.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace DCPS {
    typedef DataReaderImpl_T<DDS::ParticipantBuiltinTopicData> ParticipantBuiltinTopicDataDataReaderImpl;
    typedef DataReaderImpl_T<DDS::PublicationBuiltinTopicData> PublicationBuiltinTopicDataDataReaderImpl;
    typedef DataReaderImpl_T<DDS::SubscriptionBuiltinTopicData> SubscriptionBuiltinTopicDataDataReaderImpl;
    typedef DataReaderImpl_T<DDS::TopicBuiltinTopicData> TopicBuiltinTopicDataDataReaderImpl;

    inline void assign(DCPS::EntityKey_t& lhs, unsigned int rhs)
    {
      lhs[0] = static_cast<CORBA::Octet>(rhs);
      lhs[1] = static_cast<CORBA::Octet>(rhs >> 8);
      lhs[2] = static_cast<CORBA::Octet>(rhs >> 16);
    }

    struct DcpsUpcalls : ACE_Task_Base {
      DcpsUpcalls(DCPS::DataReaderCallbacks* drr,
                  const RepoId& reader,
                  const DCPS::WriterAssociation& wa,
                  bool active,
                  DCPS::DataWriterCallbacks* dwr)
        : drr_(drr), reader_(reader), wa_(wa), active_(active), dwr_(dwr)
        , reader_done_(false), writer_done_(false), cnd_(mtx_)
      {}

      int svc()
      {
        drr_->add_association(reader_, wa_, active_);
        {
          ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mtx_, -1);
          reader_done_ = true;
          cnd_.signal();
          while (!writer_done_) {
            cnd_.wait();
          }
        }
        dwr_->association_complete(reader_);
        return 0;
      }

      void writer_done()
      {
        {
          ACE_GUARD(ACE_Thread_Mutex, g, mtx_);
          writer_done_ = true;
          cnd_.signal();
        }
        wait();
      }

      DCPS::DataReaderCallbacks* const drr_;
      const RepoId& reader_;
      const DCPS::WriterAssociation& wa_;
      bool active_;
      DCPS::DataWriterCallbacks* const dwr_;
      bool reader_done_, writer_done_;
      ACE_Thread_Mutex mtx_;
      ACE_Condition_Thread_Mutex cnd_;
    };

    template <typename DiscoveredParticipantData_>
    class EndpointManager {
    protected:
      struct DiscoveredSubscription {
        DiscoveredSubscription() : bit_ih_(DDS::HANDLE_NIL) {}
        explicit DiscoveredSubscription(const OpenDDS::DCPS::DiscoveredReaderData& r)
          : reader_data_(r), bit_ih_(DDS::HANDLE_NIL) {}
        OpenDDS::DCPS::DiscoveredReaderData reader_data_;
        DDS::InstanceHandle_t bit_ih_;
      };
      typedef OPENDDS_MAP_CMP(DCPS::RepoId, DiscoveredSubscription,
                              DCPS::GUID_tKeyLessThan) DiscoveredSubscriptionMap;
      typedef typename DiscoveredSubscriptionMap::iterator DiscoveredSubscriptionIter;

      struct DiscoveredPublication {
        DiscoveredPublication() : bit_ih_(DDS::HANDLE_NIL) {}
        explicit DiscoveredPublication(const OpenDDS::DCPS::DiscoveredWriterData& w)
          : writer_data_(w), bit_ih_(DDS::HANDLE_NIL) {}
        OpenDDS::DCPS::DiscoveredWriterData writer_data_;
        DDS::InstanceHandle_t bit_ih_;
      };

      typedef OPENDDS_MAP_CMP(DCPS::RepoId, DiscoveredPublication,
                              DCPS::GUID_tKeyLessThan) DiscoveredPublicationMap;
      typedef typename DiscoveredPublicationMap::iterator DiscoveredPublicationIter;

    public:
      typedef DiscoveredParticipantData_ DiscoveredParticipantData;

      struct TopicDetails {
        OPENDDS_STRING data_type_;
        DDS::TopicQos qos_;
        DCPS::RepoId repo_id_;
        bool has_dcps_key_;
        RepoIdSet endpoints_;
      };

      EndpointManager(const RepoId& participant_id, ACE_Thread_Mutex& lock)
        : lock_(lock)
        , participant_id_(participant_id)
        , publication_counter_(0)
        , subscription_counter_(0)
        , topic_counter_(0)
      { }

      virtual ~EndpointManager() { }

      RepoId bit_key_to_repo_id(const char* bit_topic_name,
                                const DDS::BuiltinTopicKey_t& key)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, RepoId());
        if (0 == std::strcmp(bit_topic_name, DCPS::BUILT_IN_PUBLICATION_TOPIC)) {
          return pub_key_to_id_[key];
        }
        if (0 == std::strcmp(bit_topic_name, DCPS::BUILT_IN_SUBSCRIPTION_TOPIC)) {
          return sub_key_to_id_[key];
        }
        return RepoId();
      }

      void ignore(const DCPS::RepoId& to_ignore)
      {
        // Locked prior to call from Spdp.
        ignored_guids_.insert(to_ignore);
        {
          const DiscoveredPublicationIter iter =
            discovered_publications_.find(to_ignore);
          if (iter != discovered_publications_.end()) {
            // clean up tracking info
            topics_[get_topic_name(iter->second)].endpoints_.erase(iter->first);
            remove_from_bit(iter->second);
            OPENDDS_STRING topic_name = get_topic_name(iter->second);
            discovered_publications_.erase(iter);
            // break associations
            typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
              topics_.find(topic_name);
            if (top_it != topics_.end()) {
              match_endpoints(to_ignore, top_it->second, true /*remove*/);
            }
            return;
          }
        }
        {
          const DiscoveredSubscriptionIter iter =
            discovered_subscriptions_.find(to_ignore);
          if (iter != discovered_subscriptions_.end()) {
            // clean up tracking info
            topics_[get_topic_name(iter->second)].endpoints_.erase(iter->first);
            remove_from_bit(iter->second);
            OPENDDS_STRING topic_name = get_topic_name(iter->second);
            discovered_subscriptions_.erase(iter);
            // break associations
            typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
              topics_.find(topic_name);
            if (top_it != topics_.end()) {
              match_endpoints(to_ignore, top_it->second, true /*remove*/);
            }
            return;
          }
        }
        {
          const OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, DCPS::GUID_tKeyLessThan)::iterator
            iter = topic_names_.find(to_ignore);
          if (iter != topic_names_.end()) {
            ignored_topics_.insert(iter->second);
            // Remove all publications and subscriptions on this topic
            typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
              topics_.find(iter->second);
            if (top_it != topics_.end()) {
              TopicDetails& td = top_it->second;
              RepoIdSet::iterator ep;
              for (ep = td.endpoints_.begin(); ep!= td.endpoints_.end(); ++ep) {
                match_endpoints(*ep, td, true /*remove*/);
                if (shutting_down()) { return; }
              }
            }
          }
        }
      }

      bool ignoring(const DCPS::RepoId& guid) const {
        return ignored_guids_.count(guid);
      }
      bool ignoring(const char* topic_name) const {
        return ignored_topics_.count(topic_name);
      }

      DCPS::TopicStatus assert_topic(DCPS::RepoId_out topicId, const char* topicName,
                                     const char* dataTypeName, const DDS::TopicQos& qos,
                                     bool hasDcpsKey)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
        typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator iter =
          topics_.find(topicName);
        if (iter != topics_.end()) { // types must match, RtpsDiscovery checked for us
          iter->second.qos_ = qos;
          iter->second.has_dcps_key_ = hasDcpsKey;
          topicId = iter->second.repo_id_;
          topic_names_[iter->second.repo_id_] = topicName;
          return DCPS::FOUND;
        }

        TopicDetails& td = topics_[topicName];
        td.data_type_ = dataTypeName;
        td.qos_ = qos;
        td.has_dcps_key_ = hasDcpsKey;
        td.repo_id_ = make_topic_guid();
        topicId = td.repo_id_;
        topic_names_[td.repo_id_] = topicName;

        return DCPS::CREATED;
      }

      DCPS::TopicStatus remove_topic(const RepoId& topicId, OPENDDS_STRING& name)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
        name = topic_names_[topicId];
        typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
          topics_.find(name);
        if (top_it != topics_.end()) {
          TopicDetails& td = top_it->second;
          if (td.endpoints_.empty()) {
            topics_.erase(name);
          }
        }

        topic_names_.erase(topicId);
        return DCPS::REMOVED;
      }

      virtual bool update_topic_qos(const DCPS::RepoId& topicId, const DDS::TopicQos& qos,
                                    OPENDDS_STRING& name) = 0;

      DCPS::RepoId add_publication(const DCPS::RepoId& topicId,
                                   DCPS::DataWriterCallbacks* publication,
                                   const DDS::DataWriterQos& qos,
                                   const DCPS::TransportLocatorSeq& transInfo,
                                   const DDS::PublisherQos& publisherQos)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, RepoId());
        RepoId rid = participant_id_;
        assign_publication_key(rid, topicId, qos);
        LocalPublication& pb = local_publications_[rid];
        pb.topic_id_ = topicId;
        pb.publication_ = publication;
        pb.qos_ = qos;
        pb.trans_info_ = transInfo;
        pb.publisher_qos_ = publisherQos;
        TopicDetails& td = topics_[topic_names_[topicId]];
        td.endpoints_.insert(rid);

        if (DDS::RETCODE_OK != add_publication_i(rid, pb)) {
          return RepoId();
        }

        if (DDS::RETCODE_OK != write_publication_data(rid, pb)) {
          return RepoId();
        }

        if (DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::add_publication - ")
                     ACE_TEXT("calling match_endpoints\n")));
        }
        match_endpoints(rid, td);

        return rid;
      }

      void remove_publication(const DCPS::RepoId& publicationId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        LocalPublicationIter iter = local_publications_.find(publicationId);
        if (iter != local_publications_.end()) {
          if (DDS::RETCODE_OK == remove_publication_i(publicationId))
            {
              OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
              local_publications_.erase(publicationId);
              typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
                topics_.find(topic_name);
              if (top_it != topics_.end()) {
                match_endpoints(publicationId, top_it->second, true /*remove*/);
                top_it->second.endpoints_.erase(publicationId);
              }
            } else {
            ACE_DEBUG((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: EndpointManager::remove_publication - ")
                       ACE_TEXT("Failed to publish dispose msg\n")));
          }
        }
      }

      virtual bool update_publication_qos(const DCPS::RepoId& publicationId,
                                          const DDS::DataWriterQos& qos,
                                          const DDS::PublisherQos& publisherQos) = 0;

      DCPS::RepoId add_subscription(const DCPS::RepoId& topicId,
                                    DCPS::DataReaderCallbacks* subscription,
                                    const DDS::DataReaderQos& qos,
                                    const DCPS::TransportLocatorSeq& transInfo,
                                    const DDS::SubscriberQos& subscriberQos,
                                    const char* filterClassName,
                                    const char* filterExpr,
                                    const DDS::StringSeq& params)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, RepoId());
        RepoId rid = participant_id_;
        assign_subscription_key(rid, topicId, qos);
        LocalSubscription& sb = local_subscriptions_[rid];
        sb.topic_id_ = topicId;
        sb.subscription_ = subscription;
        sb.qos_ = qos;
        sb.trans_info_ = transInfo;
        sb.subscriber_qos_ = subscriberQos;
        sb.filterProperties.filterClassName = filterClassName;
        sb.filterProperties.filterExpression = filterExpr;
        sb.filterProperties.expressionParameters = params;

        TopicDetails& td = topics_[topic_names_[topicId]];
        td.endpoints_.insert(rid);

        if (DDS::RETCODE_OK != add_subscription_i(rid, sb)) {
          return RepoId();
        }

        if (DDS::RETCODE_OK != write_subscription_data(rid, sb)) {
          return RepoId();
        }

        if (DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::add_subscription - ")
                     ACE_TEXT("calling match_endpoints\n")));
        }
        match_endpoints(rid, td);

        return rid;
      }

      void remove_subscription(const DCPS::RepoId& subscriptionId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
        if (iter != local_subscriptions_.end()) {
          if (DDS::RETCODE_OK == remove_subscription_i(subscriptionId)
              /*subscriptions_writer_.write_unregister_dispose(subscriptionId)*/) {
            OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
            local_subscriptions_.erase(subscriptionId);
            typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
              topics_.find(topic_name);
            if (top_it != topics_.end()) {
              match_endpoints(subscriptionId, top_it->second, true /*remove*/);
              top_it->second.endpoints_.erase(subscriptionId);
            }
          } else {
            ACE_DEBUG((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: EndpointManager::remove_subscription - ")
                       ACE_TEXT("Failed to publish dispose msg\n")));
          }
        }
      }

      virtual bool update_subscription_qos(const DCPS::RepoId& subscriptionId,
                                           const DDS::DataReaderQos& qos,
                                           const DDS::SubscriberQos& subscriberQos) = 0;

      virtual bool update_subscription_params(const DCPS::RepoId& subId,
                                              const DDS::StringSeq& params) = 0;

      virtual void association_complete(const DCPS::RepoId& localId,
                                        const DCPS::RepoId& remoteId) = 0;

      virtual bool disassociate(const DiscoveredParticipantData& pdata) = 0;

    protected:
      struct LocalEndpoint {
        LocalEndpoint() : topic_id_(DCPS::GUID_UNKNOWN), sequence_(DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {}
        DCPS::RepoId topic_id_;
        DCPS::TransportLocatorSeq trans_info_;
        RepoIdSet matched_endpoints_;
        DCPS::SequenceNumber sequence_;
        RepoIdSet remote_opendds_associations_;
      };

      struct LocalPublication : LocalEndpoint {
        DCPS::DataWriterCallbacks* publication_;
        DDS::DataWriterQos qos_;
        DDS::PublisherQos publisher_qos_;
      };

      struct LocalSubscription : LocalEndpoint {
        DCPS::DataReaderCallbacks* subscription_;
        DDS::DataReaderQos qos_;
        DDS::SubscriberQos subscriber_qos_;
        OpenDDS::DCPS::ContentFilterProperty_t filterProperties;
      };

      typedef OPENDDS_MAP_CMP(DDS::BuiltinTopicKey_t, DCPS::RepoId,
                              DCPS::BuiltinTopicKeyLess) BitKeyMap;

      typedef OPENDDS_MAP_CMP(DCPS::RepoId, LocalPublication,
                              DCPS::GUID_tKeyLessThan) LocalPublicationMap;
      typedef typename LocalPublicationMap::iterator LocalPublicationIter;
      typedef typename LocalPublicationMap::const_iterator LocalPublicationCIter;

      typedef OPENDDS_MAP_CMP(DCPS::RepoId, LocalSubscription,
                              DCPS::GUID_tKeyLessThan) LocalSubscriptionMap;
      typedef typename LocalSubscriptionMap::iterator LocalSubscriptionIter;
      typedef typename LocalSubscriptionMap::const_iterator LocalSubscriptionCIter;

      typedef typename OPENDDS_MAP_CMP(DCPS::RepoId, OPENDDS_STRING, DCPS::GUID_tKeyLessThan) TopicNameMap;

      static const char* get_topic_name(const DiscoveredPublication& pub) {
        return pub.writer_data_.ddsPublicationData.topic_name;
      }
      static const char* get_topic_name(const DiscoveredSubscription& sub) {
        return sub.reader_data_.ddsSubscriptionData.topic_name;
      }
      static DDS::BuiltinTopicKey_t get_key(const DiscoveredPublication& pub) {
        return pub.writer_data_.ddsPublicationData.key;
      }
      static DDS::BuiltinTopicKey_t get_key(const DiscoveredSubscription& sub) {
        return sub.reader_data_.ddsSubscriptionData.key;
      }

      virtual void remove_from_bit_i(const DiscoveredPublication& /*pub*/) { }
      virtual void remove_from_bit_i(const DiscoveredSubscription& /*sub*/) { }

      virtual void assign_publication_key(RepoId& rid,
                                          const RepoId& topicId,
                                          const DDS::DataWriterQos& /*qos*/) {
        rid.entityId.entityKind =
          has_dcps_key(topicId)
          ? DCPS::ENTITYKIND_USER_WRITER_WITH_KEY
          : DCPS::ENTITYKIND_USER_WRITER_NO_KEY;
        assign(rid.entityId.entityKey, publication_counter_++);
      }
      virtual void assign_subscription_key(RepoId& rid,
                                           const RepoId& topicId,
                                           const DDS::DataReaderQos& /*qos*/) {
        rid.entityId.entityKind =
          has_dcps_key(topicId)
          ? DCPS::ENTITYKIND_USER_READER_WITH_KEY
          : DCPS::ENTITYKIND_USER_READER_NO_KEY;
        assign(rid.entityId.entityKey, subscription_counter_++);
      }
      virtual void assign_topic_key(RepoId& guid) {
        assign(guid.entityId.entityKey, topic_counter_++);

        if (topic_counter_ == 0x1000000) {
          ACE_DEBUG((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: EndpointManager::make_topic_guid: ")
                     ACE_TEXT("Exceeded Maximum number of topic entity keys!")
                     ACE_TEXT("Next key will be a duplicate!\n")));
          topic_counter_ = 0;
        }
      }

      virtual DDS::ReturnCode_t add_publication_i(const DCPS::RepoId& /*rid*/,
                                                  LocalPublication& /*pub*/) { return DDS::RETCODE_OK; }
      virtual DDS::ReturnCode_t write_publication_data(const DCPS::RepoId& /*rid*/,
                                                       LocalPublication& /*pub*/,
                                                       const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN) { ACE_UNUSED_ARG(reader); return DDS::RETCODE_OK; }
      virtual DDS::ReturnCode_t remove_publication_i(const RepoId& publicationId) = 0;

      virtual DDS::ReturnCode_t add_subscription_i(const DCPS::RepoId& /*rid*/,
                                                   LocalSubscription& /*pub*/) { return DDS::RETCODE_OK; };
      virtual DDS::ReturnCode_t write_subscription_data(const DCPS::RepoId& /*rid*/,
                                                        LocalSubscription& /*pub*/,
                                                        const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN) { ACE_UNUSED_ARG(reader); return DDS::RETCODE_OK; }
      virtual DDS::ReturnCode_t remove_subscription_i(const RepoId& subscriptionId) = 0;

      void match_endpoints(DCPS::RepoId repoId, const TopicDetails& td,
                           bool remove = false)
      {
        const bool reader = repoId.entityId.entityKind & 4;
        // Copy the endpoint set - lock can be released in match()
        RepoIdSet endpoints_copy = td.endpoints_;

        for (RepoIdSet::const_iterator iter = endpoints_copy.begin();
             iter != endpoints_copy.end(); ++iter) {
          // check to make sure it's a Reader/Writer or Writer/Reader match
          if (bool(iter->entityId.entityKind & 4) != reader) {
            if (remove) {
              remove_assoc(*iter, repoId);
            } else {
              match(reader ? *iter : repoId, reader ? repoId : *iter);
            }
          }
        }
      }

      void
      remove_assoc(const RepoId& remove_from,
                   const RepoId& removing)
      {
        const bool reader = remove_from.entityId.entityKind & 4;
        if (reader) {
          const LocalSubscriptionIter lsi = local_subscriptions_.find(remove_from);
          if (lsi != local_subscriptions_.end()) {
            lsi->second.matched_endpoints_.erase(removing);
            DCPS::WriterIdSeq writer_seq(1);
            writer_seq.length(1);
            writer_seq[0] = removing;
            lsi->second.remote_opendds_associations_.erase(removing);
            lsi->second.subscription_->remove_associations(writer_seq,
                                                           false /*notify_lost*/);
            // Update writer
            write_subscription_data(remove_from, lsi->second);
          }

        } else {
          const LocalPublicationIter lpi = local_publications_.find(remove_from);
          if (lpi != local_publications_.end()) {
            lpi->second.matched_endpoints_.erase(removing);
            DCPS::ReaderIdSeq reader_seq(1);
            reader_seq.length(1);
            reader_seq[0] = removing;
            lpi->second.remote_opendds_associations_.erase(removing);
            lpi->second.publication_->remove_associations(reader_seq,
                                                          false /*notify_lost*/);
          }
        }
      }

      void
      match(const RepoId& writer, const RepoId& reader)
      {
        // 0. For discovered endpoints, we'll have the QoS info in the form of the
        // publication or subscription BIT data which doesn't use the same structures
        // for QoS.  In those cases we can copy the individual QoS policies to temp
        // QoS structs:
        DDS::DataWriterQos tempDwQos;
        DDS::PublisherQos tempPubQos;
        DDS::DataReaderQos tempDrQos;
        DDS::SubscriberQos tempSubQos;
        ContentFilterProperty_t tempCfp;

        // 1. collect details about the writer, which may be local or discovered
        const DDS::DataWriterQos* dwQos = 0;
        const DDS::PublisherQos* pubQos = 0;
        DCPS::TransportLocatorSeq* wTls = 0;

        const LocalPublicationIter lpi = local_publications_.find(writer);
        DiscoveredPublicationIter dpi;
        bool writer_local = false, already_matched = false;
        if (lpi != local_publications_.end()) {
          writer_local = true;
          dwQos = &lpi->second.qos_;
          pubQos = &lpi->second.publisher_qos_;
          wTls = &lpi->second.trans_info_;
          already_matched = lpi->second.matched_endpoints_.count(reader);
        } else if ((dpi = discovered_publications_.find(writer))
                   != discovered_publications_.end()) {
          wTls = &dpi->second.writer_data_.writerProxy.allLocators;
        } else {
          return; // Possible and ok, since lock is released
        }

        // 2. collect details about the reader, which may be local or discovered
        const DDS::DataReaderQos* drQos = 0;
        const DDS::SubscriberQos* subQos = 0;
        DCPS::TransportLocatorSeq* rTls = 0;
        const ContentFilterProperty_t* cfProp = 0;

        const LocalSubscriptionIter lsi = local_subscriptions_.find(reader);
        DiscoveredSubscriptionIter dsi;
        bool reader_local = false;
        if (lsi != local_subscriptions_.end()) {
          reader_local = true;
          drQos = &lsi->second.qos_;
          subQos = &lsi->second.subscriber_qos_;
          rTls = &lsi->second.trans_info_;
          if (lsi->second.filterProperties.filterExpression[0] != 0) {
            tempCfp.filterExpression = lsi->second.filterProperties.filterExpression;
            tempCfp.expressionParameters = lsi->second.filterProperties.expressionParameters;
          }
          cfProp = &tempCfp;
          if (!already_matched) {
            already_matched = lsi->second.matched_endpoints_.count(writer);
          }
        } else if ((dsi = discovered_subscriptions_.find(reader))
                   != discovered_subscriptions_.end()) {
          if (!writer_local) {
            // this is a discovered/discovered match, nothing for us to do
            return;
          }
          rTls = &dsi->second.reader_data_.readerProxy.allLocators;

          populate_transport_locator_sequence(rTls, dsi, reader);

          const DDS::SubscriptionBuiltinTopicData& bit =
            dsi->second.reader_data_.ddsSubscriptionData;
          tempDrQos.durability = bit.durability;
          tempDrQos.deadline = bit.deadline;
          tempDrQos.latency_budget = bit.latency_budget;
          tempDrQos.liveliness = bit.liveliness;
          tempDrQos.reliability = bit.reliability;
          tempDrQos.destination_order = bit.destination_order;
          tempDrQos.history = TheServiceParticipant->initial_HistoryQosPolicy();
          tempDrQos.resource_limits =
            TheServiceParticipant->initial_ResourceLimitsQosPolicy();
          tempDrQos.user_data = bit.user_data;
          tempDrQos.ownership = bit.ownership;
          tempDrQos.time_based_filter = bit.time_based_filter;
          tempDrQos.reader_data_lifecycle =
            TheServiceParticipant->initial_ReaderDataLifecycleQosPolicy();
          drQos = &tempDrQos;
          tempSubQos.presentation = bit.presentation;
          tempSubQos.partition = bit.partition;
          tempSubQos.group_data = bit.group_data;
          tempSubQos.entity_factory =
            TheServiceParticipant->initial_EntityFactoryQosPolicy();
          subQos = &tempSubQos;
          cfProp = &dsi->second.reader_data_.contentFilterProperty;
        } else {
          return; // Possible and ok, since lock is released
        }

        // This is really part of step 1, but we're doing it here just in case we
        // are in the discovered/discovered match and we don't need the QoS data.
        if (!writer_local) {
          const DDS::PublicationBuiltinTopicData& bit =
            dpi->second.writer_data_.ddsPublicationData;
          tempDwQos.durability = bit.durability;
          tempDwQos.durability_service = bit.durability_service;
          tempDwQos.deadline = bit.deadline;
          tempDwQos.latency_budget = bit.latency_budget;
          tempDwQos.liveliness = bit.liveliness;
          tempDwQos.reliability = bit.reliability;
          tempDwQos.destination_order = bit.destination_order;
          tempDwQos.history = TheServiceParticipant->initial_HistoryQosPolicy();
          tempDwQos.resource_limits =
            TheServiceParticipant->initial_ResourceLimitsQosPolicy();
          tempDwQos.transport_priority =
            TheServiceParticipant->initial_TransportPriorityQosPolicy();
          tempDwQos.lifespan = bit.lifespan;
          tempDwQos.user_data = bit.user_data;
          tempDwQos.ownership = bit.ownership;
          tempDwQos.ownership_strength = bit.ownership_strength;
          tempDwQos.writer_data_lifecycle =
            TheServiceParticipant->initial_WriterDataLifecycleQosPolicy();
          dwQos = &tempDwQos;
          tempPubQos.presentation = bit.presentation;
          tempPubQos.partition = bit.partition;
          tempPubQos.group_data = bit.group_data;
          tempPubQos.entity_factory =
            TheServiceParticipant->initial_EntityFactoryQosPolicy();
          pubQos = &tempPubQos;

          populate_transport_locator_sequence(wTls, dpi, writer);
        }

        // Need to release lock, below, for callbacks into DCPS which could
        // call into Spdp/Sedp.  Note that this doesn't unlock, it just constructs
        // an ACE object which will be used below for unlocking.
        ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

        // 3. check transport and QoS compatibility

        // Copy entries from local publication and local subscription maps
        // prior to releasing lock
        DCPS::DataWriterCallbacks* dwr = 0;
        DCPS::DataReaderCallbacks* drr = 0;
        if (writer_local) {
          dwr = lpi->second.publication_;
        }
        if (reader_local) {
          drr = lsi->second.subscription_;
        }

        DCPS::IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
        DCPS::IncompatibleQosStatus readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};

        if (DCPS::compatibleQOS(&writerStatus, &readerStatus, *wTls, *rTls,
                                dwQos, drQos, pubQos, subQos)) {
          if (!writer_local) {
            RepoId writer_participant = writer;
            writer_participant.entityId = ENTITYID_PARTICIPANT;
            if (defer_writer(writer, writer_participant)) {
              return;
            }
          }
          if (!reader_local) {
            RepoId reader_participant = reader;
            reader_participant.entityId = ENTITYID_PARTICIPANT;
            if (defer_reader(reader, reader_participant)) {
              return;
            }
          }

          bool call_writer = false, call_reader = false;
          if (writer_local) {
            call_writer = lpi->second.matched_endpoints_.insert(reader).second;
          }
          if (reader_local) {
            call_reader = lsi->second.matched_endpoints_.insert(writer).second;
          }
          if (!call_writer && !call_reader) {
            return; // nothing more to do
          }
          // Copy reader and writer association data prior to releasing lock
#ifdef __SUNPRO_CC
          DCPS::ReaderAssociation ra;
          ra.readerTransInfo = *rTls;
          ra.readerId = reader;
          ra.subQos = *subQos;
          ra.readerQos = *drQos;
          ra.filterClassName = cfProp->filterClassName;
          ra.filterExpression = cfProp->filterExpression;
          ra.exprParams = cfProp->expressionParameters;
          DCPS::WriterAssociation wa;
          wa.writerTransInfo = *wTls;
          wa.writerId = writer;
          wa.pubQos = *pubQos;
          wa.writerQos = *dwQos;
#else
          const DCPS::ReaderAssociation ra =
            {*rTls, reader, *subQos, *drQos,
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
             cfProp->filterClassName, cfProp->filterExpression,
#else
             "", "",
#endif
             cfProp->expressionParameters};

          const DCPS::WriterAssociation wa = {*wTls, writer, *pubQos, *dwQos};
#endif

          ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
          static const bool writer_active = true;

          if (call_writer) {
            if (DCPS::DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("adding writer association\n")));
            }
            DcpsUpcalls thr(drr, reader, wa, !writer_active, dwr);
            if (call_reader) {
              thr.activate();
            }
            dwr->add_association(writer, ra, writer_active);
            if (call_reader) {
              thr.writer_done();
            }

          } else if (call_reader) {
            if (DCPS::DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("adding reader association\n")));
            }
            drr->add_association(reader, wa, !writer_active);
          }

          // change this if 'writer_active' (above) changes
          if (call_writer && !call_reader && !is_opendds(reader)) {
            if (DCPS::DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG,
                         ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("calling writer association_complete\n")));
            }
            dwr->association_complete(reader);
          }

        } else if (already_matched) { // break an existing associtaion
          if (writer_local) {
            lpi->second.matched_endpoints_.erase(reader);
            lpi->second.remote_opendds_associations_.erase(reader);
          }
          if (reader_local) {
            lsi->second.matched_endpoints_.erase(writer);
            lsi->second.remote_opendds_associations_.erase(writer);
          }
          ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
          if (writer_local) {
            DCPS::ReaderIdSeq reader_seq(1);
            reader_seq.length(1);
            reader_seq[0] = reader;
            dwr->remove_associations(reader_seq, false /*notify_lost*/);
          }
          if (reader_local) {
            DCPS::WriterIdSeq writer_seq(1);
            writer_seq.length(1);
            writer_seq[0] = writer;
            drr->remove_associations(writer_seq, false /*notify_lost*/);
          }

        } else { // something was incompatible
          ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
          if (writer_local && writerStatus.count_since_last_send) {
            if (DCPS::DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("writer incompatible\n")));
            }
            dwr->update_incompatible_qos(writerStatus);
          }
          if (reader_local && readerStatus.count_since_last_send) {
            if (DCPS::DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("reader incompatible\n")));
            }
            drr->update_incompatible_qos(readerStatus);
          }
        }
      }

      static bool is_opendds(const GUID_t& endpoint)
      {
        return !std::memcmp(endpoint.guidPrefix, DCPS::VENDORID_OCI,
                            sizeof(DCPS::VENDORID_OCI));
      }

      virtual bool shutting_down() const = 0;

      virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& tls,
                                                       DiscoveredSubscriptionIter& iter,
                                                       const RepoId& reader) = 0;

      virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& tls,
                                                       DiscoveredPublicationIter& iter,
                                                       const RepoId& reader) = 0;

      virtual bool defer_writer(const RepoId& writer,
                                const RepoId& writer_participant) = 0;

      virtual bool defer_reader(const RepoId& writer,
                                const RepoId& writer_participant) = 0;

      void remove_from_bit(const DiscoveredPublication& pub)
      {
        pub_key_to_id_.erase(get_key(pub));
        remove_from_bit_i(pub);
      }

      void remove_from_bit(const DiscoveredSubscription& sub)
      {
        sub_key_to_id_.erase(get_key(sub));
        remove_from_bit_i(sub);
      }

      RepoId make_topic_guid()
      {
        RepoId guid;
        guid = participant_id_;
        guid.entityId.entityKind = DCPS::ENTITYKIND_OPENDDS_TOPIC;
        assign_topic_key(guid);
        return guid;
      }

      bool has_dcps_key(const DCPS::RepoId& topicId) const
      {
        typedef OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, DCPS::GUID_tKeyLessThan) TNMap;
        TNMap::const_iterator tn = topic_names_.find(topicId);
        if (tn == topic_names_.end()) return false;

        typedef OPENDDS_MAP(OPENDDS_STRING, TopicDetails) TDMap;
        typename TDMap::const_iterator td = topics_.find(tn->second);
        if (td == topics_.end()) return false;

        return td->second.has_dcps_key_;
      }

      void
      increment_key(DDS::BuiltinTopicKey_t& key)
      {
        for (int idx = 0; idx < 3; ++idx) {
          CORBA::ULong ukey = static_cast<CORBA::ULong>(key.value[idx]);
          if (ukey == 0xFFFFFFFF) {
            key.value[idx] = 0;
          } else {
            ++ukey;
            key.value[idx] = ukey;
            return;
          }
        }
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::increment_key - ")
                   ACE_TEXT("ran out of builtin topic keys\n")));
      }

      ACE_Thread_Mutex& lock_;
      DCPS::RepoId participant_id_;
      BitKeyMap pub_key_to_id_, sub_key_to_id_;
      RepoIdSet ignored_guids_;
      unsigned int publication_counter_, subscription_counter_, topic_counter_;
      LocalPublicationMap local_publications_;
      LocalSubscriptionMap local_subscriptions_;
      DiscoveredPublicationMap discovered_publications_;
      DiscoveredSubscriptionMap discovered_subscriptions_;
      OPENDDS_MAP(OPENDDS_STRING, TopicDetails) topics_;
      TopicNameMap topic_names_;
      OPENDDS_SET(OPENDDS_STRING) ignored_topics_;
      DDS::BuiltinTopicKey_t pub_bit_key_, sub_bit_key_;
    };

    template <typename EndpointManagerType>
    class LocalParticipant : public DCPS::RcObject<ACE_SYNCH_MUTEX> {
    public:
      typedef typename EndpointManagerType::DiscoveredParticipantData DiscoveredParticipantData;
      typedef typename EndpointManagerType::TopicDetails TopicDetails;

      LocalParticipant (const DDS::DomainParticipantQos& qos)
        : qos_(qos)
      { }

      virtual ~LocalParticipant() { }

      DCPS::RepoId bit_key_to_repo_id(const char* bit_topic_name,
                                      const DDS::BuiltinTopicKey_t& key)
      {
        if (0 == std::strcmp(bit_topic_name, DCPS::BUILT_IN_PARTICIPANT_TOPIC)) {
          RepoId guid;
          std::memcpy(guid.guidPrefix, key.value, sizeof(DDS::BuiltinTopicKeyValue));
          guid.entityId = ENTITYID_PARTICIPANT;
          return guid;

        } else {
          return endpoint_manager().bit_key_to_repo_id(bit_topic_name, key);
        }
      }

      void ignore_domain_participant(const RepoId& ignoreId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        endpoint_manager().ignore(ignoreId);

        const DiscoveredParticipantIter iter = participants_.find(ignoreId);
        if (iter != participants_.end()) {
          remove_discovered_participant(iter);
        }
      }

      bool
      update_domain_participant_qos(const DDS::DomainParticipantQos& qos)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
        qos_ = qos;
        return true;
      }

      DCPS::TopicStatus
      assert_topic(DCPS::RepoId_out topicId, const char* topicName,
                   const char* dataTypeName, const DDS::TopicQos& qos,
                   bool hasDcpsKey)
      {
        if (std::strlen(topicName) > 256 || std::strlen(dataTypeName) > 256) {
          if (DCPS::DCPS_debug_level) {
            ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR LocalParticipant::assert_topic() - ")
                       ACE_TEXT("topic or type name length limit (256) exceeded\n")));
          }
          return DCPS::PRECONDITION_NOT_MET;
        }

        return endpoint_manager().assert_topic(topicId, topicName, dataTypeName, qos, hasDcpsKey);
      }

      DCPS::TopicStatus
      remove_topic(const RepoId& topicId, OPENDDS_STRING& name)
      {
        return endpoint_manager().remove_topic(topicId, name);
      }

      void
      ignore_topic(const RepoId& ignoreId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        endpoint_manager().ignore(ignoreId);
      }

      bool
      update_topic_qos(const RepoId& topicId, const DDS::TopicQos& qos,
                       OPENDDS_STRING& name)
      {
        return endpoint_manager().update_topic_qos(topicId, qos, name);
      }

      RepoId
      add_publication(const RepoId& topicId,
                      DCPS::DataWriterCallbacks* publication,
                      const DDS::DataWriterQos& qos,
                      const DCPS::TransportLocatorSeq& transInfo,
                      const DDS::PublisherQos& publisherQos)
      {
        return endpoint_manager().add_publication(topicId, publication, qos,
                                                  transInfo, publisherQos);
      }

      void
      remove_publication(const RepoId& publicationId)
      {
        endpoint_manager().remove_publication(publicationId);
      }

      void
      ignore_publication(const RepoId& ignoreId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        return endpoint_manager().ignore(ignoreId);
      }

      bool
      update_publication_qos(const RepoId& publicationId,
                             const DDS::DataWriterQos& qos,
                             const DDS::PublisherQos& publisherQos)
      {
        return endpoint_manager().update_publication_qos(publicationId, qos, publisherQos);
      }

      RepoId
      add_subscription(const RepoId& topicId,
                       DCPS::DataReaderCallbacks* subscription,
                       const DDS::DataReaderQos& qos,
                       const DCPS::TransportLocatorSeq& transInfo,
                       const DDS::SubscriberQos& subscriberQos,
                       const char* filterClassName,
                       const char* filterExpr,
                       const DDS::StringSeq& params)
      {
        return endpoint_manager().add_subscription(topicId, subscription, qos, transInfo,
                                                   subscriberQos, filterClassName, filterExpr, params);
      }

      void
      remove_subscription(const RepoId& subscriptionId)
      {
        endpoint_manager().remove_subscription(subscriptionId);
      }

      void
      ignore_subscription(const RepoId& ignoreId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        return endpoint_manager().ignore(ignoreId);
      }

      bool
      update_subscription_qos(const RepoId& subscriptionId,
                              const DDS::DataReaderQos& qos,
                              const DDS::SubscriberQos& subscriberQos)
      {
        return endpoint_manager().update_subscription_qos(subscriptionId, qos, subscriberQos);
      }

      bool
      update_subscription_params(const RepoId& subId,
                                 const DDS::StringSeq& params)
      {
        return endpoint_manager().update_subscription_params(subId, params);
      }

      void
      association_complete(const RepoId& localId, const RepoId& remoteId)
      {
        endpoint_manager().association_complete(localId, remoteId);
      }

      DDS::Subscriber_var bit_subscriber() const { return bit_subscriber_; }

    protected:

      struct DiscoveredParticipant {
        DiscoveredParticipant() : bit_ih_(0) {}
        DiscoveredParticipant(const DiscoveredParticipantData& p,
                              const ACE_Time_Value& t)
          : pdata_(p), last_seen_(t), bit_ih_(DDS::HANDLE_NIL) {}

        DiscoveredParticipantData pdata_;
        ACE_Time_Value last_seen_;
        DDS::InstanceHandle_t bit_ih_;
      };
      typedef OPENDDS_MAP_CMP(DCPS::RepoId, DiscoveredParticipant,
                              DCPS::GUID_tKeyLessThan) DiscoveredParticipantMap;
      typedef typename DiscoveredParticipantMap::iterator DiscoveredParticipantIter;

      virtual EndpointManagerType& endpoint_manager() = 0;

      void remove_discovered_participant(DiscoveredParticipantIter iter)
      {
        bool removed = endpoint_manager().disassociate(iter->second.pdata_);
        if (removed) {
#ifndef DDS_HAS_MINIMUM_BIT
          ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
          // bit may be null if the DomainParticipant is shutting down
          if (bit && iter->second.bit_ih_ != DDS::HANDLE_NIL) {
            bit->set_instance_state(iter->second.bit_ih_,
                                    DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
          }
#endif /* DDS_HAS_MINIMUM_BIT */
          if (DCPS::DCPS_debug_level > 3) {
            DCPS::GuidConverter conv(iter->first);
            ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) LocalParticipant::remove_discovered_participant")
                       ACE_TEXT(" - erasing %C\n"), OPENDDS_STRING(conv).c_str()));
          }
          participants_.erase(iter);
        }
      }

#ifndef DDS_HAS_MINIMUM_BIT
      ParticipantBuiltinTopicDataDataReaderImpl* part_bit()
      {
        if (!bit_subscriber_.in())
          return 0;

        DDS::DataReader_var d =
          bit_subscriber_->lookup_datareader(DCPS::BUILT_IN_PARTICIPANT_TOPIC);
        return dynamic_cast<ParticipantBuiltinTopicDataDataReaderImpl*>(d.in());
      }
#endif /* DDS_HAS_MINIMUM_BIT */

      ACE_Thread_Mutex lock_;
      DDS::Subscriber_var bit_subscriber_;
      DDS::DomainParticipantQos qos_;
      DiscoveredParticipantMap participants_;
    };

    template<typename Participant>
    class PeerDiscovery : public Discovery {
    public:
      typedef typename Participant::TopicDetails TopicDetails;

      explicit PeerDiscovery(const RepoKey& key) : Discovery(key) { }

      ~PeerDiscovery() {
        reactor_runner_.end();
      }

      virtual DDS::Subscriber_ptr init_bit(DomainParticipantImpl* participant) {
        using namespace DCPS;
        if (create_bit_topics(participant) != DDS::RETCODE_OK) {
          return 0;
        }

        DDS::Subscriber_var bit_subscriber =
          participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                         DDS::SubscriberListener::_nil(),
                                         DEFAULT_STATUS_MASK);
        SubscriberImpl* sub = dynamic_cast<SubscriberImpl*>(bit_subscriber.in());

        DDS::DataReaderQos dr_qos;
        sub->get_default_datareader_qos(dr_qos);
        dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

#ifndef DDS_HAS_MINIMUM_BIT
        DDS::TopicDescription_var bit_part_topic =
          participant->lookup_topicdescription(BUILT_IN_PARTICIPANT_TOPIC);
        create_bit_dr(bit_part_topic, BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                      sub, dr_qos);

        DDS::TopicDescription_var bit_topic_topic =
          participant->lookup_topicdescription(BUILT_IN_TOPIC_TOPIC);
        create_bit_dr(bit_topic_topic, BUILT_IN_TOPIC_TOPIC_TYPE,
                      sub, dr_qos);

        DDS::TopicDescription_var bit_pub_topic =
          participant->lookup_topicdescription(BUILT_IN_PUBLICATION_TOPIC);
        create_bit_dr(bit_pub_topic, BUILT_IN_PUBLICATION_TOPIC_TYPE,
                      sub, dr_qos);

        DDS::TopicDescription_var bit_sub_topic =
          participant->lookup_topicdescription(BUILT_IN_SUBSCRIPTION_TOPIC);
        create_bit_dr(bit_sub_topic, BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                      sub, dr_qos);
#endif /* DDS_HAS_MINIMUM_BIT */

        get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber);

        return bit_subscriber._retn();
      }

      virtual void fini_bit(DCPS::DomainParticipantImpl* participant)
      {
        get_part(participant->get_domain_id(), participant->get_id())->fini_bit();
      }

      virtual OpenDDS::DCPS::RepoId bit_key_to_repo_id(DCPS::DomainParticipantImpl* participant,
                                                       const char* bit_topic_name,
                                                       const DDS::BuiltinTopicKey_t& key) const
      {
        return get_part(participant->get_domain_id(), participant->get_id())
          ->bit_key_to_repo_id(bit_topic_name, key);
      }

      virtual bool attach_participant(DDS::DomainId_t /*domainId*/,
                                      const OpenDDS::DCPS::RepoId& /*participantId*/)
      {
        return false; // This is just for DCPSInfoRepo?
      }

      virtual bool remove_domain_participant(DDS::DomainId_t domain_id,
                                             const OpenDDS::DCPS::RepoId& participantId)
      {
        // Use reference counting to ensure participant
        // does not get deleted until lock as been released.
        ParticipantHandle participant;
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
        typename DomainParticipantMap::iterator domain = participants_.find(domain_id);
        if (domain == participants_.end()) {
          return false;
        }
        typename ParticipantMap::iterator part = domain->second.find(participantId);
        if (part == domain->second.end()) {
          return false;
        }
        participant = part->second;
        domain->second.erase(part);
        if (domain->second.empty()) {
          participants_.erase(domain);
        }

        return true;
      }

      virtual bool ignore_domain_participant(DDS::DomainId_t domain,
                                             const OpenDDS::DCPS::RepoId& myParticipantId,
                                             const OpenDDS::DCPS::RepoId& ignoreId)
      {
        get_part(domain, myParticipantId)->ignore_domain_participant(ignoreId);
        return true;
      }

      virtual bool update_domain_participant_qos(DDS::DomainId_t domain,
                                                 const OpenDDS::DCPS::RepoId& participant,
                                                 const DDS::DomainParticipantQos& qos)
      {
        return get_part(domain, participant)->update_domain_participant_qos(qos);
      }

      virtual DCPS::TopicStatus assert_topic(OpenDDS::DCPS::RepoId_out topicId,
                                             DDS::DomainId_t domainId,
                                             const OpenDDS::DCPS::RepoId& participantId,
                                             const char* topicName,
                                             const char* dataTypeName,
                                             const DDS::TopicQos& qos,
                                             bool hasDcpsKey)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
        typename OPENDDS_MAP(DDS::DomainId_t,
                             OPENDDS_MAP(OPENDDS_STRING, TopicDetails) )::iterator topic_it =
          topics_.find(domainId);
        if (topic_it != topics_.end()) {
          const typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator it =
            topic_it->second.find(topicName);
          if (it != topic_it->second.end()
              && it->second.data_type_ != dataTypeName) {
            topicId = GUID_UNKNOWN;
            return DCPS::CONFLICTING_TYPENAME;
          }
        }

        // Verified its safe to hold lock during call to assert_topic
        const DCPS::TopicStatus stat =
          participants_[domainId][participantId]->assert_topic(topicId, topicName,
                                                               dataTypeName, qos,
                                                               hasDcpsKey);
        if (stat == DCPS::CREATED || stat == DCPS::FOUND) { // qos change (FOUND)
          TopicDetails& td = topics_[domainId][topicName];
          td.data_type_ = dataTypeName;
          td.qos_ = qos;
          td.repo_id_ = topicId;
          ++topic_use_[domainId][topicName];
        }
        return stat;
      }

      virtual DCPS::TopicStatus find_topic(DDS::DomainId_t domainId, const char* topicName,
                                           CORBA::String_out dataTypeName, DDS::TopicQos_out qos,
                                           OpenDDS::DCPS::RepoId_out topicId)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
        typename OPENDDS_MAP(DDS::DomainId_t,
                             OPENDDS_MAP(OPENDDS_STRING, TopicDetails) )::iterator topic_it =
          topics_.find(domainId);
        if (topic_it == topics_.end()) {
          return DCPS::NOT_FOUND;
        }
        typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator iter =
          topic_it->second.find(topicName);
        if (iter == topic_it->second.end()) {
          return DCPS::NOT_FOUND;
        }
        TopicDetails& td = iter->second;
        dataTypeName = td.data_type_.c_str();
        qos = new DDS::TopicQos(td.qos_);
        topicId = td.repo_id_;
        ++topic_use_[domainId][topicName];
        return DCPS::FOUND;
      }

      virtual DCPS::TopicStatus remove_topic(DDS::DomainId_t domainId,
                                             const OpenDDS::DCPS::RepoId& participantId,
                                             const OpenDDS::DCPS::RepoId& topicId)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
        typename OPENDDS_MAP(DDS::DomainId_t,
                             OPENDDS_MAP(OPENDDS_STRING, TopicDetails) )::iterator topic_it =
          topics_.find(domainId);
        if (topic_it == topics_.end()) {
          return DCPS::NOT_FOUND;
        }

        OPENDDS_STRING name;
        // Safe to hold lock while calling remove topic
        const DCPS::TopicStatus stat =
          participants_[domainId][participantId]->remove_topic(topicId, name);

        if (stat == DCPS::REMOVED) {
          if (0 == --topic_use_[domainId][name]) {
            topic_use_[domainId].erase(name);
            if (topic_it->second.empty()) {
              topic_use_.erase(domainId);
            }
            topic_it->second.erase(name);
            if (topic_it->second.empty()) {
              topics_.erase(topic_it);
            }
          }
        }
        return stat;
      }

      virtual bool ignore_topic(DDS::DomainId_t domainId, const OpenDDS::DCPS::RepoId& myParticipantId,
                                const OpenDDS::DCPS::RepoId& ignoreId)
      {
        get_part(domainId, myParticipantId)->ignore_topic(ignoreId);
        return true;
      }

      virtual bool update_topic_qos(const OpenDDS::DCPS::RepoId& topicId, DDS::DomainId_t domainId,
                                    const OpenDDS::DCPS::RepoId& participantId, const DDS::TopicQos& qos)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
        OPENDDS_STRING name;
        // Safe to hold lock while calling update_topic_qos
        if (participants_[domainId][participantId]->update_topic_qos(topicId,
                                                                     qos, name)) {
          topics_[domainId][name].qos_ = qos;
          return true;
        }
        return false;
      }

      virtual OpenDDS::DCPS::RepoId add_publication(DDS::DomainId_t domainId,
                                                    const OpenDDS::DCPS::RepoId& participantId,
                                                    const OpenDDS::DCPS::RepoId& topicId,
                                                    DCPS::DataWriterCallbacks* publication,
                                                    const DDS::DataWriterQos& qos,
                                                    const DCPS::TransportLocatorSeq& transInfo,
                                                    const DDS::PublisherQos& publisherQos)
      {
        return get_part(domainId, participantId)->add_publication(
                                                                  topicId, publication, qos, transInfo, publisherQos);
      }

      virtual bool remove_publication(DDS::DomainId_t domainId,
                                      const OpenDDS::DCPS::RepoId& participantId,
                                      const OpenDDS::DCPS::RepoId& publicationId)
      {
        get_part(domainId, participantId)->remove_publication(publicationId);
        return true;
      }

      virtual bool ignore_publication(DDS::DomainId_t domainId,
                                      const OpenDDS::DCPS::RepoId& participantId,
                                      const OpenDDS::DCPS::RepoId& ignoreId)
      {
        get_part(domainId, participantId)->ignore_publication(ignoreId);
        return true;
      }

      virtual bool update_publication_qos(DDS::DomainId_t domainId,
                                          const OpenDDS::DCPS::RepoId& partId,
                                          const OpenDDS::DCPS::RepoId& dwId,
                                          const DDS::DataWriterQos& qos,
                                          const DDS::PublisherQos& publisherQos)
      {
        return get_part(domainId, partId)->update_publication_qos(dwId, qos,
                                                                  publisherQos);
      }

      virtual OpenDDS::DCPS::RepoId add_subscription(DDS::DomainId_t domainId,
                                                     const OpenDDS::DCPS::RepoId& participantId,
                                                     const OpenDDS::DCPS::RepoId& topicId,
                                                     DCPS::DataReaderCallbacks* subscription,
                                                     const DDS::DataReaderQos& qos,
                                                     const DCPS::TransportLocatorSeq& transInfo,
                                                     const DDS::SubscriberQos& subscriberQos,
                                                     const char* filterClassName,
                                                     const char* filterExpr,
                                                     const DDS::StringSeq& params)
      {
        return get_part(domainId, participantId)->add_subscription(topicId, subscription, qos, transInfo, subscriberQos, filterClassName, filterExpr, params);
      }

      virtual bool remove_subscription(DDS::DomainId_t domainId,
                                       const OpenDDS::DCPS::RepoId& participantId,
                                       const OpenDDS::DCPS::RepoId& subscriptionId)
      {
        get_part(domainId, participantId)->remove_subscription(subscriptionId);
        return true;
      }

      virtual bool ignore_subscription(DDS::DomainId_t domainId,
                                       const OpenDDS::DCPS::RepoId& participantId,
                                       const OpenDDS::DCPS::RepoId& ignoreId)
      {
        get_part(domainId, participantId)->ignore_subscription(ignoreId);
        return true;
      }

      virtual bool update_subscription_qos(DDS::DomainId_t domainId,
                                           const OpenDDS::DCPS::RepoId& partId,
                                           const OpenDDS::DCPS::RepoId& drId,
                                           const DDS::DataReaderQos& qos,
                                           const DDS::SubscriberQos& subQos)
      {
        return get_part(domainId, partId)->update_subscription_qos(drId, qos, subQos);
      }

      virtual bool update_subscription_params(DDS::DomainId_t domainId,
                                              const OpenDDS::DCPS::RepoId& partId,
                                              const OpenDDS::DCPS::RepoId& subId,
                                              const DDS::StringSeq& params)
      {
        return get_part(domainId, partId)->update_subscription_params(subId, params);
      }

      virtual void association_complete(DDS::DomainId_t domainId,
                                        const OpenDDS::DCPS::RepoId& participantId,
                                        const OpenDDS::DCPS::RepoId& localId,
                                        const OpenDDS::DCPS::RepoId& remoteId)
      {
        get_part(domainId, participantId)->association_complete(localId, remoteId);
      }

      ACE_Reactor*
      reactor()
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, reactor_runner_.mtx_, 0);
        if (!reactor_runner_.reactor_) {
          reactor_runner_.reactor_ = new ACE_Reactor(new ACE_Select_Reactor, true);
          reactor_runner_.activate();
        }
        return reactor_runner_.reactor_;
      }

    protected:

      typedef DCPS::RcHandle<Participant> ParticipantHandle;
      typedef OPENDDS_MAP_CMP(DCPS::RepoId, ParticipantHandle, DCPS::GUID_tKeyLessThan) ParticipantMap;
      typedef OPENDDS_MAP(DDS::DomainId_t, ParticipantMap) DomainParticipantMap;

      ParticipantHandle
        get_part(const DDS::DomainId_t domain_id,
                 const OpenDDS::DCPS::RepoId& part_id) const
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ParticipantHandle());
        typename DomainParticipantMap::const_iterator domain = participants_.find(domain_id);
        if (domain == participants_.end()) {
          return ParticipantHandle();
        }
        typename ParticipantMap::const_iterator part = domain->second.find(part_id);
        if (part == domain->second.end()) {
          return ParticipantHandle();
        }
        return part->second;
      }

      void create_bit_dr(DDS::TopicDescription_ptr topic, const char* type,
                         DCPS::SubscriberImpl* sub,
                         const DDS::DataReaderQos& qos)
      {
        using namespace DCPS;
        TopicDescriptionImpl* bit_topic_i =
          dynamic_cast<TopicDescriptionImpl*>(topic);

        DDS::DomainParticipant_var participant = sub->get_participant();
        DomainParticipantImpl* participant_i =
          dynamic_cast<DomainParticipantImpl*>(participant.in());

        TypeSupport_var type_support =
          Registered_Data_Types->lookup(participant, type);

        DDS::DataReader_var dr = type_support->create_datareader();
        OpenDDS::DCPS::DataReaderImpl* dri = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(dr.in());

        dri->init(bit_topic_i, qos, 0 /*listener*/, 0 /*mask*/,
                  participant_i, sub, dr);
        dri->disable_transport();
        dri->enable();
      }

      mutable ACE_Thread_Mutex lock_;

      // Before participants_ so destroyed after.
      struct ReactorRunner : ACE_Task_Base {
      ReactorRunner() : reactor_(0) {}
        ~ReactorRunner()
        {
          delete reactor_;
        }

        int svc()
        {
          reactor_->owner(ACE_Thread_Manager::instance()->thr_self());
          reactor_->run_reactor_event_loop();
          return 0;
        }

        void end()
        {
          ACE_GUARD(ACE_Thread_Mutex, g, mtx_);
          if (reactor_) {
            reactor_->end_reactor_event_loop();
            wait();
          }
        }

        ACE_Reactor* reactor_;
        ACE_Thread_Mutex mtx_;
      } reactor_runner_;

      DomainParticipantMap participants_;
      OPENDDS_MAP(DDS::DomainId_t, OPENDDS_MAP(OPENDDS_STRING, TopicDetails) ) topics_;
      OPENDDS_MAP(DDS::DomainId_t, OPENDDS_MAP(OPENDDS_STRING, unsigned int) ) topic_use_;
    };

  } // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DISCOVERYBASE_H  */
