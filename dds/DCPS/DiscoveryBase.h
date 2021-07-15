/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DISCOVERYBASE_H
#define OPENDDS_DCPS_DISCOVERYBASE_H

#include "TopicDetails.h"
#include "BuiltInTopicUtils.h"
#include "DataReaderImpl_T.h"
#include "DCPS_Utils.h"
#include "Discovery.h"
#include "DomainParticipantImpl.h"
#include "GuidUtils.h"
#include "Marked_Default_Qos.h"
#include "PoolAllocationBase.h"
#include "Registered_Data_Types.h"
#include "SubscriberImpl.h"
#include "SporadicTask.h"
#include "TimeTypes.h"
#include "ConditionVariable.h"
#ifdef OPENDDS_SECURITY
#  include "Ice.h"
#  include "security/framework/HandleRegistry.h"
#endif
#include "XTypes/TypeAssignability.h"

#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DdsSecurityCoreC.h>
#endif

#include <ace/Thread_Mutex.h>

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
    typedef DataReaderImpl_T<ParticipantLocationBuiltinTopicData> ParticipantLocationBuiltinTopicDataDataReaderImpl;
    typedef DataReaderImpl_T<InternalThreadBuiltinTopicData> InternalThreadBuiltinTopicDataDataReaderImpl;
    typedef DataReaderImpl_T<ConnectionRecord> ConnectionRecordDataReaderImpl;

#ifdef OPENDDS_SECURITY
    typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::DatareaderCryptoTokenSeq, GUID_tKeyLessThan)
      DatareaderCryptoTokenSeqMap;
    typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::DatawriterCryptoTokenSeq, GUID_tKeyLessThan)
      DatawriterCryptoTokenSeqMap;

    enum AuthState {
      AUTH_STATE_HANDSHAKE,
      AUTH_STATE_AUTHENTICATED,
      AUTH_STATE_UNAUTHENTICATED
    };

    enum HandshakeState {
      // Requester should call begin_handshake_request
      HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST,

      // Replier should call begin_handshake_reply
      HANDSHAKE_STATE_BEGIN_HANDSHAKE_REPLY,

      // Requester and replier should call process handshake
      HANDSHAKE_STATE_PROCESS_HANDSHAKE,

      // Handshake concluded or timed out
      HANDSHAKE_STATE_DONE
    };
#endif

    inline void assign(EntityKey_t& lhs, unsigned int rhs)
    {
      lhs[0] = static_cast<CORBA::Octet>(rhs);
      lhs[1] = static_cast<CORBA::Octet>(rhs >> 8);
      lhs[2] = static_cast<CORBA::Octet>(rhs >> 16);
    }

    struct DcpsUpcalls : ACE_Task_Base {
      bool has_timeout() {
        return interval_ > TimeDuration(0);
      }

      DcpsUpcalls(DataReaderCallbacks_rch drr,
                  const RepoId& reader,
                  const WriterAssociation& wa,
                  bool active,
                  DataWriterCallbacks_rch dwr)
        : drr_(drr), reader_(reader), wa_(wa), active_(active), dwr_(dwr)
        , reader_done_(false), writer_done_(false), cnd_(mtx_)
        , interval_(TheServiceParticipant->get_thread_status_interval())
        , thread_status_manager_(TheServiceParticipant->get_thread_status_manager())
        , thread_key_(ThreadStatusManager::get_key("DcpsUpcalls"))
      {
      }

      int svc()
      {
        MonotonicTimePoint expire;
        const bool update_thread_status = thread_status_manager_ && has_timeout();
        if (update_thread_status) {
          expire = MonotonicTimePoint::now() + interval_;
        }

        DataReaderCallbacks_rch drr = drr_.lock();
        if (!drr) {
          return 0;
        }
        drr->add_association(reader_, wa_, active_);
        {
          ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mtx_, -1);
          reader_done_ = true;
          cnd_.notify_one();
          while (!writer_done_) {
            if (update_thread_status) {
              switch (cnd_.wait_until(expire)) {
              case CvStatus_NoTimeout:
                break;

              case CvStatus_Timeout:
                {
                  expire = MonotonicTimePoint::now() + interval_;
                  if (DCPS_debug_level > 4) {
                    ACE_DEBUG((LM_DEBUG,
                               "(%P|%t) DcpsUpcalls::svc. Updating thread status.\n"));
                  }
                  if (!thread_status_manager_->update(thread_key_)) {
                    if (DCPS_debug_level) {
                      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::svc: "
                        "update failed\n"));
                    }
                    return -1;
                  }
                }
                break;

              case CvStatus_Error:
                if (DCPS_debug_level) {
                  ACE_ERROR((LM_ERROR, "(%P|t) ERROR: DcpsUpcalls::svc: error in wait_utill\n"));
                }
                return -1;
              }
            } else if (cnd_.wait() == CvStatus_Error) {
              if (DCPS_debug_level) {
                ACE_ERROR((LM_ERROR, "(%P|t) ERROR: DcpsUpcalls::svc: error in wait\n"));
              }
              return -1;
            }
          }
        }

        if (update_thread_status) {
          if (DCPS_debug_level > 4) {
            ACE_DEBUG((LM_DEBUG, "(%P|%t) DcpsUpcalls: "
              "Updating thread status for the last time\n"));
          }
          if (!thread_status_manager_->update(thread_key_, ThreadStatus_Finished) &&
              DCPS_debug_level) {
            ACE_ERROR((LM_ERROR, "(%P|%t) DcpsUpcalls: final update failed\n"));
          }
        }

        return 0;
      }

      void writer_done()
      {
        {
          ACE_GUARD(ACE_Thread_Mutex, g, mtx_);
          writer_done_ = true;
          cnd_.notify_one();
        }

        const MonotonicTimePoint expire = has_timeout() ?
          MonotonicTimePoint::now() + interval_ : MonotonicTimePoint();

        wait(); // ACE_Task_Base::wait does not accept a timeout

        if (thread_status_manager_ && has_timeout() && MonotonicTimePoint::now() > expire) {
          if (DCPS_debug_level > 4) {
            ACE_DEBUG((LM_DEBUG,
                       "(%P|%t) DcpsUpcalls::writer_done. Updating thread status.\n"));
          }
          if (!thread_status_manager_->update(thread_key_) && DCPS_debug_level) {
            ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::writer_done: "
              "update failed\n"));
          }
        }
      }

      DataReaderCallbacks_wrch drr_;
      const RepoId& reader_;
      const WriterAssociation& wa_;
      bool active_;
      DataWriterCallbacks_wrch dwr_;
      bool reader_done_, writer_done_;
      ACE_Thread_Mutex mtx_;
      ConditionVariable<ACE_Thread_Mutex> cnd_;

      // thread reporting
      const TimeDuration interval_;
      ThreadStatusManager* const thread_status_manager_;
      const String thread_key_;
    };

    template <typename DiscoveredParticipantData_>
    class EndpointManager : public RcEventHandler {
    protected:

      struct DiscoveredSubscription : PoolAllocationBase {
        DiscoveredSubscription()
        : bit_ih_(DDS::HANDLE_NIL)
        , participant_discovered_at_(monotonic_time_zero())
        , transport_context_(0)
#ifdef OPENDDS_SECURITY
        , have_ice_agent_info_(false)
#endif
        {
        }

        explicit DiscoveredSubscription(const DiscoveredReaderData& r)
        : reader_data_(r)
        , bit_ih_(DDS::HANDLE_NIL)
        , participant_discovered_at_(monotonic_time_zero())
        , transport_context_(0)
#ifdef OPENDDS_SECURITY
        , have_ice_agent_info_(false)
#endif
        {
        }

        RepoIdSet matched_endpoints_;
        DiscoveredReaderData reader_data_;
        DDS::InstanceHandle_t bit_ih_;
        MonotonicTime_t participant_discovered_at_;
        ACE_CDR::ULong transport_context_;
        XTypes::TypeInformation type_info_;

#ifdef OPENDDS_SECURITY
        DDS::Security::EndpointSecurityAttributes security_attribs_;
        bool have_ice_agent_info_;
        ICE::AgentInfo ice_agent_info_;
#endif

        const char* get_topic_name() const
        {
          return reader_data_.ddsSubscriptionData.topic_name;
        }

        const char* get_type_name() const
        {
          return reader_data_.ddsSubscriptionData.type_name;
        }
      };

      typedef OPENDDS_MAP_CMP(RepoId, DiscoveredSubscription,
                              GUID_tKeyLessThan) DiscoveredSubscriptionMap;

      typedef typename DiscoveredSubscriptionMap::iterator DiscoveredSubscriptionIter;

      struct DiscoveredPublication : PoolAllocationBase {
        DiscoveredPublication()
        : bit_ih_(DDS::HANDLE_NIL)
        , participant_discovered_at_(monotonic_time_zero())
        , transport_context_(0)
#ifdef OPENDDS_SECURITY
        , have_ice_agent_info_(false)
#endif
        {
        }

        explicit DiscoveredPublication(const DiscoveredWriterData& w)
        : writer_data_(w)
        , bit_ih_(DDS::HANDLE_NIL)
        , participant_discovered_at_(monotonic_time_zero())
        , transport_context_(0)
#ifdef OPENDDS_SECURITY
        , have_ice_agent_info_(false)
#endif
        {
        }

        RepoIdSet matched_endpoints_;
        DiscoveredWriterData writer_data_;
        DDS::InstanceHandle_t bit_ih_;
        MonotonicTime_t participant_discovered_at_;
        ACE_CDR::ULong transport_context_;
        XTypes::TypeInformation type_info_;

#ifdef OPENDDS_SECURITY
        DDS::Security::EndpointSecurityAttributes security_attribs_;
        bool have_ice_agent_info_;
        ICE::AgentInfo ice_agent_info_;
#endif

        const char* get_topic_name() const
        {
          return writer_data_.ddsPublicationData.topic_name;
        }

        const char* get_type_name() const
        {
          return writer_data_.ddsPublicationData.type_name;
        }
      };

      typedef OPENDDS_MAP_CMP(RepoId, DiscoveredPublication,
                              GUID_tKeyLessThan) DiscoveredPublicationMap;
      typedef typename DiscoveredPublicationMap::iterator DiscoveredPublicationIter;

    public:
      typedef DiscoveredParticipantData_ DiscoveredParticipantData;
      typedef DCPS::TopicDetails TopicDetails;

      EndpointManager(const RepoId& participant_id, ACE_Thread_Mutex& lock)
        : max_type_lookup_service_reply_period_(0)
        , type_lookup_service_sequence_number_(0)
        , use_xtypes_(true)
        , use_xtypes_complete_(false)
        , lock_(lock)
        , participant_id_(participant_id)
        , publication_counter_(0)
        , subscription_counter_(0)
        , topic_counter_(0)
#ifdef OPENDDS_SECURITY
        , permissions_handle_(DDS::HANDLE_NIL)
        , crypto_handle_(DDS::HANDLE_NIL)
#endif
      { }

      virtual ~EndpointManager()
      {
        type_lookup_fini();
      }

      void type_lookup_init(ReactorInterceptor_rch reactor_interceptor)
      {
        if (!type_lookup_reply_deadline_processor_) {
          type_lookup_reply_deadline_processor_ =
            DCPS::make_rch<EndpointManagerSporadic>(reactor_interceptor, ref(*this), &EndpointManager::remove_expired_endpoints);
        }
      }

      void type_lookup_fini()
      {
        if (type_lookup_reply_deadline_processor_) {
          type_lookup_reply_deadline_processor_->cancel_and_wait();
          type_lookup_reply_deadline_processor_.reset();
        }
      }

      void type_lookup_service(const XTypes::TypeLookupService_rch type_lookup_service)
      {
        type_lookup_service_ = type_lookup_service;
      }

      void purge_dead_topic(const OPENDDS_STRING& topic_name)
      {
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
            const OPENDDS_STRING topic_name = iter->second.get_topic_name();
            TopicDetails& td = topics_[topic_name];
            td.remove_discovered_publication(to_ignore);
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
            const OPENDDS_STRING topic_name = iter->second.get_topic_name();
            TopicDetails& td = topics_[topic_name];
            td.remove_discovered_publication(to_ignore);
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
            {
              const RepoIdSet ids = td.discovered_publications();
              for (RepoIdSet::const_iterator ep = ids.begin(); ep!= ids.end(); ++ep) {
                match_endpoints(*ep, td, true /*remove*/);
                td.remove_discovered_publication(*ep);
                // TODO: Do we need to remove from discovered_subscriptions?
                if (shutting_down()) { return; }
              }
            }
            {
              const RepoIdSet ids = td.discovered_subscriptions();
              for (RepoIdSet::const_iterator ep = ids.begin(); ep!= ids.end(); ++ep) {
                match_endpoints(*ep, td, true /*remove*/);
                td.remove_discovered_subscription(*ep);
                // TODO: Do we need to remove from discovered_publications?
                if (shutting_down()) { return; }
              }
            }
            if (td.is_dead()) {
              purge_dead_topic(iter->second);
            }
          }
        }
      }

      bool ignoring(const RepoId& guid) const
      {
        return ignored_guids_.count(guid);
      }

      bool ignoring(const char* topic_name) const
      {
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
                             DataWriterCallbacks_rch publication,
                             const DDS::DataWriterQos& qos,
                             const TransportLocatorSeq& transInfo,
                             const DDS::PublisherQos& publisherQos,
                             const XTypes::TypeInformation& type_info)
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
        pb.type_info_ = type_info;
        const OPENDDS_STRING& topic_name = topic_names_[topicId];

