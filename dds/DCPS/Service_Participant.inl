/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
Discovery::RepoKey
Service_Participant::domain_to_repo(const DDS::DomainId_t domain) const
{
  DomainRepoMap::const_iterator where = this->domainRepoMap_.find(domain);

  if (where == this->domainRepoMap_.end()) {
    return Discovery::DEFAULT_REPO;

  } else {
    return where->second;
  }
}

ACE_INLINE
const DDS::UserDataQosPolicy&
Service_Participant::initial_UserDataQosPolicy() const
{
  return initial_UserDataQosPolicy_;
}

ACE_INLINE
const DDS::TopicDataQosPolicy&
Service_Participant::initial_TopicDataQosPolicy() const
{
  return initial_TopicDataQosPolicy_;
}

ACE_INLINE
const DDS::GroupDataQosPolicy&
Service_Participant::initial_GroupDataQosPolicy() const
{
  return initial_GroupDataQosPolicy_;
}

ACE_INLINE
const DDS::TransportPriorityQosPolicy&
Service_Participant::initial_TransportPriorityQosPolicy() const
{
  return initial_TransportPriorityQosPolicy_;
}

ACE_INLINE
const DDS::LifespanQosPolicy&
Service_Participant::initial_LifespanQosPolicy() const
{
  return initial_LifespanQosPolicy_;
}

ACE_INLINE
const DDS::DurabilityQosPolicy&
Service_Participant::initial_DurabilityQosPolicy() const
{
  return initial_DurabilityQosPolicy_;
}

ACE_INLINE
const DDS::DurabilityServiceQosPolicy&
Service_Participant::initial_DurabilityServiceQosPolicy() const
{
  return initial_DurabilityServiceQosPolicy_;
}

ACE_INLINE
const DDS::PresentationQosPolicy&
Service_Participant::initial_PresentationQosPolicy() const
{
  return initial_PresentationQosPolicy_;
}

ACE_INLINE
const DDS::DeadlineQosPolicy&
Service_Participant::initial_DeadlineQosPolicy() const
{
  return initial_DeadlineQosPolicy_;
}

ACE_INLINE
const DDS::LatencyBudgetQosPolicy&
Service_Participant::initial_LatencyBudgetQosPolicy() const
{
  return initial_LatencyBudgetQosPolicy_;
}

ACE_INLINE
const DDS::OwnershipQosPolicy&
Service_Participant::initial_OwnershipQosPolicy() const
{
  return initial_OwnershipQosPolicy_;
}

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
ACE_INLINE
const DDS::OwnershipStrengthQosPolicy&
Service_Participant::initial_OwnershipStrengthQosPolicy() const
{
  return initial_OwnershipStrengthQosPolicy_;
}
#endif

ACE_INLINE
const DDS::LivelinessQosPolicy&
Service_Participant::initial_LivelinessQosPolicy() const
{
  return initial_LivelinessQosPolicy_;
}

ACE_INLINE
const DDS::TimeBasedFilterQosPolicy&
Service_Participant::initial_TimeBasedFilterQosPolicy() const
{
  return initial_TimeBasedFilterQosPolicy_;
}

ACE_INLINE
const DDS::PartitionQosPolicy&
Service_Participant::initial_PartitionQosPolicy() const
{
  return initial_PartitionQosPolicy_;
}

ACE_INLINE
const DDS::ReliabilityQosPolicy&
Service_Participant::initial_ReliabilityQosPolicy() const
{
  return initial_ReliabilityQosPolicy_;
}

ACE_INLINE
const DDS::DestinationOrderQosPolicy&
Service_Participant::initial_DestinationOrderQosPolicy() const
{
  return initial_DestinationOrderQosPolicy_;
}

ACE_INLINE
const DDS::HistoryQosPolicy&
Service_Participant::initial_HistoryQosPolicy() const
{
  return initial_HistoryQosPolicy_;
}

ACE_INLINE
const DDS::ResourceLimitsQosPolicy&
Service_Participant::initial_ResourceLimitsQosPolicy() const
{
  return initial_ResourceLimitsQosPolicy_;
}

ACE_INLINE
const DDS::EntityFactoryQosPolicy&
Service_Participant::initial_EntityFactoryQosPolicy() const
{
  return initial_EntityFactoryQosPolicy_;
}

ACE_INLINE
const DDS::WriterDataLifecycleQosPolicy&
Service_Participant::initial_WriterDataLifecycleQosPolicy() const
{
  return initial_WriterDataLifecycleQosPolicy_;
}

