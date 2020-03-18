/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DDS_DCPS_DISCOVERYBASE_H
#define OPENDDS_DDS_DCPS_DISCOVERYBASE_H

#include "dds/DCPS/TopicDetails.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DataReaderImpl_T.h"
#include "dds/DCPS/DCPS_Utils.h"
#include "dds/DCPS/Discovery.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PoolAllocationBase.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DCPS/SubscriberImpl.h"

#include "dds/DdsDcpsCoreTypeSupportImpl.h"

#ifdef OPENDDS_SECURITY
#include "dds/DdsSecurityCoreC.h"
#include "dds/DCPS/Ice.h"
#endif

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
    typedef DataReaderImpl_T<ParticipantLocationBuiltinTopicData> ParticipantLocationBuiltinTopicDataDataReaderImpl;
    typedef DataReaderImpl_T<DDS::TopicBuiltinTopicData> TopicBuiltinTopicDataDataReaderImpl;

#ifdef OPENDDS_SECURITY
    typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::DatareaderCryptoHandle, GUID_tKeyLessThan)
      DatareaderCryptoHandleMap;
    typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::DatawriterCryptoHandle, GUID_tKeyLessThan)
      DatawriterCryptoHandleMap;
    typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::DatareaderCryptoTokenSeq, GUID_tKeyLessThan)
      DatareaderCryptoTokenSeqMap;
    typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::DatawriterCryptoTokenSeq, GUID_tKeyLessThan)
      DatawriterCryptoTokenSeqMap;
    typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::EndpointSecurityAttributes, GUID_tKeyLessThan)
      EndpointSecurityAttributesMap;

    enum AuthState {
      AS_UNKNOWN,
      AS_VALIDATING_REMOTE,
      AS_HANDSHAKE_REQUEST,
      AS_HANDSHAKE_REQUEST_SENT,
      AS_HANDSHAKE_REPLY,
      AS_HANDSHAKE_REPLY_SENT,
      AS_AUTHENTICATED,
      AS_UNAUTHENTICATED
    };