#ifdef OPENDDS_SECURITY
        if (is_security_enabled()) {
          DDS::Security::SecurityException ex;

          DDS::Security::TopicSecurityAttributes topic_sec_attr;
          DDS::Security::PermissionsHandle permh = get_permissions_handle();
          if (!get_access_control()->get_topic_sec_attributes(permh, topic_name.data(), topic_sec_attr, ex)) {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: ")
              ACE_TEXT("EndpointManager::add_publication - ")
              ACE_TEXT("Unable to get security attributes for topic '%C'. SecurityException[%d.%d]: %C\n"),
                topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
            return RepoId();
          }

          if (topic_sec_attr.is_write_protected == true) {
            if (!get_access_control()->check_create_datawriter(
                  permh, get_domain_id(), topic_name.data(), qos,
                  publisherQos.partition, DDS::Security::DataTagQosPolicy(), ex)) {
              ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("EndpointManager::add_publication - ")
                ACE_TEXT("Permissions check failed for local datawriter on topic '%C'. ")
                ACE_TEXT("Security Exception[%d.%d]: %C\n"), topic_name.data(),
                  ex.code, ex.minor_code, ex.message.in()));
              return RepoId();
            }
          }

          if (!get_access_control()->get_datawriter_sec_attributes(permh, topic_name.data(),
              publisherQos.partition, DDS::Security::DataTagQosPolicy(), pb.security_attribs_, ex)) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("EndpointManager::add_publication - ")
                       ACE_TEXT("Unable to get security attributes for local datawriter. ")
                       ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                       ex.code, ex.minor_code, ex.message.in()));
            return RepoId();
          }

          if (pb.security_attribs_.is_submessage_protected || pb.security_attribs_.is_payload_protected) {
            const DDS::Security::DatawriterCryptoHandle handle =
              get_crypto_key_factory()->register_local_datawriter(
                crypto_handle_, DDS::PropertySeq(), pb.security_attribs_, ex);
            if (handle == DDS::HANDLE_NIL) {
              ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("EndpointManager::add_publication - ")
                         ACE_TEXT("Unable to get local datawriter crypto handle. ")
                         ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                         ex.code, ex.minor_code, ex.message.in()));
            }

            get_handle_registry()->insert_local_datawriter_crypto_handle(rid, handle, pb.security_attribs_);
          }
        }
#endif

        TopicDetails& td = topics_[topic_name];
        td.add_local_publication(rid);

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

#ifdef OPENDDS_SECURITY
      void cleanup_secure_writer(const RepoId& publicationId)
      {
        using namespace DDS::Security;

        Security::HandleRegistry_rch handle_registry = get_handle_registry();
        if (!handle_registry) {
          return;
        }
        const DatawriterCryptoHandle dwch =
          handle_registry->get_local_datawriter_crypto_handle(publicationId);
        if (dwch == DDS::HANDLE_NIL) {
          return;
        }

        SecurityException ex = {"", 0, 0};
        if (!get_crypto_key_factory()->unregister_datawriter(dwch, ex)) {
          if (security_debug.cleanup_error) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) {cleanup_error} Sedp::cleanup_secure_writer() - ")
                       ACE_TEXT("Failure calling unregister_datawriter. (ch %d)")
                       ACE_TEXT(" Security Exception[%d.%d]: %C\n"),
                       dwch, ex.code, ex.minor_code, ex.message.in()));
          }
        }
        handle_registry->erase_local_datawriter_crypto_handle(publicationId);
      }
