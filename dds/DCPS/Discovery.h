/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DDS_DCPS_DISCOVERY_H
#define OPENDDS_DDS_DCPS_DISCOVERY_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "RcObject_T.h"
#include "RcHandle_T.h"

#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DdsDcpsDataWriterRemoteC.h"
#include "dds/DdsDcpsSubscriptionC.h"

#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class ACE_Configuration_Heap;

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;

/**
 * @class Discovery
 *
 * @brief Discovery Strategy interface class
 *
 * This class is an abstract class that acts as an interface for both
 * InfoRepo-based discovery and RTPS Discovery.
 *
 */
class OpenDDS_Dcps_Export Discovery : public RcObject<ACE_SYNCH_MUTEX> {
public:
  /// Key type for storing discovery objects.
  /// Probably should just be Discovery::Key
  typedef std::string RepoKey;

  explicit Discovery(const RepoKey& key) : key_(key) { }

  /// Key value for the default repository IOR.
  static const std::string DEFAULT_REPO;
  static const std::string DEFAULT_RTPS;

  // TODO: NEED TO REMOVE THIS once Service_Participant::repository_lost has been refactored
  virtual bool active() { return true; }

  virtual DDS::Subscriber_ptr init_bit(DomainParticipantImpl* participant) = 0;

  virtual RepoId bit_key_to_repo_id(DomainParticipantImpl* participant,
                                    const char* bit_topic_name,
                                    const DDS::BuiltinTopicKey_t& key) const = 0;

  RepoKey key() const { return this->key_; }

  class OpenDDS_Dcps_Export Config {
  public:
    virtual ~Config();
    virtual int discovery_config(ACE_Configuration_Heap& cf) = 0;
  };

  virtual bool attach_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId) = 0;

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos) = 0;

  virtual bool remove_domain_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId) = 0;

  virtual bool ignore_domain_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId) = 0;

  virtual bool update_domain_participant_qos(
    DDS::DomainId_t domain,
    const OpenDDS::DCPS::RepoId& participantId,
    const DDS::DomainParticipantQos& qos) = 0;


  // Topic operations:

  virtual OpenDDS::DCPS::TopicStatus assert_topic(
    OpenDDS::DCPS::RepoId_out topicId,
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey) = 0;

  virtual OpenDDS::DCPS::TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    OpenDDS::DCPS::RepoId_out topicId) = 0;

  virtual OpenDDS::DCPS::TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId) = 0;

  virtual bool ignore_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId) = 0;

  virtual bool update_topic_qos(
    const OpenDDS::DCPS::RepoId& topicId,
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const DDS::TopicQos& qos) = 0;


  // Publication operations:

  virtual OpenDDS::DCPS::RepoId add_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataWriterRemote_ptr publication,
    const DDS::DataWriterQos& qos,
    const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos) = 0;

  virtual bool remove_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& publicationId) = 0;

  virtual bool ignore_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId) = 0;

  virtual bool update_publication_qos(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& partId,
    const OpenDDS::DCPS::RepoId& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos) = 0;


  // Subscription operations:

  /// add the passed in subscription into discovery.
  /// Discovery does not participate in memory management
  /// for the subscription pointer, so it requires that
  /// the subscription pointer remain valid until
  /// remove_subscription is called.
  virtual OpenDDS::DCPS::RepoId add_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataReaderCallbacks* subscription,
    const DDS::DataReaderQos& qos,
    const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterExpression,
    const DDS::StringSeq& exprParams) = 0;

  virtual bool remove_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId) = 0;

  virtual bool ignore_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId) = 0;

  virtual bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& partId,
    const OpenDDS::DCPS::RepoId& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos) = 0;

  virtual bool update_subscription_params(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId,
    const DDS::StringSeq& params) = 0;


  // Managing reader/writer associations:

  virtual void association_complete(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& localId,
    const OpenDDS::DCPS::RepoId& remoteId) = 0;

protected:
#ifndef DDS_HAS_MINIMUM_BIT
  DDS::ReturnCode_t create_bit_topics(DomainParticipantImpl* participant);
#endif

private:
  RepoKey        key_;

};

typedef RcHandle<Discovery> Discovery_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_DISCOVERY_H  */