#endif

    inline void assign(EntityKey_t& lhs, unsigned int rhs)
    {
      lhs[0] = static_cast<CORBA::Octet>(rhs);
      lhs[1] = static_cast<CORBA::Octet>(rhs >> 8);
      lhs[2] = static_cast<CORBA::Octet>(rhs >> 16);
    }

    struct DcpsUpcalls : ACE_Task_Base {
      DcpsUpcalls(DataReaderCallbacks* drr,
                  const RepoId& reader,
                  const WriterAssociation& wa,
                  bool active,
                  DataWriterCallbacks* dwr)
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

      DataReaderCallbacks* const drr_;
      const RepoId& reader_;
      const WriterAssociation& wa_;
      bool active_;
      DataWriterCallbacks* const dwr_;
      bool reader_done_, writer_done_;
      ACE_Thread_Mutex mtx_;
      ACE_Condition_Thread_Mutex cnd_;
    };

    template <typename DiscoveredParticipantData_>
    class EndpointManager {
    protected:

      struct DiscoveredSubscription : PoolAllocationBase {
        DiscoveredSubscription()
        : bit_ih_(DDS::HANDLE_NIL)
#ifdef OPENDDS_SECURITY
        , have_ice_agent_info_(false)
#endif
        {
        }

        explicit DiscoveredSubscription(const DiscoveredReaderData& r)
        : reader_data_(r)
        , bit_ih_(DDS::HANDLE_NIL)
#ifdef OPENDDS_SECURITY
        , have_ice_agent_info_(false)
#endif
        {
        }

        DiscoveredReaderData reader_data_;
        DDS::InstanceHandle_t bit_ih_;

#ifdef OPENDDS_SECURITY
        DDS::Security::EndpointSecurityAttributes security_attribs_;
        bool have_ice_agent_info_;
        ICE::AgentInfo ice_agent_info_;
#endif

      };

      typedef OPENDDS_MAP_CMP(RepoId, DiscoveredSubscription,
                              GUID_tKeyLessThan) DiscoveredSubscriptionMap;

      typedef typename DiscoveredSubscriptionMap::iterator DiscoveredSubscriptionIter;

      struct DiscoveredPublication : PoolAllocationBase {
        DiscoveredPublication()
        : bit_ih_(DDS::HANDLE_NIL)
#ifdef OPENDDS_SECURITY
        , have_ice_agent_info_(false)
#endif
        {
        }

        explicit DiscoveredPublication(const DiscoveredWriterData& w)
        : writer_data_(w)
        , bit_ih_(DDS::HANDLE_NIL)
#ifdef OPENDDS_SECURITY
        , have_ice_agent_info_(false)
#endif
        {
        }

        DiscoveredWriterData writer_data_;
        DDS::InstanceHandle_t bit_ih_;

#ifdef OPENDDS_SECURITY
        DDS::Security::EndpointSecurityAttributes security_attribs_;
        bool have_ice_agent_info_;
        ICE::AgentInfo ice_agent_info_;
#endif

      };

      typedef OPENDDS_MAP_CMP(RepoId, DiscoveredPublication,
                              GUID_tKeyLessThan) DiscoveredPublicationMap;
      typedef typename DiscoveredPublicationMap::iterator DiscoveredPublicationIter;

    public:
      typedef DiscoveredParticipantData_ DiscoveredParticipantData;
      typedef DCPS::TopicDetails TopicDetails;

      EndpointManager(const RepoId& participant_id, ACE_Thread_Mutex& lock)
        : lock_(lock)
        , participant_id_(participant_id)
        , publication_counter_(0)
        , subscription_counter_(0)
        , topic_counter_(0)
#ifdef OPENDDS_SECURITY
        , permissions_handle_(DDS::HANDLE_NIL)
        , crypto_handle_(DDS::HANDLE_NIL)
#endif
      {
      }

      virtual ~EndpointManager() { }

      RepoId bit_key_to_repo_id(const char* bit_topic_name,
                                const DDS::BuiltinTopicKey_t& key)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, RepoId());
        if (0 == std::strcmp(bit_topic_name, BUILT_IN_PUBLICATION_TOPIC)) {
          return pub_key_to_id_[key];
        }
        if (0 == std::strcmp(bit_topic_name, BUILT_IN_SUBSCRIPTION_TOPIC)) {
          return sub_key_to_id_[key];
        }
        return RepoId();
      }

      void purge_dead_topic(const OPENDDS_STRING& topic_name) {
        typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it = topics_.find(topic_name);
        topic_names_.erase(top_it->second.topic_id());
        topics_.erase(top_it);
      }

      void ignore(const RepoId& to_ignore)
      {
        // Locked prior to call from Spdp.
        ignored_guids_.insert(to_ignore);
        {
          const DiscoveredPublicationIter iter =
            discovered_publications_.find(to_ignore);
          if (iter != discovered_publications_.end()) {
            // clean up tracking info
            OPENDDS_STRING topic_name = get_topic_name(iter->second);
            TopicDetails& td = topics_[topic_name];
            td.remove_pub_sub(iter->first);
            remove_from_bit(iter->second);
            discovered_publications_.erase(iter);
            // break associations
            match_endpoints(to_ignore, td, true /*remove*/);
            if (td.is_dead()) {
              purge_dead_topic(topic_name);
            }
            return;
          }
        }
        {
          const DiscoveredSubscriptionIter iter =
            discovered_subscriptions_.find(to_ignore);
          if (iter != discovered_subscriptions_.end()) {
            // clean up tracking info
            OPENDDS_STRING topic_name = get_topic_name(iter->second);
            TopicDetails& td = topics_[topic_name];
            td.remove_pub_sub(iter->first);
            remove_from_bit(iter->second);
            discovered_subscriptions_.erase(iter);
            // break associations
            match_endpoints(to_ignore, td, true /*remove*/);
            if (td.is_dead()) {
              purge_dead_topic(topic_name);
            }
            return;
          }
        }
        {
          const OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, GUID_tKeyLessThan)::iterator
            iter = topic_names_.find(to_ignore);
          if (iter != topic_names_.end()) {
            ignored_topics_.insert(iter->second);
            // Remove all publications and subscriptions on this topic
            TopicDetails& td = topics_[iter->second];
            RepoIdSet ids = td.endpoints();
            for (RepoIdSet::iterator ep = ids.begin(); ep!= ids.end(); ++ep) {
              match_endpoints(*ep, td, true /*remove*/);
              td.remove_pub_sub(*ep);
              if (shutting_down()) { return; }
            }
            if (td.is_dead()) {
              purge_dead_topic(iter->second);
            }
          }
        }
      }

      bool ignoring(const RepoId& guid) const {
        return ignored_guids_.count(guid);
      }
      bool ignoring(const char* topic_name) const {
        return ignored_topics_.count(topic_name);
      }

      TopicStatus assert_topic(RepoId_out topicId, const char* topicName,
                                     const char* dataTypeName, const DDS::TopicQos& qos,
                                     bool hasDcpsKey, TopicCallbacks* topic_callbacks)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
        typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator iter =
          topics_.find(topicName);
        if (iter != topics_.end()) {
          if (iter->second.local_is_set() && iter->second.local_data_type_name() != dataTypeName) {
            return CONFLICTING_TYPENAME;
          }
          topicId = iter->second.topic_id();
          iter->second.set_local(dataTypeName, qos, hasDcpsKey, topic_callbacks);
          return FOUND;
        }

        TopicDetails& td = topics_[topicName];
        topicId = make_topic_guid();
        td.init(topicName, topicId);
        topic_names_[topicId] = topicName;
        td.set_local(dataTypeName, qos, hasDcpsKey, topic_callbacks);

        return CREATED;
      }

      TopicStatus find_topic(const char* topicName,
                                   CORBA::String_out dataTypeName,
                                   DDS::TopicQos_out qos,
                                   RepoId_out topicId)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
        typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::const_iterator iter =
          topics_.find(topicName);
        if (iter == topics_.end()) {
          return NOT_FOUND;
        }

        const TopicDetails& td = iter->second;

        dataTypeName = td.local_data_type_name().c_str();
        qos = new DDS::TopicQos(td.local_qos());
        topicId = td.topic_id();
        return FOUND;
      }

      TopicStatus remove_topic(const RepoId& topicId)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
        TopicNameMap::iterator name_iter = topic_names_.find(topicId);
        if (name_iter == topic_names_.end()) {
          return NOT_FOUND;
        }
        const OPENDDS_STRING& name = name_iter->second;
        TopicDetails& td = topics_[name];
        td.unset_local();
        if (td.is_dead()) {
          purge_dead_topic(name);
        }

        return REMOVED;
      }

      virtual bool update_topic_qos(const RepoId& topicId, const DDS::TopicQos& qos) = 0;

      RepoId add_publication(const RepoId& topicId,
                                   DataWriterCallbacks* publication,
                                   const DDS::DataWriterQos& qos,
                                   const TransportLocatorSeq& transInfo,
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

        const OPENDDS_STRING& topic_name = topic_names_[topicId];

#ifdef OPENDDS_SECURITY
        if (is_security_enabled()) {
          DDS::Security::SecurityException ex;

          DDS::Security::TopicSecurityAttributes topic_sec_attr;
          DDS::Security::PermissionsHandle permh = get_permissions_handle();
          if (!get_access_control()->get_topic_sec_attributes(permh, topic_name.data(), topic_sec_attr, ex)) {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: ")
              ACE_TEXT("DomainParticipant::add_publication, ")
              ACE_TEXT("Unable to get security attributes for topic '%C'. SecurityException[%d.%d]: %C\n"),
                topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
            return RepoId();
          }

          if (topic_sec_attr.is_write_protected == true) {
            if (!get_access_control()->check_create_datawriter(
                  permh, get_domain_id(), topic_name.data(), qos,
                  publisherQos.partition, DDS::Security::DataTagQosPolicy(), ex)) {
              ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("EndpointManager::add_publication() - ")
                ACE_TEXT("Permissions check failed for local datawriter on topic '%C'. ")
                ACE_TEXT("Security Exception[%d.%d]: %C\n"), topic_name.data(),
                  ex.code, ex.minor_code, ex.message.in()));
              return RepoId();
            }
          }

          if (!get_access_control()->get_datawriter_sec_attributes(permh, topic_name.data(),
              publisherQos.partition, DDS::Security::DataTagQosPolicy(), pb.security_attribs_, ex)) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("EndpointManager::add_publication() - ")
                       ACE_TEXT("Unable to get security attributes for local datawriter. ")
                       ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                       ex.code, ex.minor_code, ex.message.in()));
            return RepoId();
          }

          if (pb.security_attribs_.is_submessage_protected || pb.security_attribs_.is_payload_protected) {
            DDS::Security::DatawriterCryptoHandle handle =
              get_crypto_key_factory()->register_local_datawriter(
                crypto_handle_, DDS::PropertySeq(), pb.security_attribs_, ex);
            if (handle == DDS::HANDLE_NIL) {
              ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("EndpointManager::add_publication() - ")
                         ACE_TEXT("Unable to get local datawriter crypto handle. ")
                         ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                         ex.code, ex.minor_code, ex.message.in()));
            }

            local_writer_crypto_handles_[rid] = handle;
            local_writer_security_attribs_[rid] = pb.security_attribs_;
          }
        }
