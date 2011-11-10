/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsInfo.h"
#include "BaseMessageTypes.h"

namespace OpenDDS {
namespace RTPS {

using OpenDDS::DCPS::RepoId;


// Participant operations:

bool
RtpsInfo::attach_participant(DDS::DomainId_t domainId,
                             const RepoId& participantId)
{
  return false; // This is just for DCPSInfoRepo?
}

DCPS::AddDomainStatus
RtpsInfo::add_domain_participant(DDS::DomainId_t domain,
                                 const DDS::DomainParticipantQos& qos)
{
  DCPS::AddDomainStatus ads = {RepoId(), false /*federated*/};
  guid_gen_.populate(ads.id);
  ads.id.entityId = RTPS::ENTITYID_PARTICIPANT;
  participants_[domain][ads.id] = new SPDP(domain, ads.id, qos);
  return ads;
}

void
RtpsInfo::remove_domain_participant(DDS::DomainId_t domain,
                                    const RepoId& participantId)
{
  participants_[domain].erase(participantId);
  if (participants_[domain].empty()) {
    participants_.erase(domain);
  }
}

void
RtpsInfo::ignore_domain_participant(DDS::DomainId_t domain,
                                    const RepoId& myParticipantId,
                                    const RepoId& ignoreId)
{
  participants_[domain][myParticipantId]->ignore_domain_participant(ignoreId);
}

bool
RtpsInfo::update_domain_participant_qos(DDS::DomainId_t domain,
                                        const RepoId& participant,
                                        const DDS::DomainParticipantQos& qos)
{
  return participants_[domain][participant]->update_domain_participant_qos(qos);
}


// Topic operations:

OpenDDS::DCPS::TopicStatus
RtpsInfo::assert_topic(OpenDDS::DCPS::RepoId_out topicId,
                       DDS::DomainId_t domainId, const RepoId& participantId,
                       const char* topicName, const char* dataTypeName,
                       const DDS::TopicQos& qos)
{
  return DCPS::INTERNAL_ERROR;
}

OpenDDS::DCPS::TopicStatus
RtpsInfo::find_topic(DDS::DomainId_t domainId, const char* topicName,
                     CORBA::String_out dataTypeName, DDS::TopicQos_out qos,
                     OpenDDS::DCPS::RepoId_out topicId)
{
  return DCPS::INTERNAL_ERROR;
}

OpenDDS::DCPS::TopicStatus
RtpsInfo::remove_topic(DDS::DomainId_t domainId, const RepoId& participantId,
                       const RepoId& topicId)
{
  return DCPS::INTERNAL_ERROR;
}

OpenDDS::DCPS::TopicStatus
RtpsInfo::enable_topic(DDS::DomainId_t domainId, const RepoId& participantId,
                       const RepoId& topicId)
{
  return DCPS::INTERNAL_ERROR;
}

void
RtpsInfo::ignore_topic(DDS::DomainId_t domainId, const RepoId& myParticipantId,
                       const RepoId& ignoreId)
{
}

bool
RtpsInfo::update_topic_qos(const RepoId& topicId, DDS::DomainId_t domainId,
                           const RepoId& participantId, const DDS::TopicQos& qos)
{
  return false;
}


// Publication operations:

RepoId
RtpsInfo::add_publication(DDS::DomainId_t domainId, const RepoId& participantId,
                          const RepoId& topicId,
                          OpenDDS::DCPS::DataWriterRemote_ptr publication,
                          const DDS::DataWriterQos& qos,
                          const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
                          const DDS::PublisherQos& publisherQos)
{
  return RepoId();
}

void
RtpsInfo::remove_publication(DDS::DomainId_t domainId,
                             const RepoId& participantId,
                             const RepoId& publicationId)
{
}

void
RtpsInfo::ignore_publication(DDS::DomainId_t domainId,
                             const RepoId& myParticipantId,
                             const RepoId& ignoreId)
{
}

bool
RtpsInfo::update_publication_qos(DDS::DomainId_t domainId, const RepoId& partId,
                                 const RepoId& dwId,
                                 const DDS::DataWriterQos& qos,
                                 const DDS::PublisherQos& publisherQos)
{
  return false;
}


// Subscription operations:

RepoId
RtpsInfo::add_subscription(DDS::DomainId_t domainId,
                           const RepoId& participantId, const RepoId& topicId,
                           OpenDDS::DCPS::DataReaderRemote_ptr subscription,
                           const DDS::DataReaderQos& qos,
                           const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
                           const DDS::SubscriberQos& subscriberQos,
                           const char* filterExpression,
                           const DDS::StringSeq& exprParams)
{
  return RepoId();
}

void
RtpsInfo::remove_subscription(DDS::DomainId_t domainId,
                              const RepoId& participantId,
                              const RepoId& subscriptionId)
{
}

void
RtpsInfo::ignore_subscription(DDS::DomainId_t domainId,
                              const RepoId& myParticipantId,
                              const RepoId& ignoreId)
{
}

bool
RtpsInfo::update_subscription_qos(DDS::DomainId_t domainId,
                                  const RepoId& partId, const RepoId& drId,
                                  const DDS::DataReaderQos& qos,
                                  const DDS::SubscriberQos& subscriberQos)
{
  return false;
}

bool
RtpsInfo::update_subscription_params(DDS::DomainId_t domainId,
                                     const RepoId& participantId,
                                     const RepoId& subscriptionId,
                                     const DDS::StringSeq& params)

{
  return false;
}

// Managing reader/writer associations:

void
RtpsInfo::association_complete(DDS::DomainId_t domainId,
                               const RepoId& participantId,
                               const RepoId& localId, const RepoId& remoteId)
{
}

void
RtpsInfo::disassociate_participant(DDS::DomainId_t domainId,
                                   const RepoId& local_id,
                                   const RepoId& remote_id)
{
}

void
RtpsInfo::disassociate_subscription(DDS::DomainId_t domainId,
                                    const RepoId& participantId,
                                    const RepoId& local_id,
                                    const RepoId& remote_id)
{
}

void
RtpsInfo::disassociate_publication(DDS::DomainId_t domainId,
                                   const RepoId& participantId,
                                   const RepoId& local_id,
                                   const RepoId& remote_id)
{
}


}
}
