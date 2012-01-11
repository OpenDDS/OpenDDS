/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsInfo.h"
#include "BaseMessageTypes.h"

#include "dds/DCPS/DomainParticipantImpl.h"

namespace OpenDDS {
namespace RTPS {
using DCPS::RepoId;

RtpsInfo::RtpsInfo(RtpsDiscovery* disco)
  : disco_(disco)
{
}

// Participant operations:

bool
RtpsInfo::attach_participant(DDS::DomainId_t /*domainId*/,
                             const RepoId& /*participantId*/)
{
  return false; // This is just for DCPSInfoRepo?
}

DCPS::AddDomainStatus
RtpsInfo::add_domain_participant(DDS::DomainId_t domain,
                                 const DDS::DomainParticipantQos& qos)
{
  DCPS::AddDomainStatus ads = {RepoId(), false /*federated*/};
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ads);
  guid_gen_.populate(ads.id);
  ads.id.entityId = ENTITYID_PARTICIPANT;
  try {
    participants_[domain][ads.id] = new Spdp(domain, ads.id, qos, disco_);
  } catch (const std::exception& e) {
    ads.id = GUID_UNKNOWN;
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsInfo::add_domain_participant() - "
      "failed to initialize RTPS Simple Participant Discovery Protocol: %C\n",
      e.what()));
  }
  return ads;
}

void
RtpsInfo::remove_domain_participant(DDS::DomainId_t domain,
                                    const RepoId& participantId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
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
  get_part(domain, myParticipantId)->ignore_domain_participant(ignoreId);
}

bool
RtpsInfo::update_domain_participant_qos(DDS::DomainId_t domain,
                                        const RepoId& participant,
                                        const DDS::DomainParticipantQos& qos)
{
  return get_part(domain, participant)->update_domain_participant_qos(qos);
}

void
RtpsInfo::init_bit(const DDS::Subscriber_var& bit_subscriber)
{
  DDS::DomainParticipant_var participant = bit_subscriber->get_participant();
  DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<DCPS::DomainParticipantImpl*>(participant.in());
  DCPS::RepoId participantId = dpi->get_id();
  DDS::DomainId_t domainId = participant->get_domain_id();

  get_part(domainId, participantId)->bit_subscriber(bit_subscriber);
}

RepoId
RtpsInfo::bit_key_to_repo_id(DDS::DomainId_t domainId,
                             const RepoId& partId,
                             const char* bit_topic_name,
                             const DDS::BuiltinTopicKey_t& key)
{
  return get_part(domainId, partId)->bit_key_to_repo_id(bit_topic_name, key);
}

// Topic operations:

DCPS::TopicStatus
RtpsInfo::assert_topic(DCPS::RepoId_out topicId,
                       DDS::DomainId_t domainId, const RepoId& participantId,
                       const char* topicName, const char* dataTypeName,
                       const DDS::TopicQos& qos, bool hasDcpsKey)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  std::map<DDS::DomainId_t,
           std::map<std::string, Sedp::TopicDetails> >::iterator topic_it =
    topics_.find(domainId);
  if (topic_it != topics_.end()) {
    const std::map<std::string, Sedp::TopicDetails>::iterator it =
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
    Sedp::TopicDetails& td = topics_[domainId][topicName];
    td.data_type_ = dataTypeName;
    td.qos_ = qos;
    td.repo_id_ = topicId;
    ++topic_use_[domainId][topicName];
  }
  return stat;
}

DCPS::TopicStatus
RtpsInfo::find_topic(DDS::DomainId_t domainId, const char* topicName,
                     CORBA::String_out dataTypeName, DDS::TopicQos_out qos,
                     DCPS::RepoId_out topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  std::map<DDS::DomainId_t,
           std::map<std::string, Sedp::TopicDetails> >::iterator topic_it =
    topics_.find(domainId);
  if (topic_it == topics_.end()) {
    return DCPS::NOT_FOUND;
  }
  std::map<std::string, Sedp::TopicDetails>::iterator iter =
    topic_it->second.find(topicName);
  if (iter == topic_it->second.end()) {
    return DCPS::NOT_FOUND;
  }
  Sedp::TopicDetails& td = iter->second;
  dataTypeName = td.data_type_.c_str();
  qos = new DDS::TopicQos(td.qos_);
  topicId = td.repo_id_;
  ++topic_use_[domainId][topicName];
  return DCPS::FOUND;
}