#endif

        TopicDetails& td = topics_[topic_name];
        td.add_pub_sub(rid);

        if (DDS::RETCODE_OK != add_publication_i(rid, pb)) {
          return RepoId();
        }

        if (DDS::RETCODE_OK != write_publication_data(rid, pb)) {
          return RepoId();
        }

        if (DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::add_publication - ")
                     ACE_TEXT("calling match_endpoints\n")));
        }
        match_endpoints(rid, td);

        return rid;
      }

      void remove_publication(const RepoId& publicationId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        LocalPublicationIter iter = local_publications_.find(publicationId);
        if (iter != local_publications_.end()) {
          if (DDS::RETCODE_OK == remove_publication_i(publicationId, iter->second)) {
            OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
            local_publications_.erase(publicationId);
            typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
              topics_.find(topic_name);
            if (top_it != topics_.end()) {
              match_endpoints(publicationId, top_it->second, true /*remove*/);
              top_it->second.remove_pub_sub(publicationId);
              // Local, no need to check for dead topic.
            }
          } else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: EndpointManager::remove_publication - ")
                       ACE_TEXT("Failed to publish dispose msg\n")));
          }
        }
      }

      virtual bool update_publication_qos(const RepoId& publicationId,
                                          const DDS::DataWriterQos& qos,
                                          const DDS::PublisherQos& publisherQos) = 0;

      void update_publication_locators(const RepoId& publicationId,
                                       const TransportLocatorSeq& transInfo)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        LocalPublicationIter iter = local_publications_.find(publicationId);
        if (iter != local_publications_.end()) {
          if (DCPS_debug_level > 3) {
            const GuidConverter conv(publicationId);
            ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) EndpointManager::update_publication_locators updating locators for %C\n"), OPENDDS_STRING(conv).c_str()));
          }
          iter->second.trans_info_ = transInfo;
          write_publication_data(publicationId, iter->second);
        }
      }

      RepoId add_subscription(const RepoId& topicId,
                                    DataReaderCallbacks* subscription,
                                    const DDS::DataReaderQos& qos,
                                    const TransportLocatorSeq& transInfo,
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

        const OPENDDS_STRING& topic_name = topic_names_[topicId];

#ifdef OPENDDS_SECURITY
        if (is_security_enabled()) {
          DDS::Security::SecurityException ex;

          DDS::Security::TopicSecurityAttributes topic_sec_attr;
          if (!get_access_control()->get_topic_sec_attributes(
              get_permissions_handle(), topic_name.data(), topic_sec_attr, ex)) {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: ")
              ACE_TEXT("DomainParticipant::add_subscription, ")
              ACE_TEXT("Unable to get security attributes for topic '%C'. ")
              ACE_TEXT("SecurityException[%d.%d]: %C\n"),
                topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
            return RepoId();
          }

          if (topic_sec_attr.is_read_protected == true) {
            if (!get_access_control()->check_create_datareader(
              get_permissions_handle(), get_domain_id(), topic_name.data(), qos,
              subscriberQos.partition, DDS::Security::DataTagQosPolicy(), ex)) {
              ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("EndpointManager::add_subscription() - ")
                ACE_TEXT("Permissions check failed for local datareader on topic '%C'. ")
                ACE_TEXT("Security Exception[%d.%d]: %C\n"), topic_name.data(),
                  ex.code, ex.minor_code, ex.message.in()));
              return RepoId();
            }
          }

          if (!get_access_control()->get_datareader_sec_attributes(get_permissions_handle(), topic_name.data(),
              subscriberQos.partition, DDS::Security::DataTagQosPolicy(), sb.security_attribs_, ex)) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("EndpointManager::add_subscription() - ")
                       ACE_TEXT("Unable to get security attributes for local datareader. ")
                       ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                       ex.code, ex.minor_code, ex.message.in()));
            return RepoId();
          }

          if (sb.security_attribs_.is_submessage_protected || sb.security_attribs_.is_payload_protected) {
            DDS::Security::DatareaderCryptoHandle handle =
              get_crypto_key_factory()->register_local_datareader(
                crypto_handle_, DDS::PropertySeq(), sb.security_attribs_, ex);
            if (handle == DDS::HANDLE_NIL) {
              ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("EndpointManager::add_subscription() - ")
                         ACE_TEXT("Unable to get local datareader crypto handle. ")
                         ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                         ex.code, ex.minor_code, ex.message.in()));
            }

            local_reader_crypto_handles_[rid] = handle;
            local_reader_security_attribs_[rid] = sb.security_attribs_;
          }
        }
#endif

        TopicDetails& td = topics_[topic_name];
        td.add_pub_sub(rid);

        if (DDS::RETCODE_OK != add_subscription_i(rid, sb)) {
          return RepoId();
        }

        if (DDS::RETCODE_OK != write_subscription_data(rid, sb)) {
          return RepoId();
        }

        if (DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::add_subscription - ")
                     ACE_TEXT("calling match_endpoints\n")));
        }
        match_endpoints(rid, td);

        return rid;
      }

      void remove_subscription(const RepoId& subscriptionId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
        if (iter != local_subscriptions_.end()) {
          if (DDS::RETCODE_OK == remove_subscription_i(subscriptionId, iter->second)) {
            OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
            local_subscriptions_.erase(subscriptionId);
            typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
              topics_.find(topic_name);
            if (top_it != topics_.end()) {
              match_endpoints(subscriptionId, top_it->second, true /*remove*/);
              top_it->second.remove_pub_sub(subscriptionId);
              // Local, no need to check for dead topic.
            }
          } else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: EndpointManager::remove_subscription - ")
                       ACE_TEXT("Failed to publish dispose msg\n")));
          }
        }
      }

      virtual bool update_subscription_qos(const RepoId& subscriptionId,
                                           const DDS::DataReaderQos& qos,
                                           const DDS::SubscriberQos& subscriberQos) = 0;

      virtual bool update_subscription_params(const RepoId& subId,
                                              const DDS::StringSeq& params) = 0;

      void update_subscription_locators(const RepoId& subscriptionId,
                                        const TransportLocatorSeq& transInfo)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
        if (iter != local_subscriptions_.end()) {
          if (DCPS_debug_level > 3) {
            const GuidConverter conv(subscriptionId);
            ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) EndpointManager::update_subscription_locators updating locators for %C\n"), OPENDDS_STRING(conv).c_str()));
          }
          iter->second.trans_info_ = transInfo;
          write_subscription_data(subscriptionId, iter->second);
        }
      }

      virtual void association_complete(const RepoId& localId,
                                        const RepoId& remoteId) = 0;

      virtual bool disassociate(const DiscoveredParticipantData& pdata) = 0;

    protected:
      struct LocalEndpoint {
        LocalEndpoint() : topic_id_(GUID_UNKNOWN), sequence_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
#ifdef OPENDDS_SECURITY
          , have_ice_agent_info(false)
        {
          security_attribs_.base.is_read_protected = false;
          security_attribs_.base.is_write_protected = false;
          security_attribs_.base.is_discovery_protected = false;
          security_attribs_.base.is_liveliness_protected = false;
          security_attribs_.is_submessage_protected = false;
          security_attribs_.is_payload_protected = false;
          security_attribs_.is_key_protected = false;
          security_attribs_.plugin_endpoint_attributes = 0;
        }
#else
        {}
#endif

        RepoId topic_id_;
        TransportLocatorSeq trans_info_;
        RepoIdSet matched_endpoints_;
        SequenceNumber sequence_;
        RepoIdSet remote_expectant_opendds_associations_;
#ifdef OPENDDS_SECURITY
        bool have_ice_agent_info;
        ICE::AgentInfo ice_agent_info;
        DDS::Security::EndpointSecurityAttributes security_attribs_;
#endif
      };

      struct LocalPublication : LocalEndpoint {
        DataWriterCallbacks* publication_;
        DDS::DataWriterQos qos_;
        DDS::PublisherQos publisher_qos_;
      };

      struct LocalSubscription : LocalEndpoint {
        DataReaderCallbacks* subscription_;
        DDS::DataReaderQos qos_;
        DDS::SubscriberQos subscriber_qos_;
        ContentFilterProperty_t filterProperties;
      };

      typedef OPENDDS_MAP_CMP(DDS::BuiltinTopicKey_t, RepoId,
                              BuiltinTopicKeyLess) BitKeyMap;

      typedef OPENDDS_MAP_CMP(RepoId, LocalPublication,
                              GUID_tKeyLessThan) LocalPublicationMap;
      typedef typename LocalPublicationMap::iterator LocalPublicationIter;
      typedef typename LocalPublicationMap::const_iterator LocalPublicationCIter;

      typedef OPENDDS_MAP_CMP(RepoId, LocalSubscription,
                              GUID_tKeyLessThan) LocalSubscriptionMap;
      typedef typename LocalSubscriptionMap::iterator LocalSubscriptionIter;
      typedef typename LocalSubscriptionMap::const_iterator LocalSubscriptionCIter;

      typedef typename OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, GUID_tKeyLessThan) TopicNameMap;

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
          ? ENTITYKIND_USER_WRITER_WITH_KEY
          : ENTITYKIND_USER_WRITER_NO_KEY;
        assign(rid.entityId.entityKey, publication_counter_++);
      }
      virtual void assign_subscription_key(RepoId& rid,
                                           const RepoId& topicId,
                                           const DDS::DataReaderQos& /*qos*/) {
        rid.entityId.entityKind =
          has_dcps_key(topicId)
          ? ENTITYKIND_USER_READER_WITH_KEY
          : ENTITYKIND_USER_READER_NO_KEY;
        assign(rid.entityId.entityKey, subscription_counter_++);
      }
      virtual void assign_topic_key(RepoId& guid) {
        assign(guid.entityId.entityKey, topic_counter_++);

        if (topic_counter_ == 0x1000000) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: EndpointManager::make_topic_guid: ")
                     ACE_TEXT("Exceeded Maximum number of topic entity keys!")
                     ACE_TEXT("Next key will be a duplicate!\n")));
          topic_counter_ = 0;
        }
      }

      virtual DDS::ReturnCode_t add_publication_i(const RepoId& /*rid*/,
                                                  LocalPublication& /*pub*/)
      { return DDS::RETCODE_OK; }

      virtual DDS::ReturnCode_t write_publication_data(const RepoId& /*rid*/,
                                                       LocalPublication& /*pub*/,
                                                       const RepoId& reader = GUID_UNKNOWN)
      { ACE_UNUSED_ARG(reader); return DDS::RETCODE_OK; }

      virtual DDS::ReturnCode_t remove_publication_i(const RepoId& publicationId,
                                                     LocalPublication& /*pub*/) = 0;

      virtual DDS::ReturnCode_t add_subscription_i(const RepoId& /*rid*/,
                                                   LocalSubscription& /*pub*/)
      { return DDS::RETCODE_OK; };

      virtual DDS::ReturnCode_t write_subscription_data(const RepoId& /*rid*/,
                                                        LocalSubscription& /*pub*/,
                                                        const RepoId& reader = GUID_UNKNOWN)
      { ACE_UNUSED_ARG(reader); return DDS::RETCODE_OK; }

      virtual DDS::ReturnCode_t remove_subscription_i(const RepoId& subscriptionId, LocalSubscription& /*sub*/) = 0;

      void match_endpoints(RepoId repoId, const TopicDetails& td,
                           bool remove = false)
      {
        const bool reader = repoId.entityId.entityKind & 4;
        // Copy the endpoint set - lock can be released in match()
        RepoIdSet endpoints_copy = td.endpoints();

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
            WriterIdSeq writer_seq(1);
            writer_seq.length(1);
            writer_seq[0] = removing;
            const size_t count = lsi->second.remote_expectant_opendds_associations_.erase(removing);
            lsi->second.subscription_->remove_associations(writer_seq,
                                                           false /*notify_lost*/);
            remove_assoc_i(remove_from, lsi->second, removing);
            // Update writer
            if (count) {
              write_subscription_data(remove_from, lsi->second);
            }
          }

        } else {
          const LocalPublicationIter lpi = local_publications_.find(remove_from);
          if (lpi != local_publications_.end()) {
            lpi->second.matched_endpoints_.erase(removing);
            ReaderIdSeq reader_seq(1);
            reader_seq.length(1);
            reader_seq[0] = removing;
            lpi->second.remote_expectant_opendds_associations_.erase(removing);
            lpi->second.publication_->remove_associations(reader_seq,
                                                          false /*notify_lost*/);
            remove_assoc_i(remove_from, lpi->second, removing);
          }
        }
      }

      virtual void add_assoc_i(const RepoId& /* local_guid */, const LocalPublication& /* lpub */,
                               const RepoId& /* remote_guid */, const DiscoveredSubscription& /* dsub */) {}
      virtual void remove_assoc_i(const RepoId& /* local_guid */, const LocalPublication& /* lpub */,
                                  const RepoId& /* remote_guid */) {}
      virtual void add_assoc_i(const RepoId& /* local_guid */, const LocalSubscription& /* lsub */,
                               const RepoId& /* remote_guid */, const DiscoveredPublication& /* dpub */) {}
      virtual void remove_assoc_i(const RepoId& /* local_guid */, const LocalSubscription& /* lsub */,
                                  const RepoId& /* remote_guid */) {}

