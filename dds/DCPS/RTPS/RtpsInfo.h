/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_RTPSINFO_H
#define OPENDDS_RTPS_RTPSINFO_H

#include "dds/DdsDcpsInfoS.h"

#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/RcHandle_T.h"

#include "GuidGenerator.h"
#include "Spdp.h"

#include "ace/Synch_Traits.h"

#include <map>
#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace RTPS {

/// The RtpsInfo object implements the DCPSInfo CORBA Interface for the
/// RTPS-standard discovery protocol.  A single RtpsInfo is used per "discovery
/// object" in DCPS (see class RtpsDiscovery).
class RtpsInfo : public virtual POA_OpenDDS::DCPS::DCPSInfo {
public:
  explicit RtpsInfo(RtpsDiscovery* disco);


  // Participant operations:

  virtual bool attach_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId);

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos);

  virtual void remove_domain_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId);

  virtual void ignore_domain_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_domain_participant_qos(
    DDS::DomainId_t domain,
    const OpenDDS::DCPS::RepoId& participantId,
    const DDS::DomainParticipantQos& qos);


  // Topic operations:

  virtual OpenDDS::DCPS::TopicStatus assert_topic(
    OpenDDS::DCPS::RepoId_out topicId,
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey);

  virtual OpenDDS::DCPS::TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    OpenDDS::DCPS::RepoId_out topicId);

  virtual OpenDDS::DCPS::TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId);

  virtual void ignore_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_topic_qos(
    const OpenDDS::DCPS::RepoId& topicId,
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const DDS::TopicQos& qos);


  // Publication operations:

  virtual OpenDDS::DCPS::RepoId add_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataWriterRemote_ptr publication,
    const DDS::DataWriterQos& qos,
    const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos);

  virtual void remove_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& publicationId);

  virtual void ignore_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_publication_qos(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& partId,
    const OpenDDS::DCPS::RepoId& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos);


  // Subscription operations:

  virtual OpenDDS::DCPS::RepoId add_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataReaderRemote_ptr subscription,
    const DDS::DataReaderQos& qos,
    const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterExpression,
    const DDS::StringSeq& exprParams);

  virtual void remove_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId);

  virtual void ignore_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& partId,
    const OpenDDS::DCPS::RepoId& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos);

  virtual bool update_subscription_params(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId,
    const DDS::StringSeq& params);


  // Managing reader/writer associations:

  virtual void association_complete(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& localId,
    const OpenDDS::DCPS::RepoId& remoteId);

  virtual void disassociate_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& localId,
    const OpenDDS::DCPS::RepoId& remoteId);

  virtual void disassociate_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& localId,
    const OpenDDS::DCPS::RepoId& remoteId);

  virtual void disassociate_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& localId,
    const OpenDDS::DCPS::RepoId& remoteId);


  virtual void shutdown() {} // no-op for RTPS


  // used by RtpsDiscovery:

  void init_bit(const DDS::Subscriber_var& bit_subscriber);

  DCPS::RepoId bit_key_to_repo_id(DDS::DomainId_t domainId,
                                  const DCPS::RepoId& participantId,
                                  const char* bit_topic_name,
                                  const DDS::BuiltinTopicKey_t& key);

private:

  RtpsDiscovery* disco_;

  std::map<DDS::DomainId_t,
           std::map<DCPS::RepoId, DCPS::RcHandle<Spdp>, DCPS::GUID_tKeyLessThan>
          > participants_;

  std::map<DDS::DomainId_t, std::map<std::string, Sedp::TopicDetails> > topics_;
  std::map<DDS::DomainId_t, std::map<std::string, unsigned int> > topic_use_;

  /// Guids will be unique within this RTPS configuration
  GuidGenerator guid_gen_;
  ACE_Thread_Mutex lock_;
};

}
}

#endif // OPENDDS_RTPS_RTPSINFO_H