#endif

      void remove_publication(const RepoId& publicationId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        LocalPublicationIter iter = local_publications_.find(publicationId);
        if (iter != local_publications_.end()) {
          if (DDS::RETCODE_OK == remove_publication_i(publicationId, iter->second)) {
            OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
#ifdef OPENDDS_SECURITY
            if (is_security_enabled()) {
              cleanup_secure_writer(publicationId);
            }
#endif
            local_publications_.erase(publicationId);
            typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
              topics_.find(topic_name);
            if (top_it != topics_.end()) {
              match_endpoints(publicationId, top_it->second, true /*remove*/);
              top_it->second.remove_local_publication(publicationId);
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
            ACE_DEBUG((LM_INFO,
              ACE_TEXT("(%P|%t) EndpointManager::update_publication_locators updating locators for %C\n"),
              OPENDDS_STRING(conv).c_str()));
          }
          iter->second.trans_info_ = transInfo;
          write_publication_data(publicationId, iter->second);
        }
      }

      RepoId add_subscription(const RepoId& topicId,
                              DataReaderCallbacks_rch subscription,
                              const DDS::DataReaderQos& qos,
                              const TransportLocatorSeq& transInfo,
                              const DDS::SubscriberQos& subscriberQos,
                              const char* filterClassName,
                              const char* filterExpr,
                              const DDS::StringSeq& params,
                              const XTypes::TypeInformation& type_info)
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
        sb.type_info_ = type_info;
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

            get_handle_registry()->insert_local_datareader_crypto_handle(rid, handle, sb.security_attribs_);
          }
        }
#endif

        TopicDetails& td = topics_[topic_name];
        td.add_local_subscription(rid);

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

#ifdef OPENDDS_SECURITY
      void cleanup_secure_reader(const RepoId& subscriptionId)
      {
        using namespace DDS::Security;

        Security::HandleRegistry_rch handle_registry = get_handle_registry();
        if (!handle_registry) {
          return;
        }
        const DatareaderCryptoHandle drch =
          handle_registry->get_local_datareader_crypto_handle(subscriptionId);
        if (drch == DDS::HANDLE_NIL) {
          return;
        }

        SecurityException ex = {"", 0, 0};
        if (!get_crypto_key_factory()->unregister_datareader(drch, ex)) {
          if (security_debug.cleanup_error) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) {cleanup_error} Sedp::cleanup_secure_reader() - ")
                       ACE_TEXT("Failure calling unregister_datareader (ch %d).")
                       ACE_TEXT(" Security Exception[%d.%d]: %C\n"),
                       drch, ex.code, ex.minor_code, ex.message.in()));
          }
        }
        handle_registry->erase_local_datareader_crypto_handle(subscriptionId);
      }