#ifdef OPENDDS_SECURITY
      virtual DDS::Security::DatawriterCryptoHandle
      generate_remote_matched_writer_crypto_handle(const RepoId&, const DDS::Security::DatareaderCryptoHandle&)
      {
        return DDS::HANDLE_NIL;
      }

      virtual DDS::Security::DatareaderCryptoHandle
      generate_remote_matched_reader_crypto_handle(const RepoId&, const DDS::Security::DatawriterCryptoHandle&, bool)
      {
        return DDS::HANDLE_NIL;
      }

      virtual void
      create_and_send_datareader_crypto_tokens(
        const DDS::Security::DatareaderCryptoHandle&, const RepoId&,
        const DDS::Security::DatawriterCryptoHandle&, const RepoId&)
      {
        return;
      }

      virtual void
      create_and_send_datawriter_crypto_tokens(
        const DDS::Security::DatawriterCryptoHandle&, const RepoId&,
        const DDS::Security::DatareaderCryptoHandle&, const RepoId&)
      {
        return;
      }
#endif

      virtual DDS::DomainId_t
      get_domain_id() const
      {
        return -1;
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
        TransportLocatorSeq* wTls = 0;

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
        TransportLocatorSeq* rTls = 0;
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
        DataWriterCallbacks* dwr = 0;
        DataReaderCallbacks* drr = 0;
        if (writer_local) {
          dwr = lpi->second.publication_;
        }
        if (reader_local) {
          drr = lsi->second.subscription_;
        }

        IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
        IncompatibleQosStatus readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};

        if (compatibleQOS(&writerStatus, &readerStatus, *wTls, *rTls,
                                dwQos, drQos, pubQos, subQos)) {

          bool call_writer = false, call_reader = false;
          if (writer_local) {
            call_writer = lpi->second.matched_endpoints_.insert(reader).second;
          }
          if (reader_local) {
            call_reader = lsi->second.matched_endpoints_.insert(writer).second;
          }

          if (writer_local && !reader_local) {
            add_assoc_i(writer, lpi->second, reader, dsi->second);
          }
          if (reader_local && !writer_local) {
            add_assoc_i(reader, lsi->second, writer, dpi->second);
          }

          if (!call_writer && !call_reader) {
            return; // nothing more to do
          }

#ifdef OPENDDS_SECURITY
          if (is_security_enabled()) {
            DDS::Security::CryptoKeyExchange_var keyexg = get_crypto_key_exchange();
            if (call_reader) {
              RepoId writer_participant = writer;
              writer_participant.entityId = ENTITYID_PARTICIPANT;
              DatareaderCryptoHandleMap::const_iterator iter =
                local_reader_crypto_handles_.find(reader);

              // It might not exist due to security attributes, and that's OK
              if (iter != local_reader_crypto_handles_.end()) {
                DDS::Security::DatareaderCryptoHandle drch = iter->second;
                DDS::Security::DatawriterCryptoHandle dwch =
                  generate_remote_matched_writer_crypto_handle(writer_participant, drch);
                remote_writer_crypto_handles_[writer] = dwch;
                DatawriterCryptoTokenSeqMap::iterator t_iter =
                  pending_remote_writer_crypto_tokens_.find(writer);
                if (t_iter != pending_remote_writer_crypto_tokens_.end()) {
                  DDS::Security::SecurityException se;
                  if (!keyexg->set_remote_datawriter_crypto_tokens(iter->second, dwch, t_iter->second, se)) {
                    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("(%P|%t) ERROR: DiscoveryBase::match() - ")
                      ACE_TEXT("Unable to set pending remote datawriter crypto tokens with ")
                      ACE_TEXT("crypto key exchange plugin. Security Exception[%d.%d]: %C\n"),
                        se.code, se.minor_code, se.message.in()));
                  }
                  pending_remote_writer_crypto_tokens_.erase(t_iter);
                }
                EndpointSecurityAttributesMap::const_iterator s_iter =
                  local_reader_security_attribs_.find(reader);
                // Yes, this is different for remote datawriters than readers (see 8.8.9.3 vs 8.8.9.2)
                if (s_iter != local_reader_security_attribs_.end() && s_iter->second.is_submessage_protected) {
                  create_and_send_datareader_crypto_tokens(drch, reader, dwch, writer);
                }
              }
            }

            if (call_writer) {
              RepoId reader_participant = reader;
              reader_participant.entityId = ENTITYID_PARTICIPANT;
              DatawriterCryptoHandleMap::const_iterator iter =
                local_writer_crypto_handles_.find(writer);

              // It might not exist due to security attributes, and that's OK
              if (iter != local_writer_crypto_handles_.end()) {
                DDS::Security::DatawriterCryptoHandle dwch = iter->second;
                DDS::Security::DatareaderCryptoHandle drch =
                  generate_remote_matched_reader_crypto_handle(
                    reader_participant, dwch, relay_only_readers_.count(reader));
                remote_reader_crypto_handles_[reader] = drch;
                DatareaderCryptoTokenSeqMap::iterator t_iter =
                  pending_remote_reader_crypto_tokens_.find(reader);
                if (t_iter != pending_remote_reader_crypto_tokens_.end()) {
                  DDS::Security::SecurityException se;
                  if (!keyexg->set_remote_datareader_crypto_tokens(iter->second, drch, t_iter->second, se)) {
                    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("(%P|%t) ERROR: DiscoveryBase::match() - ")
                      ACE_TEXT("Unable to set pending remote datareader crypto tokens with crypto ")
                      ACE_TEXT("key exchange plugin. Security Exception[%d.%d]: %C\n"),
                        se.code, se.minor_code, se.message.in()));
                  }
                  pending_remote_reader_crypto_tokens_.erase(t_iter);
                }
                EndpointSecurityAttributesMap::const_iterator s_iter =
                  local_writer_security_attribs_.find(writer);
                if (s_iter != local_writer_security_attribs_.end() &&
                    (s_iter->second.is_submessage_protected || s_iter->second.is_payload_protected)) {
                  create_and_send_datawriter_crypto_tokens(dwch, writer, drch, reader);
                }
              }
            }
          }