DCPS::TopicStatus
RtpsInfo::remove_topic(DDS::DomainId_t domainId, const RepoId& participantId,
                       const RepoId& topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  std::map<DDS::DomainId_t,
           std::map<std::string, Sedp::TopicDetails> >::iterator topic_it =
    topics_.find(domainId);
  if (topic_it == topics_.end()) {
    return DCPS::NOT_FOUND;
  }

  std::string name;
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

void
RtpsInfo::ignore_topic(DDS::DomainId_t domainId, const RepoId& myParticipantId,
                       const RepoId& ignoreId)
{
  get_part(domainId, myParticipantId)->ignore_topic(ignoreId);
}

bool
RtpsInfo::update_topic_qos(const RepoId& topicId, DDS::DomainId_t domainId,
                           const RepoId& participantId, const DDS::TopicQos& qos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  std::string name;
  // Safe to hold lock while calling update_topic_qos
  if (participants_[domainId][participantId]->update_topic_qos(topicId,
                                                               qos, name)) {
    topics_[domainId][name].qos_ = qos;
    return true;
  }
  return false;
}


// Publication operations:

RepoId
RtpsInfo::add_publication(DDS::DomainId_t domainId, const RepoId& participantId,
                          const RepoId& topicId,
                          DCPS::DataWriterRemote_ptr publication,
                          const DDS::DataWriterQos& qos,
                          const DCPS::TransportLocatorSeq& transInfo,
                          const DDS::PublisherQos& publisherQos)
{
  return get_part(domainId, participantId)->add_publication(
    topicId, publication, qos, transInfo, publisherQos);
}

void
RtpsInfo::remove_publication(DDS::DomainId_t domainId,
                             const RepoId& participantId,
                             const RepoId& publicationId)
{
  get_part(domainId, participantId)->remove_publication(publicationId);
}

void
RtpsInfo::ignore_publication(DDS::DomainId_t domainId,
                             const RepoId& participantId,
                             const RepoId& ignoreId)
{
  get_part(domainId, participantId)->ignore_publication(ignoreId);
}

bool
RtpsInfo::update_publication_qos(DDS::DomainId_t domainId, const RepoId& partId,
                                 const RepoId& dwId,
                                 const DDS::DataWriterQos& qos,
                                 const DDS::PublisherQos& publisherQos)
{
  return get_part(domainId, partId)->update_publication_qos(dwId, qos,
                                                            publisherQos);
}


// Subscription operations:

RepoId
RtpsInfo::add_subscription(DDS::DomainId_t domainId,
                           const RepoId& participantId, const RepoId& topicId,
                           DCPS::DataReaderRemote_ptr subscription,
                           const DDS::DataReaderQos& qos,
                           const DCPS::TransportLocatorSeq& transInfo,
                           const DDS::SubscriberQos& subscriberQos,
                           const char* filterExpr,
                           const DDS::StringSeq& params)
{
  return get_part(domainId, participantId)->add_subscription(
    topicId, subscription, qos, transInfo, subscriberQos, filterExpr, params);
}

void
RtpsInfo::remove_subscription(DDS::DomainId_t domainId,
                              const RepoId& participantId,
                              const RepoId& subscriptionId)
{
  get_part(domainId, participantId)->remove_subscription(subscriptionId);
}

void
RtpsInfo::ignore_subscription(DDS::DomainId_t domainId,
                              const RepoId& participantId,
                              const RepoId& ignoreId)
{
  get_part(domainId, participantId)->ignore_subscription(ignoreId);
}

bool
RtpsInfo::update_subscription_qos(DDS::DomainId_t domainId,
                                  const RepoId& partId, const RepoId& drId,
                                  const DDS::DataReaderQos& qos,
                                  const DDS::SubscriberQos& subQos)
{
  return get_part(domainId, partId)->update_subscription_qos(drId, qos, subQos);
}

bool
RtpsInfo::update_subscription_params(DDS::DomainId_t domainId,
                                     const RepoId& partId,
                                     const RepoId& subId,
                                     const DDS::StringSeq& params)

{
  return get_part(domainId, partId)->update_subscription_params(subId, params);
}


// Managing reader/writer associations:

void
RtpsInfo::association_complete(DDS::DomainId_t domainId,
                               const RepoId& participantId,
                               const RepoId& localId, const RepoId& remoteId)
{
  get_part(domainId, participantId)->association_complete(localId, remoteId);
}

void
RtpsInfo::disassociate_participant(DDS::DomainId_t domainId,
                                   const RepoId& localId,
                                   const RepoId& remoteId)
{
  get_part(domainId, localId)->disassociate_participant(remoteId);
}

void
RtpsInfo::disassociate_subscription(DDS::DomainId_t domainId,
                                    const RepoId& participantId,
                                    const RepoId& localId,
                                    const RepoId& remoteId)
{
  get_part(domainId, participantId)->disassociate_subscription(localId,
                                                               remoteId);
}

void
RtpsInfo::disassociate_publication(DDS::DomainId_t domainId,
                                   const RepoId& participantId,
                                   const RepoId& localId,
                                   const RepoId& remoteId)
{
  get_part(domainId, participantId)->disassociate_publication(localId,
                                                              remoteId);
}


DCPS::RcHandle<Spdp>
RtpsInfo::get_part(const DDS::DomainId_t domain_id,
                   const DCPS::RepoId& part_id)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::RcHandle<Spdp>());
  return participants_[domain_id][part_id];
}

}
}