#endif

      void remove_subscription(const RepoId& subscriptionId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
        if (iter != local_subscriptions_.end()) {
          if (DDS::RETCODE_OK == remove_subscription_i(subscriptionId, iter->second)) {
            OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
#ifdef OPENDDS_SECURITY
            if (is_security_enabled()) {
              cleanup_secure_reader(subscriptionId);
            }
#endif
            local_subscriptions_.erase(subscriptionId);
            typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
              topics_.find(topic_name);
            if (top_it != topics_.end()) {
              match_endpoints(subscriptionId, top_it->second, true /*remove*/);
              top_it->second.remove_local_subscription(subscriptionId);
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
            ACE_DEBUG((LM_INFO,
              ACE_TEXT("(%P|%t) EndpointManager::update_subscription_locators updating locators for %C\n"),
              OPENDDS_STRING(conv).c_str()));
          }
          iter->second.trans_info_ = transInfo;
          write_subscription_data(subscriptionId, iter->second);
        }
      }

      virtual bool disassociate(DiscoveredParticipantData& pdata) = 0;

#ifdef OPENDDS_SECURITY
      inline Security::HandleRegistry_rch get_handle_registry() const
      {
        return handle_registry_;
      }
#endif

    protected:
      struct LocalEndpoint {
        LocalEndpoint()
          : topic_id_(GUID_UNKNOWN)
          , participant_discovered_at_(monotonic_time_zero())
          , transport_context_(0)
          , sequence_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
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
        MonotonicTime_t participant_discovered_at_;
        ACE_CDR::ULong transport_context_;
        RepoIdSet matched_endpoints_;
        SequenceNumber sequence_;
        RepoIdSet remote_expectant_opendds_associations_;
        XTypes::TypeInformation type_info_;
#ifdef OPENDDS_SECURITY
        bool have_ice_agent_info;
        ICE::AgentInfo ice_agent_info;
        DDS::Security::EndpointSecurityAttributes security_attribs_;
#endif
      };

      struct LocalPublication : LocalEndpoint {
        DataWriterCallbacks_wrch publication_;
        DDS::DataWriterQos qos_;
        DDS::PublisherQos publisher_qos_;
      };

      struct LocalSubscription : LocalEndpoint {
        DataReaderCallbacks_wrch subscription_;
        DDS::DataReaderQos qos_;
        DDS::SubscriberQos subscriber_qos_;
        ContentFilterProperty_t filterProperties;
      };

      typedef OPENDDS_MAP_CMP(RepoId, LocalPublication,
                              GUID_tKeyLessThan) LocalPublicationMap;
      typedef typename LocalPublicationMap::iterator LocalPublicationIter;
      typedef typename LocalPublicationMap::const_iterator LocalPublicationCIter;

      typedef OPENDDS_MAP_CMP(RepoId, LocalSubscription,
                              GUID_tKeyLessThan) LocalSubscriptionMap;
      typedef typename LocalSubscriptionMap::iterator LocalSubscriptionIter;
      typedef typename LocalSubscriptionMap::const_iterator LocalSubscriptionCIter;

      typedef typename OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, GUID_tKeyLessThan) TopicNameMap;

      static DDS::BuiltinTopicKey_t get_key(const DiscoveredPublication& pub)
      {
        return pub.writer_data_.ddsPublicationData.key;
      }

      static DDS::BuiltinTopicKey_t get_key(const DiscoveredSubscription& sub)
      {
        return sub.reader_data_.ddsSubscriptionData.key;
      }

      virtual void remove_from_bit_i(const DiscoveredPublication& /*pub*/) { }
      virtual void remove_from_bit_i(const DiscoveredSubscription& /*sub*/) { }

      virtual void assign_publication_key(RepoId& rid,
                                          const RepoId& topicId,
                                          const DDS::DataWriterQos& /*qos*/)
      {
        rid.entityId.entityKind =
          has_dcps_key(topicId)
          ? ENTITYKIND_USER_WRITER_WITH_KEY
          : ENTITYKIND_USER_WRITER_NO_KEY;
        assign(rid.entityId.entityKey, publication_counter_++);
      }

      virtual void assign_subscription_key(RepoId& rid,
                                           const RepoId& topicId,
                                           const DDS::DataReaderQos& /*qos*/)
      {
        rid.entityId.entityKind =
          has_dcps_key(topicId)
          ? ENTITYKIND_USER_READER_WITH_KEY
          : ENTITYKIND_USER_READER_NO_KEY;
        assign(rid.entityId.entityKey, subscription_counter_++);
      }

      virtual void assign_topic_key(RepoId& guid)
      {
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

      virtual bool send_type_lookup_request(const XTypes::TypeIdentifierSeq& /*type_ids*/,
                                            const DCPS::RepoId& /*endpoint*/,
                                            bool /*is_discovery_protected*/,
                                            bool /*send_get_types*/)
      { return true; }

      // TODO: This is perhaps too generic since the context probably has the details this function computes.
      void match_endpoints(RepoId repoId, const TopicDetails& td,
                           bool remove = false)
      {
        if (DCPS_debug_level >= 4) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::match_endpoints %C%C\n",
            remove ? "remove " : "", LogGuid(repoId).c_str()));
        }

        const bool reader = GuidConverter(repoId).isReader();
        // Copy the endpoint set - lock can be released in match()
        RepoIdSet local_endpoints;
        RepoIdSet discovered_endpoints;
        if (reader) {
          local_endpoints = td.local_publications();
          discovered_endpoints = td.discovered_publications();
        } else {
          local_endpoints = td.local_subscriptions();
          discovered_endpoints = td.discovered_subscriptions();
        }

        const bool is_remote = !equal_guid_prefixes(repoId, participant_id_);
        if (is_remote && local_endpoints.empty()) {
          // Nothing to match.
          return;
        }

        for (RepoIdSet::const_iterator iter = local_endpoints.begin();
             iter != local_endpoints.end(); ++iter) {
          // check to make sure it's a Reader/Writer or Writer/Reader match
          if (GuidConverter(*iter).isReader() != reader) {
            if (remove) {
              remove_assoc(*iter, repoId);
            } else {
              match(reader ? *iter : repoId, reader ? repoId : *iter);
            }
          }
        }

        // Remote/remote matches are a waste of time
        if (is_remote) {
          return;
        }

        for (RepoIdSet::const_iterator iter = discovered_endpoints.begin();
             iter != discovered_endpoints.end(); ++iter) {
          // check to make sure it's a Reader/Writer or Writer/Reader match
          if (GuidConverter(*iter).isReader() != reader) {
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
        if (GuidConverter(remove_from).isReader()) {
          const LocalSubscriptionIter lsi = local_subscriptions_.find(remove_from);
          if (lsi != local_subscriptions_.end()) {
            lsi->second.matched_endpoints_.erase(removing);
            const DiscoveredPublicationIter dpi = discovered_publications_.find(removing);
            if (dpi != discovered_publications_.end()) {
              dpi->second.matched_endpoints_.erase(remove_from);
            }
            WriterIdSeq writer_seq(1);
            writer_seq.length(1);
            writer_seq[0] = removing;
            const size_t count = lsi->second.remote_expectant_opendds_associations_.erase(removing);
            DataReaderCallbacks_rch drr = lsi->second.subscription_.lock();
            if (drr) {
              drr->remove_associations(writer_seq,
                                       false /*notify_lost*/);
            }
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
            const DiscoveredSubscriptionIter dsi = discovered_subscriptions_.find(removing);
            if (dsi != discovered_subscriptions_.end()) {
              dsi->second.matched_endpoints_.erase(remove_from);
            }
            ReaderIdSeq reader_seq(1);
            reader_seq.length(1);
            reader_seq[0] = removing;
            lpi->second.remote_expectant_opendds_associations_.erase(removing);
            DataWriterCallbacks_rch dwr = lpi->second.publication_.lock();
            if (dwr) {
              dwr->remove_associations(reader_seq,
                                       false /*notify_lost*/);
            }
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
      generate_remote_matched_writer_crypto_handle(const RepoId& /*writer*/,
                                                   const RepoId& /*reader*/)
      {
        return DDS::HANDLE_NIL;
      }

      virtual DDS::Security::DatareaderCryptoHandle
      generate_remote_matched_reader_crypto_handle(const RepoId& /*reader*/,
                                                   const RepoId& /*writer*/,
                                                   bool)
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


      struct MatchingData {
        // Sequence number of the first request for remote minimal types.
        SequenceNumber rpc_seqnum_minimal;

        // Whether all minimal types are obtained.
        bool got_minimal;

        // Sequence number of the first request for remote complete types.
        // Set to SEQUENCENUMBER_UNKNOWN if there is no such request.
        SequenceNumber rpc_seqnum_complete;

        // Whether all complete types are obtained.
        bool got_complete;

        MonotonicTimePoint time_added_to_map;
      };

      struct MatchingPair {
        MatchingPair(RepoId writer, RepoId reader)
          : writer_(writer), reader_(reader) {}

        RepoId writer_;
        RepoId reader_;

        bool operator<(const MatchingPair& a_other) const
        {
          if (GUID_tKeyLessThan()(writer_, a_other.writer_)) return true;

          if (GUID_tKeyLessThan()(a_other.writer_, writer_)) return false;

          if (GUID_tKeyLessThan()(reader_, a_other.reader_)) return true;

          if (GUID_tKeyLessThan()(a_other.reader_, reader_)) return false;

          return false;
        }
      };

      typedef OPENDDS_MAP_T(MatchingPair, MatchingData) MatchingDataMap;
      typedef typename MatchingDataMap::iterator MatchingDataIter;
      MatchingDataMap matching_data_buffer_;
      typedef PmfSporadicTask<EndpointManager> EndpointManagerSporadic;
      RcHandle<EndpointManagerSporadic> type_lookup_reply_deadline_processor_;
      TimeDuration max_type_lookup_service_reply_period_;
      DCPS::SequenceNumber type_lookup_service_sequence_number_;
      bool use_xtypes_;
      bool use_xtypes_complete_;

      void
      match(const RepoId& writer, const RepoId& reader)
      {
        if (DCPS_debug_level >= 4) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::match: w: %C r: %C\n",
            LogGuid(writer).c_str(), LogGuid(reader).c_str()));
        }

        // 1. Collect type info about the writer, which may be local or discovered
        XTypes::TypeInformation* writer_type_info = 0;

        const LocalPublicationIter lpi = local_publications_.find(writer);
        DiscoveredPublicationIter dpi;
        bool writer_local = false;
        if (lpi != local_publications_.end()) {
          writer_local = true;
          writer_type_info = &lpi->second.type_info_;
        } else if ((dpi = discovered_publications_.find(writer))
                   != discovered_publications_.end()) {
          writer_type_info = &dpi->second.type_info_;
        } else {
          if (DCPS_debug_level >= 4) {
            ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::match: Undiscovered Writer\n"));
          }
          return; // Possible and ok, since lock is released
        }

        // 2. Collect type info about the reader, which may be local or discovered
        XTypes::TypeInformation* reader_type_info = 0;

        const LocalSubscriptionIter lsi = local_subscriptions_.find(reader);
        DiscoveredSubscriptionIter dsi;
        bool reader_local = false;
        if (lsi != local_subscriptions_.end()) {
          reader_local = true;
          reader_type_info = &lsi->second.type_info_;
        } else if ((dsi = discovered_subscriptions_.find(reader))
                   != discovered_subscriptions_.end()) {
          reader_type_info = &dsi->second.type_info_;
        } else {
          if (DCPS_debug_level >= 4) {
            ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::match: Undiscovered Reader\n"));
          }
          return; // Possible and ok, since lock is released
        }

        MatchingData md;

        // If the type object is not in cache, send RPC request
        md.time_added_to_map = MonotonicTimePoint::now();

        // NOTE(sonndinh): Is it possible for a discovered endpoint to include only the "complete"
        // part in its TypeInformation? If it's possible, then we may need to handle that case, i.e.,
        // request only the remote complete TypeObject (if it's not already in the cache).
        // The following code assumes when the "minimal" part is not included in the discovered
        // endpoint's TypeInformation, then the "complete" part also is not included.
        if ((writer_type_info->minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE) &&
            (reader_type_info->minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE)) {
          if (!writer_local && reader_local) {
            const bool need_minimal_tobjs = type_lookup_service_ &&
              !type_lookup_service_->type_object_in_cache(writer_type_info->minimal.typeid_with_size.type_id);
            const bool need_complete_tobjs = type_lookup_service_ && use_xtypes_complete_ &&
              writer_type_info->complete.typeid_with_size.type_id.kind() != XTypes::TK_NONE &&
              !type_lookup_service_->type_object_in_cache(writer_type_info->complete.typeid_with_size.type_id);

            if (need_minimal_tobjs || need_complete_tobjs) {
              if (DCPS_debug_level >= 4) {
                ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::match: Remote Writer\n"));
              }
              bool is_discovery_protected = false;
#ifdef OPENDDS_SECURITY
              is_discovery_protected = lsi->second.security_attribs_.base.is_discovery_protected;
#endif
              save_matching_data_and_get_typeobjects(writer_type_info, md,
                                                     MatchingPair(writer, reader),
                                                     writer, is_discovery_protected,
                                                     need_minimal_tobjs, need_complete_tobjs);
              return;
            }
          } else if (!reader_local && writer_local) {
            const bool need_minimal_tobjs = type_lookup_service_ &&
              !type_lookup_service_->type_object_in_cache(reader_type_info->minimal.typeid_with_size.type_id);
            const bool need_complete_tobjs = type_lookup_service_ && use_xtypes_complete_ &&
              reader_type_info->complete.typeid_with_size.type_id.kind() != XTypes::TK_NONE &&
              !type_lookup_service_->type_object_in_cache(reader_type_info->complete.typeid_with_size.type_id);

            if (need_minimal_tobjs || need_complete_tobjs) {
              if (DCPS_debug_level >= 4) {
                ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::match: Remote Reader\n"));
              }
              bool is_discovery_protected = false;
#ifdef OPENDDS_SECURITY
              is_discovery_protected = lpi->second.security_attribs_.base.is_discovery_protected;
#endif
              save_matching_data_and_get_typeobjects(reader_type_info, md,
                                                     MatchingPair(writer, reader),
                                                     reader, is_discovery_protected,
                                                     need_minimal_tobjs, need_complete_tobjs);
              return;
            }
          }
        }

        match_continue(writer, reader);
      }

      void
      remove_expired_endpoints(const MonotonicTimePoint& /*now*/)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        const MonotonicTimePoint now = MonotonicTimePoint::now();

        MatchingDataIter end_iter = matching_data_buffer_.end();
        for (MatchingDataIter iter = matching_data_buffer_.begin(); iter != end_iter; ) {
          // Do not try to simplify increment: "associative container erase idiom"
          if (now - iter->second.time_added_to_map >= max_type_lookup_service_reply_period_) {
            if (DCPS_debug_level >= 4) {
              ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::remove_expired_endpoints: "
                "clean up pending pair w: %C r: %C\n",
                LogGuid(iter->first.writer_).c_str(), LogGuid(iter->first.reader_).c_str()));
            }
            matching_data_buffer_.erase(iter++);
          } else {
            ++iter;
          }
        }

        // Clean up internal data used by getTypeDependencies
        for (typename OrigSeqNumberMap::iterator it = orig_seq_numbers_.begin(); it != orig_seq_numbers_.end();) {
          if (now - it->second.time_started >= max_type_lookup_service_reply_period_) {
            if (DCPS_debug_level >= 4) {
              ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::remove_expired_endpoints: "
                "clean up type lookup data for %C\n",
                LogGuid(it->second.participant).c_str()));
            }
            cleanup_type_lookup_data(it->second.participant, it->second.type_id, it->second.secure);
            orig_seq_numbers_.erase(it++);
          } else {
            ++it;
          }
        }
      }

      void
      match_continue(const RepoId& writer, const RepoId& reader)
      {
        if (DCPS_debug_level >= 4) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) EndpointManager::match_continue: w: %C r: %C\n",
            LogGuid(writer).c_str(), LogGuid(reader).c_str()));
        }

        // 0. For discovered endpoints, we'll have the QoS info in the form of the
        // publication or subscription BIT data which doesn't use the same structures
        // for QoS.  In those cases we can copy the individual QoS policies to temp
        // QoS structs:
        DDS::DataWriterQos tempDwQos;
        DDS::PublisherQos tempPubQos;
        DDS::DataReaderQos tempDrQos;
        DDS::SubscriberQos tempSubQos;
        ContentFilterProperty_t tempCfp;

        DiscoveredPublicationIter dpi = discovered_publications_.find(writer);
        DiscoveredSubscriptionIter dsi = discovered_subscriptions_.find(reader);
        if (dpi != discovered_publications_.end() && dsi != discovered_subscriptions_.end()) {
          // This is a discovered/discovered match, nothing for us to do
          return;
        }

        // 1. Collect details about the writer, which may be local or discovered
        const DDS::DataWriterQos* dwQos = 0;
        const DDS::PublisherQos* pubQos = 0;
        TransportLocatorSeq* wTls = 0;
        ACE_CDR::ULong wTransportContext = 0;
        XTypes::TypeInformation* writer_type_info = 0;
        OPENDDS_STRING topic_name;
        MonotonicTime_t writer_participant_discovered_at;

        const LocalPublicationIter lpi = local_publications_.find(writer);
        bool writer_local = false, already_matched = false;
        if (lpi != local_publications_.end()) {
          writer_local = true;
          dwQos = &lpi->second.qos_;
          pubQos = &lpi->second.publisher_qos_;
          wTls = &lpi->second.trans_info_;
          wTransportContext = lpi->second.transport_context_;
          already_matched = lpi->second.matched_endpoints_.count(reader);
          writer_type_info = &lpi->second.type_info_;
          topic_name = topic_names_[lpi->second.topic_id_];
          writer_participant_discovered_at = lpi->second.participant_discovered_at_;
        } else if (dpi != discovered_publications_.end()) {
          wTls = &dpi->second.writer_data_.writerProxy.allLocators;
          wTransportContext = dpi->second.transport_context_;
          writer_type_info = &dpi->second.type_info_;
          topic_name = dpi->second.get_topic_name();
          writer_participant_discovered_at = dpi->second.participant_discovered_at_;

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
          tempDwQos.representation = bit.representation;
          dwQos = &tempDwQos;

          tempPubQos.presentation = bit.presentation;
          tempPubQos.partition = bit.partition;
          tempPubQos.group_data = bit.group_data;
          tempPubQos.entity_factory =
            TheServiceParticipant->initial_EntityFactoryQosPolicy();
          pubQos = &tempPubQos;

          populate_transport_locator_sequence(wTls, dpi, writer);
        } else {
          return; // Possible and ok, since lock is released
        }

        // 2. Collect details about the reader, which may be local or discovered
        const DDS::DataReaderQos* drQos = 0;
        const DDS::SubscriberQos* subQos = 0;
        TransportLocatorSeq* rTls = 0;
        ACE_CDR::ULong rTransportContext = 0;
        const ContentFilterProperty_t* cfProp = 0;
        XTypes::TypeInformation* reader_type_info = 0;
        MonotonicTime_t reader_participant_discovered_at;

        const LocalSubscriptionIter lsi = local_subscriptions_.find(reader);
        bool reader_local = false;
        if (lsi != local_subscriptions_.end()) {
          reader_local = true;
          drQos = &lsi->second.qos_;
          subQos = &lsi->second.subscriber_qos_;
          rTls = &lsi->second.trans_info_;
          rTransportContext = lsi->second.transport_context_;
          reader_type_info = &lsi->second.type_info_;
          if (lsi->second.filterProperties.filterExpression[0] != 0) {
            tempCfp.filterExpression = lsi->second.filterProperties.filterExpression;
            tempCfp.expressionParameters = lsi->second.filterProperties.expressionParameters;
          }
          cfProp = &tempCfp;
          if (!already_matched) {
            already_matched = lsi->second.matched_endpoints_.count(writer);
          }
          reader_participant_discovered_at = lsi->second.participant_discovered_at_;
        } else if (dsi != discovered_subscriptions_.end()) {
          rTls = &dsi->second.reader_data_.readerProxy.allLocators;

          populate_transport_locator_sequence(rTls, dsi, reader);
          rTransportContext = dsi->second.transport_context_;

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
          tempDrQos.representation = bit.representation;
          tempDrQos.type_consistency = bit.type_consistency;
          drQos = &tempDrQos;

          tempSubQos.presentation = bit.presentation;
          tempSubQos.partition = bit.partition;
          tempSubQos.group_data = bit.group_data;
          tempSubQos.entity_factory =
            TheServiceParticipant->initial_EntityFactoryQosPolicy();
          subQos = &tempSubQos;

          cfProp = &dsi->second.reader_data_.contentFilterProperty;
          reader_type_info = &dsi->second.type_info_;
          reader_participant_discovered_at = dsi->second.participant_discovered_at_;
        } else {
          return; // Possible and ok, since lock is released
        }

        // 3. Perform type consistency check (XTypes 1.3, Section 7.6.3.4.2)
        bool consistent = false;

        typename OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator td_iter = topics_.find(topic_name);
        if (td_iter == topics_.end()) {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) EndpointManager::match_continue - ERROR ")
                    ACE_TEXT("Didn't find topic for consistency check\n")));
          return;
        } else {
          const XTypes::TypeIdentifier& writer_type_id = writer_type_info->minimal.typeid_with_size.type_id;
          const XTypes::TypeIdentifier& reader_type_id = reader_type_info->minimal.typeid_with_size.type_id;
          if (writer_type_id.kind() != XTypes::TK_NONE && reader_type_id.kind() != XTypes::TK_NONE) {
            if (!writer_local || !reader_local) {
              const DDS::DataRepresentationIdSeq repIds =
                get_effective_data_rep_qos(tempDwQos.representation.value, false);
              Encoding::Kind encoding_kind;
              if (repr_to_encoding_kind(repIds[0], encoding_kind) && encoding_kind == Encoding::KIND_XCDR1) {
                const XTypes::TypeFlag extensibility_mask = XTypes::IS_APPENDABLE;
                if (type_lookup_service_->extensibility(extensibility_mask, writer_type_id)) {
                  if (OpenDDS::DCPS::DCPS_debug_level) {
                    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
                      ACE_TEXT("EndpointManager::match_continue: ")
                      ACE_TEXT("Encountered unsupported combination of XCDR1 encoding and appendable extensibility\n")));
                  }
                }
              }
            }

            XTypes::TypeConsistencyAttributes type_consistency;
            type_consistency.ignore_sequence_bounds = drQos->type_consistency.ignore_sequence_bounds;
            type_consistency.ignore_string_bounds = drQos->type_consistency.ignore_string_bounds;
            type_consistency.ignore_member_names = drQos->type_consistency.ignore_member_names;
            type_consistency.prevent_type_widening = drQos->type_consistency.prevent_type_widening;
            XTypes::TypeAssignability ta(type_lookup_service_, type_consistency);

            if (drQos->type_consistency.kind == DDS::ALLOW_TYPE_COERCION) {
              consistent = ta.assignable(reader_type_id, writer_type_id);
            } else {
              // The two types must be equivalent for DISALLOW_TYPE_COERCION
              consistent = reader_type_id == writer_type_id;
            }
          } else {
            if (drQos->type_consistency.force_type_validation) {
              // Cannot do type validation since not both TypeObjects are available
              consistent = false;
            } else {
              // Fall back to matching type names
              OPENDDS_STRING writer_type_name;
              OPENDDS_STRING reader_type_name;
              if (writer_local) {
                writer_type_name = td_iter->second.local_data_type_name();
              } else {
                writer_type_name = dpi->second.get_type_name();
              }
              if (reader_local) {
                reader_type_name = td_iter->second.local_data_type_name();
              } else {
                reader_type_name = dsi->second.get_type_name();
              }
              consistent = writer_type_name == reader_type_name;
            }
          }

          if (!consistent) {
            td_iter->second.increment_inconsistent();
            if (DCPS::DCPS_debug_level) {
              ACE_DEBUG((LM_WARNING,
                         ACE_TEXT("(%P|%t) EndpointManager::match_continue - WARNING ")
                         ACE_TEXT("Data types of topic %C does not match (inconsistent)\n"),
                         topic_name.c_str()));
            }
            return;
          }
        }

        // Need to release lock, below, for callbacks into DCPS which could
        // call into Spdp/Sedp.  Note that this doesn't unlock, it just constructs
        // an ACE object which will be used below for unlocking.
        ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

        // 4. Check transport and QoS compatibility

        // Copy entries from local publication and local subscription maps
        // prior to releasing lock
        DataWriterCallbacks_wrch dwr;
        DataReaderCallbacks_wrch drr;
        if (writer_local) {
          dwr = lpi->second.publication_;
          OPENDDS_ASSERT(lpi->second.publication_);
          OPENDDS_ASSERT(dwr);
        }
        if (reader_local) {
          drr = lsi->second.subscription_;
          OPENDDS_ASSERT(lsi->second.subscription_);
          OPENDDS_ASSERT(drr);
        }

        IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
        IncompatibleQosStatus readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};

        if (compatibleQOS(&writerStatus, &readerStatus, *wTls, *rTls,
                          dwQos, drQos, pubQos, subQos)) {

          bool call_writer = false, call_reader = false;

          if (writer_local) {
            call_writer = lpi->second.matched_endpoints_.insert(reader).second;
            dwr = lpi->second.publication_;
            if (!reader_local) {
              dsi->second.matched_endpoints_.insert(writer);
            }
          }
          if (reader_local) {
            call_reader = lsi->second.matched_endpoints_.insert(writer).second;
            drr = lsi->second.subscription_;
            if (!writer_local) {
              dpi->second.matched_endpoints_.insert(reader);
            }
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
            match_continue_security_enabled(writer, reader, call_writer, call_reader);
          }
#endif

          // Copy reader and writer association data prior to releasing lock
          DDS::OctetSeq octet_seq_type_info_reader;
          XTypes::serialize_type_info(*reader_type_info, octet_seq_type_info_reader);
          const ReaderAssociation ra = {
            *rTls, rTransportContext, reader, *subQos, *drQos,
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
            cfProp->filterClassName, cfProp->filterExpression,
#else
            "", "",
#endif
            cfProp->expressionParameters,
            octet_seq_type_info_reader,
            reader_participant_discovered_at
          };

          DDS::OctetSeq octet_seq_type_info_writer;
          XTypes::serialize_type_info(*writer_type_info, octet_seq_type_info_writer);
          const WriterAssociation wa = {
            *wTls, wTransportContext, writer, *pubQos, *dwQos,
            octet_seq_type_info_writer,
            writer_participant_discovered_at
          };

          ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
          static const bool writer_active = true;

          if (call_writer) {
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match_continue - ")
                         ACE_TEXT("adding writer %C association for reader %C\n"),
                         OPENDDS_STRING(GuidConverter(writer)).c_str(),
                         OPENDDS_STRING(GuidConverter(reader)).c_str()));
            }
            DataWriterCallbacks_rch dwr_lock = dwr.lock();
            if (dwr_lock) {
              if (call_reader) {
                DataReaderCallbacks_rch drr_lock = drr.lock();
                if (drr_lock) {
                  DcpsUpcalls thr(drr_lock, reader, wa, !writer_active, dwr_lock);
                  thr.activate();
                  dwr_lock->add_association(writer, ra, writer_active);
                  thr.writer_done();
                }
              } else {
                dwr_lock->add_association(writer, ra, writer_active);
              }
            }
          } else if (call_reader) {
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match_continue - ")
                         ACE_TEXT("adding reader %C association for writer %C\n"),
                         OPENDDS_STRING(GuidConverter(reader)).c_str(),
                         OPENDDS_STRING(GuidConverter(writer)).c_str()));
            }
            DataReaderCallbacks_rch drr_lock = drr.lock();
            if (drr_lock) {
              drr_lock->add_association(reader, wa, !writer_active);
            }
          }

        } else if (already_matched) { // break an existing association
          if (writer_local) {
            lpi->second.matched_endpoints_.erase(reader);
            lpi->second.remote_expectant_opendds_associations_.erase(reader);
            if (dsi != discovered_subscriptions_.end()) {
              dsi->second.matched_endpoints_.erase(writer);
            }
          }
          if (reader_local) {
            lsi->second.matched_endpoints_.erase(writer);
            lsi->second.remote_expectant_opendds_associations_.erase(writer);
            if (dpi != discovered_publications_.end()) {
              dpi->second.matched_endpoints_.erase(reader);
            }
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
            DataWriterCallbacks_rch dwr_lock = dwr.lock();
            if (dwr_lock) {
              dwr_lock->remove_associations(reader_seq, false /*notify_lost*/);
            }
          }
          if (reader_local) {
            WriterIdSeq writer_seq(1);
            writer_seq.length(1);
            writer_seq[0] = writer;
            DataReaderCallbacks_rch drr_lock = drr.lock();
            if (drr_lock) {
              drr_lock->remove_associations(writer_seq, false /*notify_lost*/);
            }
          }
        } else { // something was incompatible
          ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
          if (writer_local && writerStatus.count_since_last_send) {
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match_continue - ")
                         ACE_TEXT("writer incompatible\n")));
            }
            DataWriterCallbacks_rch dwr_lock = dwr.lock();
            if (dwr_lock) {
              dwr_lock->update_incompatible_qos(writerStatus);
            }
          }
          if (reader_local && readerStatus.count_since_last_send) {
            if (DCPS_debug_level > 3) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) EndpointManager::match_continue - ")
                         ACE_TEXT("reader incompatible\n")));
            }
            DataReaderCallbacks_rch drr_lock = drr.lock();
            if (drr_lock) {
              drr_lock->update_incompatible_qos(readerStatus);
            }
          }
        }
      }

      void save_matching_data_and_get_typeobjects(const XTypes::TypeInformation* type_info,
                                                  MatchingData& md, const MatchingPair& mp,
                                                  const RepoId& remote_id,
                                                  bool is_discovery_protected,
                                                  bool get_minimal, bool get_complete)
      {
        if (get_minimal) {
          md.rpc_seqnum_minimal = ++type_lookup_service_sequence_number_;
          md.got_minimal = false;
        } else {
          md.rpc_seqnum_minimal = SequenceNumber::SEQUENCENUMBER_UNKNOWN();
          md.got_minimal = true;
        }

        if (get_complete) {
          md.rpc_seqnum_complete = ++type_lookup_service_sequence_number_;
          md.got_complete = false;
        } else {
          md.rpc_seqnum_complete = SequenceNumber::SEQUENCENUMBER_UNKNOWN();
          md.got_complete = true;
        }

        matching_data_buffer_[mp] = md;

        // Send a sequence of requests for minimal remote TypeObjects
        if (get_minimal) {
          if (DCPS_debug_level >= 4) {
            ACE_DEBUG((LM_DEBUG,
                       "(%P|%t) EndpointManager::save_matching_data_and_get_typeobjects: "
                       "remote: %C seq: %q\n",
                       LogGuid(remote_id).c_str(), md.rpc_seqnum_minimal.getValue()));
          }
          get_remote_type_objects(type_info->minimal, md, true, remote_id, is_discovery_protected);
        }

        // Send another sequence of requests for complete remote TypeObjects
        if (get_complete) {
          if (DCPS_debug_level >= 4) {
            ACE_DEBUG((LM_DEBUG,
                       "(%P|%t) EndpointManager::save_matching_data_and_get_typeobjects: "
                       "remote: %C seq: %q\n",
                       LogGuid(remote_id).c_str(), md.rpc_seqnum_complete.getValue()));
          }
          get_remote_type_objects(type_info->complete, md, false, remote_id, is_discovery_protected);
        }
      }

      void get_remote_type_objects(const XTypes::TypeIdentifierWithDependencies& tid_with_deps,
                                   MatchingData& md, bool get_minimal, const RepoId& remote_id,
                                   bool is_discovery_protected)
      {
        // Store an entry for the first request in the sequence.
        TypeIdOrigSeqNumber orig_req_data;
        std::memcpy(orig_req_data.participant, remote_id.guidPrefix, sizeof(GuidPrefix_t));
        orig_req_data.type_id = tid_with_deps.typeid_with_size.type_id;
        const SequenceNumber& orig_seqnum = get_minimal ? md.rpc_seqnum_minimal : md.rpc_seqnum_complete;
        orig_req_data.seq_number = orig_seqnum;
        orig_req_data.secure = false;
#ifdef OPENDDS_SECURITY
        if (is_security_enabled() && is_discovery_protected) {
          orig_req_data.secure = true;
        }
#endif
        orig_req_data.time_started = md.time_added_to_map;
        orig_seq_numbers_.insert(std::make_pair(orig_seqnum, orig_req_data));

        XTypes::TypeIdentifierSeq type_ids;
        if (tid_with_deps.dependent_typeid_count == -1 ||
            tid_with_deps.dependent_typeids.length() < (CORBA::ULong)tid_with_deps.dependent_typeid_count) {
          type_ids.append(tid_with_deps.typeid_with_size.type_id);

          // Get dependencies of the topic type. TypeObjects of both topic type and
          // its dependencies are obtained in subsequent type lookup requests.
          send_type_lookup_request(type_ids, remote_id, is_discovery_protected, false);
        } else {
          type_ids.length(tid_with_deps.dependent_typeid_count + 1);
          type_ids[0] = tid_with_deps.typeid_with_size.type_id;
          for (unsigned i = 1; i <= (unsigned)tid_with_deps.dependent_typeid_count; ++i) {
            type_ids[i] = tid_with_deps.dependent_typeids[i - 1].type_id;
          }

          // Get TypeObjects of topic type and all of its dependencies.
          send_type_lookup_request(type_ids, remote_id, is_discovery_protected, true);
        }
        type_lookup_reply_deadline_processor_->schedule(max_type_lookup_service_reply_period_);
      }

      // Cleanup internal data used by type lookup operations
      virtual void cleanup_type_lookup_data(const GuidPrefix_t& guid_prefix,
                                            const XTypes::TypeIdentifier& ti,
                                            bool secure) = 0;