#endif

          // Copy reader and writer association data prior to releasing lock
#ifdef __SUNPRO_CC
          ReaderAssociation ra;
          ra.readerTransInfo = *rTls;
          ra.readerId = reader;
          ra.subQos = *subQos;
          ra.readerQos = *drQos;
          ra.filterClassName = cfProp->filterClassName;
          ra.filterExpression = cfProp->filterExpression;
          ra.exprParams = cfProp->expressionParameters;
          WriterAssociation wa;
          wa.writerTransInfo = *wTls;
          wa.writerId = writer;
          wa.pubQos = *pubQos;
          wa.writerQos = *dwQos;
#else
          const ReaderAssociation ra =
            {add_security_info(*rTls, writer, reader), reader, *subQos, *drQos,
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
             cfProp->filterClassName, cfProp->filterExpression,
#else
             "", "",
#endif
             cfProp->expressionParameters};

          const WriterAssociation wa =
            {add_security_info(*wTls, writer, reader), writer, *pubQos, *dwQos};
#endif

          ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
          static const bool writer_active = true;

          if (call_writer) {
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("adding writer %C association for reader %C\n"), OPENDDS_STRING(GuidConverter(writer)).c_str(), OPENDDS_STRING(GuidConverter(reader)).c_str()));
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
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("adding reader %C association for writer %C\n"), OPENDDS_STRING(GuidConverter(reader)).c_str(), OPENDDS_STRING(GuidConverter(writer)).c_str()));
            }
            drr->add_association(reader, wa, !writer_active);
          }

          // change this if 'writer_active' (above) changes
          if (call_writer && !call_reader && !is_expectant_opendds(reader)) {
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG,
                         ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("calling writer %C association_complete for %C\n"), OPENDDS_STRING(GuidConverter(writer)).c_str(), OPENDDS_STRING(GuidConverter(reader)).c_str()));
            }
            dwr->association_complete(reader);
          }

        } else if (already_matched) { // break an existing associtaion
          if (writer_local) {
            lpi->second.matched_endpoints_.erase(reader);
            lpi->second.remote_expectant_opendds_associations_.erase(reader);
          }
          if (reader_local) {
            lsi->second.matched_endpoints_.erase(writer);
            lsi->second.remote_expectant_opendds_associations_.erase(writer);
          }
          if (writer_local && !reader_local) {
            remove_assoc_i(writer, lpi->second, reader);
          }
          if (reader_local && !writer_local) {
            remove_assoc_i(reader, lsi->second, writer);
          }
          ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
          if (writer_local) {
            ReaderIdSeq reader_seq(1);
            reader_seq.length(1);
            reader_seq[0] = reader;
            dwr->remove_associations(reader_seq, false /*notify_lost*/);
          }
          if (reader_local) {
            WriterIdSeq writer_seq(1);
            writer_seq.length(1);
            writer_seq[0] = writer;
            drr->remove_associations(writer_seq, false /*notify_lost*/);
          }

        } else { // something was incompatible
          ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
          if (writer_local && writerStatus.count_since_last_send) {
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("writer incompatible\n")));
            }
            dwr->update_incompatible_qos(writerStatus);
          }
          if (reader_local && readerStatus.count_since_last_send) {
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match - ")
                         ACE_TEXT("reader incompatible\n")));
            }
            drr->update_incompatible_qos(readerStatus);
          }
        }
      }

      virtual bool is_expectant_opendds(const GUID_t& endpoint) const = 0;

      virtual bool shutting_down() const = 0;

      virtual void populate_transport_locator_sequence(TransportLocatorSeq*& tls,
                                                       DiscoveredSubscriptionIter& iter,
                                                       const RepoId& reader) = 0;

      virtual void populate_transport_locator_sequence(TransportLocatorSeq*& tls,
                                                       DiscoveredPublicationIter& iter,
                                                       const RepoId& reader) = 0;

      virtual TransportLocatorSeq
      add_security_info(const TransportLocatorSeq& locators,
                        const RepoId& /*writer*/, const RepoId& /*reader*/)
      { return locators; }

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
        guid.entityId.entityKind = ENTITYKIND_OPENDDS_TOPIC;
        assign_topic_key(guid);
        return guid;
      }

      bool has_dcps_key(const RepoId& topicId) const
      {
        typedef OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, GUID_tKeyLessThan) TNMap;
        TNMap::const_iterator tn = topic_names_.find(topicId);
        if (tn == topic_names_.end()) return false;

        typedef OPENDDS_MAP(OPENDDS_STRING, TopicDetails) TDMap;
        typename TDMap::const_iterator td = topics_.find(tn->second);
        if (td == topics_.end()) return false;

        return td->second.has_dcps_key();
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

#ifdef OPENDDS_SECURITY
      inline bool is_security_enabled()
      {
        return (permissions_handle_ != DDS::HANDLE_NIL) && (access_control_ != 0);
      }

      inline void set_permissions_handle(DDS::Security::PermissionsHandle h)
      {
        permissions_handle_ = h;
      }

      inline DDS::Security::PermissionsHandle get_permissions_handle() const
      {
        return permissions_handle_;
      }

      inline void set_access_control(DDS::Security::AccessControl_var acl)
      {
        access_control_ = acl;
      }

      inline DDS::Security::AccessControl_var get_access_control() const
      {
        return access_control_;
      }

      inline void set_crypto_key_factory(DDS::Security::CryptoKeyFactory_var ckf)
      {
        crypto_key_factory_ = ckf;
      }

      inline DDS::Security::CryptoKeyFactory_var get_crypto_key_factory() const
      {
        return crypto_key_factory_;
      }

      inline void set_crypto_key_exchange(DDS::Security::CryptoKeyExchange_var ckf)
      {
        crypto_key_exchange_ = ckf;
      }

      inline DDS::Security::CryptoKeyExchange_var get_crypto_key_exchange() const
      {
        return crypto_key_exchange_;
      }