ACE_INLINE
const DDS::ReaderDataLifecycleQosPolicy&
Service_Participant::initial_ReaderDataLifecycleQosPolicy() const
{
  return initial_ReaderDataLifecycleQosPolicy_;
}

ACE_INLINE
const DDS::PropertyQosPolicy&
Service_Participant::initial_PropertyQosPolicy() const
{
  return initial_PropertyQosPolicy_;
}

ACE_INLINE
const DDS::DataRepresentationQosPolicy&
Service_Participant::initial_DataRepresentationQosPolicy() const
{
  return initial_DataRepresentationQosPolicy_;
}

ACE_INLINE
const DDS::DomainParticipantFactoryQos&
Service_Participant::initial_DomainParticipantFactoryQos() const
{
  return initial_DomainParticipantFactoryQos_;
}

ACE_INLINE
const DDS::DomainParticipantQos&
Service_Participant::initial_DomainParticipantQos() const
{
  return initial_DomainParticipantQos_;
}

ACE_INLINE
const DDS::TopicQos&
Service_Participant::initial_TopicQos() const
{
  return initial_TopicQos_;
}

ACE_INLINE
const DDS::DataWriterQos&
Service_Participant::initial_DataWriterQos() const
{
  return initial_DataWriterQos_;
}

ACE_INLINE
const DDS::PublisherQos&
Service_Participant::initial_PublisherQos() const
{
  return initial_PublisherQos_;
}

ACE_INLINE
const DDS::DataReaderQos&
Service_Participant::initial_DataReaderQos() const
{
  return initial_DataReaderQos_;
}

ACE_INLINE
const DDS::SubscriberQos&
Service_Participant::initial_SubscriberQos() const
{
  return initial_SubscriberQos_;
}

ACE_INLINE
const DDS::TypeConsistencyEnforcementQosPolicy&
Service_Participant::initial_TypeConsistencyEnforcementQosPolicy() const
{
  return initial_TypeConsistencyEnforcementQosPolicy_;
}

ACE_INLINE
int&
Service_Participant::federation_recovery_duration()
{
  return this->federation_recovery_duration_;
}

ACE_INLINE
int
Service_Participant::federation_recovery_duration() const
{
  return this->federation_recovery_duration_;
}

ACE_INLINE
int&
Service_Participant::federation_initial_backoff_seconds()
{
  return this->federation_initial_backoff_seconds_;
}

ACE_INLINE
int
Service_Participant::federation_initial_backoff_seconds() const
{
  return this->federation_initial_backoff_seconds_;
}

ACE_INLINE
int&
Service_Participant::federation_backoff_multiplier()
{
  return this->federation_backoff_multiplier_;
}

ACE_INLINE
int
Service_Participant::federation_backoff_multiplier() const
{
  return this->federation_backoff_multiplier_;
}

ACE_INLINE
int&
Service_Participant::federation_liveliness()
{
  return this->federation_liveliness_;
}

ACE_INLINE
int
Service_Participant::federation_liveliness() const
{
  return this->federation_liveliness_;
}

ACE_INLINE
long&
Service_Participant::scheduler()
{
  return this->scheduler_;
}

ACE_INLINE
long
Service_Participant::scheduler() const
{
  return this->scheduler_;
}

ACE_INLINE
TimeDuration
Service_Participant::pending_timeout() const
{
  return this->pending_timeout_;
}

ACE_INLINE
void Service_Participant::pending_timeout(const TimeDuration& value)
{
  pending_timeout_ = value;
}

ACE_INLINE
MonotonicTimePoint Service_Participant::new_pending_timeout_deadline() const
{
  return pending_timeout_.is_zero() ?
    MonotonicTimePoint() : MonotonicTimePoint::now() + pending_timeout_;
}

ACE_INLINE
int
Service_Participant::priority_min() const
{
  return this->priority_min_;
}

ACE_INLINE
int
Service_Participant::priority_max() const
{
  return this->priority_max_;
}

ACE_INLINE
bool&
Service_Participant::publisher_content_filter()
{
  return this->publisher_content_filter_;
}

ACE_INLINE
bool
Service_Participant::publisher_content_filter() const
{
  return this->publisher_content_filter_;
}

ACE_INLINE
bool
Service_Participant::is_shut_down() const
{
  return this->shut_down_;
}

ACE_INLINE
bool
Service_Participant::use_bidir_giop() const
{
  return bidir_giop_;
}

ACE_INLINE
Service_Participant::TypeObjectEncoding
Service_Participant::type_object_encoding() const
{
  return type_object_encoding_;
}

ACE_INLINE
void Service_Participant::type_object_encoding(TypeObjectEncoding encoding)
{
  type_object_encoding_ = encoding;
}

} // namespace DDS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