#ifdef OPENDDS_SECURITY
      void match_continue_security_enabled(
        const RepoId& writer, const RepoId& reader, bool call_writer, bool call_reader)
      {
        DDS::Security::CryptoKeyExchange_var keyexg = get_crypto_key_exchange();
        if (call_reader) {
          const DDS::Security::DatareaderCryptoHandle drch =
            get_handle_registry()->get_local_datareader_crypto_handle(reader);
          const DDS::Security::EndpointSecurityAttributes attribs =
            get_handle_registry()->get_local_datareader_security_attributes(reader);

          // It might not exist due to security attributes, and that's OK
          if (drch != DDS::HANDLE_NIL) {
            DDS::Security::DatawriterCryptoHandle dwch =
              generate_remote_matched_writer_crypto_handle(writer, reader);
            DatawriterCryptoTokenSeqMap::iterator t_iter =
              pending_remote_writer_crypto_tokens_.find(writer);
            if (t_iter != pending_remote_writer_crypto_tokens_.end()) {
              DDS::Security::SecurityException se;
              if (!keyexg->set_remote_datawriter_crypto_tokens(drch, dwch, t_iter->second, se)) {
                ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("DiscoveryBase::match_continue_security_enabled: ")
                  ACE_TEXT("Unable to set pending remote datawriter crypto tokens with ")
                  ACE_TEXT("crypto key exchange plugin. Security Exception[%d.%d]: %C\n"),
                    se.code, se.minor_code, se.message.in()));
              }
              pending_remote_writer_crypto_tokens_.erase(t_iter);
            }
            // Yes, this is different for remote datawriters than readers (see 8.8.9.3 vs 8.8.9.2)
            if (attribs.is_submessage_protected) {
              create_and_send_datareader_crypto_tokens(drch, reader, dwch, writer);
            }
          }
        }

        if (call_writer) {
          const DDS::Security::DatawriterCryptoHandle dwch =
            get_handle_registry()->get_local_datawriter_crypto_handle(writer);
          const DDS::Security::EndpointSecurityAttributes attribs =
            get_handle_registry()->get_local_datawriter_security_attributes(writer);

          // It might not exist due to security attributes, and that's OK
          if (dwch != DDS::HANDLE_NIL) {
            DDS::Security::DatareaderCryptoHandle drch =
              generate_remote_matched_reader_crypto_handle(reader, writer,
                                                           relay_only_readers_.count(reader));
            DatareaderCryptoTokenSeqMap::iterator t_iter =
              pending_remote_reader_crypto_tokens_.find(reader);
            if (t_iter != pending_remote_reader_crypto_tokens_.end()) {
              DDS::Security::SecurityException se;
              if (!keyexg->set_remote_datareader_crypto_tokens(dwch, drch, t_iter->second, se)) {
                ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("DiscoveryBase::match_continue_security_enabled: ")
                  ACE_TEXT("Unable to set pending remote datareader crypto tokens with crypto ")
                  ACE_TEXT("key exchange plugin. Security Exception[%d.%d]: %C\n"),
                    se.code, se.minor_code, se.message.in()));
              }
              pending_remote_reader_crypto_tokens_.erase(t_iter);
            }
            if (attribs.is_submessage_protected || attribs.is_payload_protected) {
              create_and_send_datawriter_crypto_tokens(dwch, writer, drch, reader);
            }
          }
        }
      }