#endif

      ACE_Thread_Mutex& lock_;
      RepoId participant_id_;
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
      OPENDDS_SET_CMP(RepoId, GUID_tKeyLessThan) relay_only_readers_;
      DDS::BuiltinTopicKey_t pub_bit_key_, sub_bit_key_;

#ifdef OPENDDS_SECURITY
      DDS::Security::AccessControl_var access_control_;
      DDS::Security::CryptoKeyFactory_var crypto_key_factory_;
      DDS::Security::CryptoKeyExchange_var crypto_key_exchange_;

      DDS::Security::PermissionsHandle permissions_handle_;
      DDS::Security::ParticipantCryptoHandle crypto_handle_;

      DatareaderCryptoHandleMap local_reader_crypto_handles_;
      DatawriterCryptoHandleMap local_writer_crypto_handles_;

      EndpointSecurityAttributesMap local_reader_security_attribs_;
      EndpointSecurityAttributesMap local_writer_security_attribs_;

      DatareaderCryptoHandleMap remote_reader_crypto_handles_;
      DatawriterCryptoHandleMap remote_writer_crypto_handles_;

      DatareaderCryptoTokenSeqMap pending_remote_reader_crypto_tokens_;
      DatawriterCryptoTokenSeqMap pending_remote_writer_crypto_tokens_;
#endif

    };

    template <typename EndpointManagerType>
    class LocalParticipant : public RcObject {
    public:
      typedef typename EndpointManagerType::DiscoveredParticipantData DiscoveredParticipantData;
      typedef typename EndpointManagerType::TopicDetails TopicDetails;

      LocalParticipant (const DDS::DomainParticipantQos& qos)
        : qos_(qos)
      { }

      virtual ~LocalParticipant() { }

      RepoId bit_key_to_repo_id(const char* bit_topic_name,
                                      const DDS::BuiltinTopicKey_t& key)
      {
        if (0 == std::strcmp(bit_topic_name, BUILT_IN_PARTICIPANT_TOPIC)) {
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

      virtual bool
      announce_domain_participant_qos()
      {
        return true;
      }

      bool
      update_domain_participant_qos(const DDS::DomainParticipantQos& qos)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
        qos_ = qos;
        return announce_domain_participant_qos();
      }

      TopicStatus
      assert_topic(RepoId_out topicId, const char* topicName,
                   const char* dataTypeName, const DDS::TopicQos& qos,
                   bool hasDcpsKey, TopicCallbacks* topic_callbacks)
      {
        if (std::strlen(topicName) > 256 || std::strlen(dataTypeName) > 256) {
          if (DCPS_debug_level) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR LocalParticipant::assert_topic() - ")
                       ACE_TEXT("topic or type name length limit (256) exceeded\n")));
          }
          return PRECONDITION_NOT_MET;
        }

        return endpoint_manager().assert_topic(topicId, topicName, dataTypeName, qos, hasDcpsKey, topic_callbacks);
      }

      TopicStatus
      find_topic(const char* topicName,
                 CORBA::String_out dataTypeName,
                 DDS::TopicQos_out qos,
                 RepoId_out topicId)
      {
        return endpoint_manager().find_topic(topicName, dataTypeName, qos, topicId);
      }

      TopicStatus
      remove_topic(const RepoId& topicId)
      {
        return endpoint_manager().remove_topic(topicId);
      }

      void
      ignore_topic(const RepoId& ignoreId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        endpoint_manager().ignore(ignoreId);
      }

      bool
      update_topic_qos(const RepoId& topicId, const DDS::TopicQos& qos)
      {
        return endpoint_manager().update_topic_qos(topicId, qos);
      }

      RepoId
      add_publication(const RepoId& topicId,
                      DataWriterCallbacks* publication,
                      const DDS::DataWriterQos& qos,
                      const TransportLocatorSeq& transInfo,
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

      void update_publication_locators(const RepoId& publicationId,
                                       const TransportLocatorSeq& transInfo)
      {
        endpoint_manager().update_publication_locators(publicationId, transInfo);
      }

      RepoId
      add_subscription(const RepoId& topicId,
                       DataReaderCallbacks* subscription,
                       const DDS::DataReaderQos& qos,
                       const TransportLocatorSeq& transInfo,
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
      update_subscription_locators(const RepoId& subId,
                                   const TransportLocatorSeq& transInfo)
      {
        endpoint_manager().update_subscription_locators(subId, transInfo);
      }

      void
      association_complete(const RepoId& localId, const RepoId& remoteId)
      {
        endpoint_manager().association_complete(localId, remoteId);
      }

      DDS::Subscriber_var bit_subscriber() const { return bit_subscriber_; }

    protected:

      struct DiscoveredParticipant {

        DiscoveredParticipant()
        : location_ih_(DDS::HANDLE_NIL)
        , bit_ih_(DDS::HANDLE_NIL)
        , seq_reset_count_(0)
#ifdef OPENDDS_SECURITY
        , has_last_stateless_msg_(false)
        , auth_state_(AS_UNKNOWN)
        , identity_handle_(DDS::HANDLE_NIL)
        , handshake_handle_(DDS::HANDLE_NIL)
        , permissions_handle_(DDS::HANDLE_NIL)
        , crypto_handle_(DDS::HANDLE_NIL)
#endif
        {
#ifdef OPENDDS_SECURITY
          security_info_.participant_security_attributes = 0;
          security_info_.plugin_participant_security_attributes = 0;
#endif
        }

        DiscoveredParticipant(
          const DiscoveredParticipantData& p,
          const MonotonicTimePoint& t,
          const SequenceNumber& seq)
        : pdata_(p)
        , location_ih_(DDS::HANDLE_NIL)
        , last_seen_(t)
        , bit_ih_(DDS::HANDLE_NIL)
        , last_seq_(seq)
        , seq_reset_count_(0)
#ifdef OPENDDS_SECURITY
        , has_last_stateless_msg_(false)
        , auth_state_(AS_UNKNOWN)
        , identity_handle_(DDS::HANDLE_NIL)
        , handshake_handle_(DDS::HANDLE_NIL)
        , permissions_handle_(DDS::HANDLE_NIL)
        , crypto_handle_(DDS::HANDLE_NIL)
#endif
        {
          RepoId guid;
          std::memcpy(guid.guidPrefix, p.participantProxy.guidPrefix, sizeof(p.participantProxy.guidPrefix));
          guid.entityId = DCPS::ENTITYID_PARTICIPANT;
          std::memcpy(location_data_.guid, &guid, sizeof(guid));
          location_data_.location = 0;
          location_data_.change_mask = 0;
          location_data_.local_timestamp.sec = 0;
          location_data_.local_timestamp.nanosec = 0;
          location_data_.ice_timestamp.sec = 0;
          location_data_.ice_timestamp.nanosec = 0;
          location_data_.relay_timestamp.sec = 0;
          location_data_.relay_timestamp.nanosec = 0;

#ifdef OPENDDS_SECURITY
          security_info_.participant_security_attributes = 0;
          security_info_.plugin_participant_security_attributes = 0;
#endif
        }

        DiscoveredParticipantData pdata_;
        struct LocationUpdate {
          ParticipantLocation mask_;
          ACE_INET_Addr from_;
          LocationUpdate() {}
          LocationUpdate(ParticipantLocation mask,
                         const ACE_INET_Addr& from)
            : mask_(mask), from_(from) {}
        };
        typedef OPENDDS_VECTOR(LocationUpdate) LocationUpdateList;
        LocationUpdateList location_updates_;
        ParticipantLocationBuiltinTopicData location_data_;
        DDS::InstanceHandle_t location_ih_;

        MonotonicTimePoint last_seen_;
        DDS::InstanceHandle_t bit_ih_;
        SequenceNumber last_seq_;
        ACE_UINT16 seq_reset_count_;

#ifdef OPENDDS_SECURITY
        bool has_last_stateless_msg_;
        MonotonicTimePoint stateless_msg_deadline_;
        DDS::Security::ParticipantStatelessMessage last_stateless_msg_;

        MonotonicTimePoint auth_deadline_;
        AuthState auth_state_;

        DDS::Security::IdentityToken identity_token_;
        DDS::Security::PermissionsToken permissions_token_;
        DDS::Security::PropertyQosPolicy property_qos_;
        DDS::Security::ParticipantSecurityInfo security_info_;
        DDS::Security::IdentityStatusToken identity_status_token_;
        DDS::Security::IdentityHandle identity_handle_;
        DDS::Security::HandshakeHandle handshake_handle_;
        DDS::Security::AuthRequestMessageToken local_auth_request_token_;
        DDS::Security::AuthRequestMessageToken remote_auth_request_token_;
        DDS::Security::AuthenticatedPeerCredentialToken authenticated_peer_credential_token_;
        DDS::Security::SharedSecretHandle_var shared_secret_handle_;
        DDS::Security::PermissionsHandle permissions_handle_;
        DDS::Security::ParticipantCryptoHandle crypto_handle_;
        DDS::Security::ParticipantCryptoTokenSeq crypto_tokens_;
#endif
      };

      typedef OPENDDS_MAP_CMP(RepoId, DiscoveredParticipant,
                              GUID_tKeyLessThan) DiscoveredParticipantMap;
      typedef typename DiscoveredParticipantMap::iterator DiscoveredParticipantIter;
      typedef typename DiscoveredParticipantMap::const_iterator
        DiscoveredParticipantConstIter;

#ifdef OPENDDS_SECURITY
      typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::AuthRequestMessageToken, GUID_tKeyLessThan)
        PendingRemoteAuthTokenMap;
#endif

      virtual EndpointManagerType& endpoint_manager() = 0;

      void remove_discovered_participant(DiscoveredParticipantIter iter)
      {
        RepoId part_id = iter->first;
        bool removed = endpoint_manager().disassociate(iter->second.pdata_);
        iter = participants_.find(part_id); // refresh iter after disassociate, which can unlock
        if (iter == participants_.end()) {
          return;
        }
        if (removed) {
#ifndef DDS_HAS_MINIMUM_BIT
          ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
          // bit may be null if the DomainParticipant is shutting down
          if (bit && iter->second.bit_ih_ != DDS::HANDLE_NIL) {
            bit->set_instance_state(iter->second.bit_ih_,
                                    DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
          }
          ParticipantLocationBuiltinTopicDataDataReaderImpl* loc_bit = part_loc_bit();
          // bit may be null if the DomainParticipant is shutting down
          if (loc_bit && iter->second.location_ih_ != DDS::HANDLE_NIL) {
            loc_bit->set_instance_state(iter->second.location_ih_,
                                        DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
          }
#endif /* DDS_HAS_MINIMUM_BIT */
          if (DCPS_debug_level > 3) {
            GuidConverter conv(iter->first);
            ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) LocalParticipant::remove_discovered_participant")
                       ACE_TEXT(" - erasing %C\n"), OPENDDS_STRING(conv).c_str()));
          }
          participants_.erase(iter);
        }
      }

#ifndef DDS_HAS_MINIMUM_BIT
    DCPS::ParticipantBuiltinTopicDataDataReaderImpl* part_bit()
    {
      if (!bit_subscriber_.in())
        return 0;

      DDS::DataReader_var d =
        bit_subscriber_->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
      return dynamic_cast<ParticipantBuiltinTopicDataDataReaderImpl*>(d.in());
    }

    DCPS::ParticipantLocationBuiltinTopicDataDataReaderImpl* part_loc_bit()
    {
      if (!bit_subscriber_.in())
        return 0;

      DDS::DataReader_var d =
        bit_subscriber_->lookup_datareader(DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
      return dynamic_cast<ParticipantLocationBuiltinTopicDataDataReaderImpl*>(d.in());
    }
#endif /* DDS_HAS_MINIMUM_BIT */

      mutable ACE_Thread_Mutex lock_;
      DDS::Subscriber_var bit_subscriber_;
      DDS::DomainParticipantQos qos_;
      DiscoveredParticipantMap participants_;

#ifdef OPENDDS_SECURITY
      PendingRemoteAuthTokenMap pending_remote_auth_tokens_;
#endif

    };

    template<typename Participant>
    class PeerDiscovery : public Discovery {
    public:
      typedef typename Participant::TopicDetails TopicDetails;

      explicit PeerDiscovery(const RepoKey& key) : Discovery(key) { }

      virtual DDS::Subscriber_ptr init_bit(DomainParticipantImpl* participant) {
        DDS::Subscriber_var bit_subscriber;
#ifndef DDS_HAS_MINIMUM_BIT
        if (!TheServiceParticipant->get_BIT()) {
          get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber);
          return 0;
        }

        if (create_bit_topics(participant) != DDS::RETCODE_OK) {
          return 0;
        }

        bit_subscriber =
          participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                         DDS::SubscriberListener::_nil(),
                                         DEFAULT_STATUS_MASK);
        SubscriberImpl* sub = dynamic_cast<SubscriberImpl*>(bit_subscriber.in());
        if (sub == 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::init_bit")
                     ACE_TEXT(" - Could not cast Subscriber to SubscriberImpl\n")));
          return 0;
        }

        DDS::DataReaderQos dr_qos;
        sub->get_default_datareader_qos(dr_qos);
        dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

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

    DDS::TopicDescription_var bit_part_loc_topic =
      participant->lookup_topicdescription(BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    create_bit_dr(bit_part_loc_topic, BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE,
      sub, dr_qos);

        const DDS::ReturnCode_t ret = bit_subscriber->enable();
        if (ret != DDS::RETCODE_OK) {
          if (DCPS_debug_level) {
            ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) PeerDiscovery::init_bit")
                       ACE_TEXT(" - Error %d enabling subscriber\n"), ret));
          }
          return 0;
        }