#endif

      virtual bool is_expectant_opendds(const GUID_t& endpoint) const = 0;

      virtual bool shutting_down() const = 0;

      virtual void populate_transport_locator_sequence(TransportLocatorSeq*& tls,
                                                       DiscoveredSubscriptionIter& iter,
                                                       const RepoId& reader) = 0;

      virtual void populate_transport_locator_sequence(TransportLocatorSeq*& tls,
                                                       DiscoveredPublicationIter& iter,
                                                       const RepoId& reader) = 0;

      void remove_from_bit(const DiscoveredPublication& pub)
      {
        remove_from_bit_i(pub);
      }

      void remove_from_bit(const DiscoveredSubscription& sub)
      {
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

      inline void set_handle_registry(const Security::HandleRegistry_rch& hr)
      {
        handle_registry_ = hr;
      }

#endif

      ACE_Thread_Mutex& lock_;
      RepoId participant_id_;
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
      XTypes::TypeLookupService_rch type_lookup_service_;

      struct TypeIdOrigSeqNumber {
        GuidPrefix_t participant; // Prefix of remote participant
        XTypes::TypeIdentifier type_id; // Remote type
        SequenceNumber seq_number; // Of the original request
        bool secure; // Communicate via secure endpoints or not
        MonotonicTimePoint time_started;
      };

      // Map from the sequence number of the most recent request for a type to its TypeIdentifier
      // and the sequence number of the first request sent for that type. Every time a new request
      // is sent for a type, a new entry must be stored.
      typedef OPENDDS_MAP(SequenceNumber, TypeIdOrigSeqNumber) OrigSeqNumberMap;
      OrigSeqNumberMap orig_seq_numbers_;

#ifdef OPENDDS_SECURITY
      DDS::Security::AccessControl_var access_control_;
      DDS::Security::CryptoKeyFactory_var crypto_key_factory_;
      DDS::Security::CryptoKeyExchange_var crypto_key_exchange_;
      Security::HandleRegistry_rch handle_registry_;

      DDS::Security::PermissionsHandle permissions_handle_;
      DDS::Security::ParticipantCryptoHandle crypto_handle_;

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

      void ignore_domain_participant(const RepoId& ignoreId)
      {
        ACE_GUARD(ACE_Thread_Mutex, g, lock_);
        endpoint_manager().ignore(ignoreId);

        DiscoveredParticipantIter iter = participants_.find(ignoreId);
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
                      DataWriterCallbacks_rch publication,
                      const DDS::DataWriterQos& qos,
                      const TransportLocatorSeq& transInfo,
                      const DDS::PublisherQos& publisherQos,
                      const XTypes::TypeInformation& type_info)
      {
        return endpoint_manager().add_publication(topicId, publication, qos,
                                                  transInfo, publisherQos, type_info);
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
                       DataReaderCallbacks_rch subscription,
                       const DDS::DataReaderQos& qos,
                       const TransportLocatorSeq& transInfo,
                       const DDS::SubscriberQos& subscriberQos,
                       const char* filterClassName,
                       const char* filterExpr,
                       const DDS::StringSeq& params,
                       const XTypes::TypeInformation& type_info)
      {
        return endpoint_manager().add_subscription(topicId, subscription, qos, transInfo,
                                                   subscriberQos, filterClassName, filterExpr, params, type_info);
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

      DDS::Subscriber_var bit_subscriber() const { return bit_subscriber_; }

      void type_lookup_service(const XTypes::TypeLookupService_rch type_lookup_service)
      {
        endpoint_manager().type_lookup_service(type_lookup_service);
      }

    protected:

      struct DiscoveredParticipant {

        DiscoveredParticipant()
        : location_ih_(DDS::HANDLE_NIL)
        , bit_ih_(DDS::HANDLE_NIL)
        , seq_reset_count_(0)
#ifdef OPENDDS_SECURITY
        , have_spdp_info_(false)
        , have_sedp_info_(false)
        , have_auth_req_msg_(false)
        , have_handshake_msg_(false)
        , auth_state_(AUTH_STATE_HANDSHAKE)
        , handshake_state_(HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST)
        , is_requester_(false)
        , auth_req_sequence_number_(0)
        , handshake_sequence_number_(0)
        , identity_handle_(DDS::HANDLE_NIL)
        , handshake_handle_(DDS::HANDLE_NIL)
        , permissions_handle_(DDS::HANDLE_NIL)
        , extended_builtin_endpoints_(0)
#endif
        {
#ifdef OPENDDS_SECURITY
          security_info_.participant_security_attributes = 0;
          security_info_.plugin_participant_security_attributes = 0;
#endif
        }

        DiscoveredParticipant(
          const DiscoveredParticipantData& p,
          const SequenceNumber& seq)
        : pdata_(p)
        , location_ih_(DDS::HANDLE_NIL)
        , bit_ih_(DDS::HANDLE_NIL)
        , last_seq_(seq)
        , seq_reset_count_(0)
#ifdef OPENDDS_SECURITY
        , have_spdp_info_(false)
        , have_sedp_info_(false)
        , have_auth_req_msg_(false)
        , have_handshake_msg_(false)
        , auth_state_(AUTH_STATE_HANDSHAKE)
        , handshake_state_(HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST)
        , is_requester_(false)
        , auth_req_sequence_number_(0)
        , handshake_sequence_number_(0)
        , identity_handle_(DDS::HANDLE_NIL)
        , handshake_handle_(DDS::HANDLE_NIL)
        , permissions_handle_(DDS::HANDLE_NIL)
        , extended_builtin_endpoints_(0)
#endif
        {
          const RepoId guid = make_id(p.participantProxy.guidPrefix, DCPS::ENTITYID_PARTICIPANT);
          std::memcpy(location_data_.guid, &guid, sizeof(guid));
          location_data_.location = 0;
          location_data_.change_mask = 0;
          location_data_.local_timestamp.sec = 0;
          location_data_.local_timestamp.nanosec = 0;
          location_data_.ice_timestamp.sec = 0;
          location_data_.ice_timestamp.nanosec = 0;
          location_data_.relay_timestamp.sec = 0;
          location_data_.relay_timestamp.nanosec = 0;
          location_data_.local6_timestamp.sec = 0;
          location_data_.local6_timestamp.nanosec = 0;
          location_data_.ice6_timestamp.sec = 0;
          location_data_.ice6_timestamp.nanosec = 0;
          location_data_.relay6_timestamp.sec = 0;
          location_data_.relay6_timestamp.nanosec = 0;

#ifdef OPENDDS_SECURITY
          security_info_.participant_security_attributes = 0;
          security_info_.plugin_participant_security_attributes = 0;
#endif
        }

        DiscoveredParticipantData pdata_;
        struct LocationUpdate {
          ParticipantLocation mask_;
          ACE_INET_Addr from_;
          SystemTimePoint timestamp_;
          LocationUpdate() {}
          LocationUpdate(ParticipantLocation mask,
                         const ACE_INET_Addr& from,
                         const SystemTimePoint& timestamp)
            : mask_(mask), from_(from), timestamp_(timestamp) {}
        };
        typedef OPENDDS_VECTOR(LocationUpdate) LocationUpdateList;
        LocationUpdateList location_updates_;
        ParticipantLocationBuiltinTopicData location_data_;
        DDS::InstanceHandle_t location_ih_;

        ACE_INET_Addr local_address_;
        MonotonicTimePoint discovered_at_;
        MonotonicTimePoint lease_expiration_;
        DDS::InstanceHandle_t bit_ih_;
        SequenceNumber last_seq_;
        ACE_UINT16 seq_reset_count_;
#ifdef OPENDDS_SECURITY
        bool have_spdp_info_;
        ICE::AgentInfo spdp_info_;
        bool have_sedp_info_;
        ICE::AgentInfo sedp_info_;
        bool have_auth_req_msg_;
        DDS::Security::ParticipantStatelessMessage auth_req_msg_;
        bool have_handshake_msg_;
        DDS::Security::ParticipantStatelessMessage handshake_msg_;
        MonotonicTimePoint stateless_msg_deadline_;

        MonotonicTimePoint handshake_deadline_;
        AuthState auth_state_;
        HandshakeState handshake_state_;
        bool is_requester_;
        CORBA::LongLong auth_req_sequence_number_;
        CORBA::LongLong handshake_sequence_number_;

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
        DDS::Security::ParticipantCryptoTokenSeq crypto_tokens_;
        DDS::Security::ExtendedBuiltinEndpointSet_t extended_builtin_endpoints_;
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

      void remove_discovered_participant(DiscoveredParticipantIter& iter)
      {
        if (iter == participants_.end()) {
          return;
        }
        RepoId part_id = iter->first;
        bool removed = endpoint_manager().disassociate(iter->second.pdata_);
        iter = participants_.find(part_id); // refresh iter after disassociate, which can unlock
        if (iter == participants_.end()) {
          return;
        }
        if (removed) {
#ifndef DDS_HAS_MINIMUM_BIT
          ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
          ParticipantLocationBuiltinTopicDataDataReaderImpl* loc_bit = part_loc_bit();
          // bit may be null if the DomainParticipant is shutting down
          if ((bit && iter->second.bit_ih_ != DDS::HANDLE_NIL) ||
              (loc_bit && iter->second.location_ih_ != DDS::HANDLE_NIL)) {
            {
              const DDS::InstanceHandle_t bit_ih = iter->second.bit_ih_;
              const DDS::InstanceHandle_t location_ih = iter->second.location_ih_;

              ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);
              ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
              if (bit && bit_ih != DDS::HANDLE_NIL) {
                bit->set_instance_state(bit_ih,
                                        DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
              }
              if (loc_bit && location_ih != DDS::HANDLE_NIL) {
                loc_bit->set_instance_state(location_ih,
                                            DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
              }
            }
            iter = participants_.find(part_id);
            if (iter == participants_.end()) {
              return;
            }
          }
#endif /* DDS_HAS_MINIMUM_BIT */
          if (DCPS_debug_level > 3) {
            GuidConverter conv(iter->first);
            ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) LocalParticipant::remove_discovered_participant")
                       ACE_TEXT(" - erasing %C\n"), OPENDDS_STRING(conv).c_str()));
          }

          remove_discovered_participant_i(iter);

          participants_.erase(iter);
        }
      }

      virtual void remove_discovered_participant_i(DiscoveredParticipantIter&) {}

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

    DCPS::ConnectionRecordDataReaderImpl* connection_record_bit()
    {
      if (!bit_subscriber_.in())
        return 0;

      DDS::DataReader_var d =
        bit_subscriber_->lookup_datareader(DCPS::BUILT_IN_CONNECTION_RECORD_TOPIC);
      return dynamic_cast<ConnectionRecordDataReaderImpl*>(d.in());
    }

    DCPS::InternalThreadBuiltinTopicDataDataReaderImpl* internal_thread_bit()
    {
      if (!bit_subscriber_.in())
        return 0;

      DDS::DataReader_var d =
        bit_subscriber_->lookup_datareader(DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
      return dynamic_cast<InternalThreadBuiltinTopicDataDataReaderImpl*>(d.in());
    }
#endif /* DDS_HAS_MINIMUM_BIT */

      mutable ACE_Thread_Mutex lock_;
      DDS::Subscriber_var bit_subscriber_;
      DDS::DomainParticipantQos qos_;
      DiscoveredParticipantMap participants_;
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

        dr_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay =
          TheServiceParticipant->bit_autopurge_nowriter_samples_delay();
        dr_qos.reader_data_lifecycle.autopurge_disposed_samples_delay =
          TheServiceParticipant->bit_autopurge_disposed_samples_delay();

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

        DDS::TopicDescription_var bit_connection_record_topic =
          participant->lookup_topicdescription(BUILT_IN_CONNECTION_RECORD_TOPIC);
        create_bit_dr(bit_connection_record_topic, BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE,
                      sub, dr_qos);

        DDS::TopicDescription_var bit_internal_thread_topic =
          participant->lookup_topicdescription(BUILT_IN_INTERNAL_THREAD_TOPIC);
        create_bit_dr(bit_internal_thread_topic, BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE,
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

        participant->shutdown();
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
                                     DataWriterCallbacks_rch publication,
                                     const DDS::DataWriterQos& qos,
                                     const TransportLocatorSeq& transInfo,
                                     const DDS::PublisherQos& publisherQos,
                                     const XTypes::TypeInformation& type_info)
      {
        return get_part(domainId, participantId)->add_publication(topicId, publication, qos, transInfo, publisherQos, type_info);
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
                                      DataReaderCallbacks_rch subscription,
                                      const DDS::DataReaderQos& qos,
                                      const TransportLocatorSeq& transInfo,
                                      const DDS::SubscriberQos& subscriberQos,
                                      const char* filterClassName,
                                      const char* filterExpr,
                                      const DDS::StringSeq& params,
                                      const XTypes::TypeInformation& type_info)
      {
        return get_part(domainId, participantId)->
          add_subscription(
            topicId, subscription, qos, transInfo, subscriberQos, filterClassName, filterExpr, params, type_info);
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


      virtual void set_type_lookup_service(DDS::DomainId_t domainId,
                                           const RepoId& participantId,
                                           XTypes::TypeLookupService_rch type_lookup_service)
      {
        get_part(domainId, participantId)->type_lookup_service(type_lookup_service);
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