#endif /* DDS_HAS_MINIMUM_BIT */

        get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber);

        return bit_subscriber._retn();
      }

      virtual void fini_bit(DomainParticipantImpl* participant)
      {
        get_part(participant->get_domain_id(), participant->get_id())->fini_bit();
      }

      virtual RepoId bit_key_to_repo_id(DomainParticipantImpl* participant,
                                                       const char* bit_topic_name,
                                                       const DDS::BuiltinTopicKey_t& key) const
      {
        return get_part(participant->get_domain_id(), participant->get_id())
          ->bit_key_to_repo_id(bit_topic_name, key);
      }

      virtual bool attach_participant(DDS::DomainId_t /*domainId*/,
                                      const RepoId& /*participantId*/)
      {
        return false; // This is just for DCPSInfoRepo?
      }

      virtual bool remove_domain_participant(DDS::DomainId_t domain_id,
                                             const RepoId& participantId)
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
                                             const RepoId& myParticipantId,
                                             const RepoId& ignoreId)
      {
        get_part(domain, myParticipantId)->ignore_domain_participant(ignoreId);
        return true;
      }

      virtual bool update_domain_participant_qos(DDS::DomainId_t domain,
                                                 const RepoId& participant,
                                                 const DDS::DomainParticipantQos& qos)
      {
        return get_part(domain, participant)->update_domain_participant_qos(qos);
      }

      virtual TopicStatus assert_topic(RepoId_out topicId,
                                             DDS::DomainId_t domainId,
                                             const RepoId& participantId,
                                             const char* topicName,
                                             const char* dataTypeName,
                                             const DDS::TopicQos& qos,
                                             bool hasDcpsKey,
                                             TopicCallbacks* topic_callbacks)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
        // Verified its safe to hold lock during call to assert_topic
        return participants_[domainId][participantId]->assert_topic(topicId, topicName,
                                                                    dataTypeName, qos,
                                                                    hasDcpsKey, topic_callbacks);
      }

      virtual TopicStatus find_topic(DDS::DomainId_t domainId,
                                           const RepoId& participantId,
                                           const char* topicName,
                                           CORBA::String_out dataTypeName,
                                           DDS::TopicQos_out qos,
                                           RepoId_out topicId)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
        return participants_[domainId][participantId]->find_topic(topicName, dataTypeName, qos, topicId);
      }

      virtual TopicStatus remove_topic(DDS::DomainId_t domainId,
                                             const RepoId& participantId,
                                             const RepoId& topicId)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
        // Safe to hold lock while calling remove topic
        return participants_[domainId][participantId]->remove_topic(topicId);
      }

      virtual bool ignore_topic(DDS::DomainId_t domainId, const RepoId& myParticipantId,
                                const RepoId& ignoreId)
      {
        get_part(domainId, myParticipantId)->ignore_topic(ignoreId);
        return true;
      }

      virtual bool update_topic_qos(const RepoId& topicId, DDS::DomainId_t domainId,
                                    const RepoId& participantId, const DDS::TopicQos& qos)
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
        // Safe to hold lock while calling update_topic_qos
        return participants_[domainId][participantId]->update_topic_qos(topicId, qos);
      }

      virtual RepoId add_publication(DDS::DomainId_t domainId,
                                                    const RepoId& participantId,
                                                    const RepoId& topicId,
                                                    DataWriterCallbacks* publication,
                                                    const DDS::DataWriterQos& qos,
                                                    const TransportLocatorSeq& transInfo,
                                                    const DDS::PublisherQos& publisherQos)
      {
        return get_part(domainId, participantId)->add_publication(topicId, publication, qos, transInfo, publisherQos);
      }

      virtual bool remove_publication(DDS::DomainId_t domainId,
                                      const RepoId& participantId,
                                      const RepoId& publicationId)
      {
        get_part(domainId, participantId)->remove_publication(publicationId);
        return true;
      }

      virtual bool ignore_publication(DDS::DomainId_t domainId,
                                      const RepoId& participantId,
                                      const RepoId& ignoreId)
      {
        get_part(domainId, participantId)->ignore_publication(ignoreId);
        return true;
      }

      virtual bool update_publication_qos(DDS::DomainId_t domainId,
                                          const RepoId& partId,
                                          const RepoId& dwId,
                                          const DDS::DataWriterQos& qos,
                                          const DDS::PublisherQos& publisherQos)
      {
        return get_part(domainId, partId)->update_publication_qos(dwId, qos,
                                                                  publisherQos);
      }

      virtual void update_publication_locators(DDS::DomainId_t domainId,
                                               const RepoId& partId,
                                               const RepoId& dwId,
                                               const TransportLocatorSeq& transInfo)
      {
        get_part(domainId, partId)->update_publication_locators(dwId, transInfo);
      }

      virtual RepoId add_subscription(DDS::DomainId_t domainId,
                                                     const RepoId& participantId,
                                                     const RepoId& topicId,
                                                     DataReaderCallbacks* subscription,
                                                     const DDS::DataReaderQos& qos,
                                                     const TransportLocatorSeq& transInfo,
                                                     const DDS::SubscriberQos& subscriberQos,
                                                     const char* filterClassName,
                                                     const char* filterExpr,
                                                     const DDS::StringSeq& params)
      {
        return get_part(domainId, participantId)->
          add_subscription(
            topicId, subscription, qos, transInfo, subscriberQos, filterClassName, filterExpr, params);
      }

      virtual bool remove_subscription(DDS::DomainId_t domainId,
                                       const RepoId& participantId,
                                       const RepoId& subscriptionId)
      {
        get_part(domainId, participantId)->remove_subscription(subscriptionId);
        return true;
      }

      virtual bool ignore_subscription(DDS::DomainId_t domainId,
                                       const RepoId& participantId,
                                       const RepoId& ignoreId)
      {
        get_part(domainId, participantId)->ignore_subscription(ignoreId);
        return true;
      }

      virtual bool update_subscription_qos(DDS::DomainId_t domainId,
                                           const RepoId& partId,
                                           const RepoId& drId,
                                           const DDS::DataReaderQos& qos,
                                           const DDS::SubscriberQos& subQos)
      {
        return get_part(domainId, partId)->update_subscription_qos(drId, qos, subQos);
      }

      virtual bool update_subscription_params(DDS::DomainId_t domainId,
                                              const RepoId& partId,
                                              const RepoId& subId,
                                              const DDS::StringSeq& params)
      {
        return get_part(domainId, partId)->update_subscription_params(subId, params);
      }

      virtual void update_subscription_locators(DDS::DomainId_t domainId,
                                                const RepoId& partId,
                                                const RepoId& subId,
                                                const TransportLocatorSeq& transInfo)
      {
        get_part(domainId, partId)->update_subscription_locators(subId, transInfo);
      }

      virtual void association_complete(DDS::DomainId_t domainId,
                                        const RepoId& participantId,
                                        const RepoId& localId,
                                        const RepoId& remoteId)
      {
        get_part(domainId, participantId)->association_complete(localId, remoteId);
      }

    protected:

      typedef RcHandle<Participant> ParticipantHandle;
      typedef OPENDDS_MAP_CMP(RepoId, ParticipantHandle, GUID_tKeyLessThan) ParticipantMap;
      typedef OPENDDS_MAP(DDS::DomainId_t, ParticipantMap) DomainParticipantMap;

      ParticipantHandle
        get_part(const DDS::DomainId_t domain_id,
                 const RepoId& part_id) const
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
                         SubscriberImpl* sub,
                         const DDS::DataReaderQos& qos)
      {
        TopicDescriptionImpl* bit_topic_i =
          dynamic_cast<TopicDescriptionImpl*>(topic);
        if (bit_topic_i == 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::create_bit_dr")
                     ACE_TEXT(" - Could not cast TopicDescription to TopicDescriptionImpl\n")));
          return;
        }

        DDS::DomainParticipant_var participant = sub->get_participant();
        DomainParticipantImpl* participant_i =
          dynamic_cast<DomainParticipantImpl*>(participant.in());
        if (participant_i == 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::create_bit_dr")
                     ACE_TEXT(" - Could not cast DomainParticipant to DomainParticipantImpl\n")));
          return;
        }

        TypeSupport_var type_support =
          Registered_Data_Types->lookup(participant, type);

        DDS::DataReader_var dr = type_support->create_datareader();
        DataReaderImpl* dri = dynamic_cast<DataReaderImpl*>(dr.in());
        if (dri == 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::create_bit_dr")
                     ACE_TEXT(" - Could not cast DataReader to DataReaderImpl\n")));
          return;
        }

        dri->init(bit_topic_i, qos, 0 /*listener*/, 0 /*mask*/, participant_i, sub);
        dri->disable_transport();
        dri->enable();
      }

      mutable ACE_Thread_Mutex lock_;

      DomainParticipantMap participants_;
    };

  } // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DISCOVERYBASE_H  */
