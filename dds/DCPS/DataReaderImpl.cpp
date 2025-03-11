/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "DataReaderImpl.h"

#include "DCPS_Utils.h"
#include "DomainParticipantImpl.h"
#include "FeatureDisabledQosCheck.h"
#include "GuidConverter.h"
#include "MonitorFactory.h"
#include "Qos_Helper.h"
#include "QueryConditionImpl.h"
#include "ReadConditionImpl.h"
#include "ReceivedDataElementList.h"
#include "SafetyProfileStreams.h"
#include "Serializer.h"
#include "Service_Participant.h"
#include "SubscriberImpl.h"
#include "SubscriptionInstance.h"
#include "TopicImpl.h"
#include "Transient_Kludge.h"
#include "TypeSupportImpl.h"
#include "Util.h"

#include "transport/framework/EntryExit.h"
#include "transport/framework/TransportExceptions.h"

#include "XTypes/TypeObject.h"

#include <dds/OpenDDSConfigWrapper.h>
#ifndef DDS_HAS_MINIMUM_BIT
#  include "BuiltInTopicUtils.h"
#endif

#ifndef DDS_HAS_MINIMUM_BIT
#  include <dds/DdsDcpsCoreTypeSupportC.h>
#endif
#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsGuidTypeSupportImpl.h>

#include <ace/Reactor.h>
#include <ace/OS_NS_sys_time.h>

#include <cstdio>
#include <stdexcept>
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
#  include <sstream>
#endif

#ifndef __ACE_INLINE__
#  include "DataReaderImpl.inl"
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DataReaderImpl::DataReaderImpl()
  : qos_(TheServiceParticipant->initial_DataReaderQos())
  , reverse_sample_lock_(sample_lock_)
  , topic_servant_(0)
  , type_support_(0)
  , topic_id_(GUID_UNKNOWN)
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  , is_exclusive_ownership_(false)
#endif
  , coherent_(false)
  , subqos_(TheServiceParticipant->initial_SubscriberQos())
  , topic_desc_(0)
  , listener_mask_(DEFAULT_STATUS_MASK)
  , domain_id_(0)
  , n_chunks_(TheServiceParticipant->n_chunks())
  , reactor_(0)
  , last_deadline_missed_total_count_(0)
  , deadline_queue_enabled_(false)
  , deadline_task_(make_rch<DRISporadicTask>(TheServiceParticipant->time_source(), TheServiceParticipant->reactor_task(), rchandle_from(this), &DataReaderImpl::deadline_task))
  , is_bit_(false)
  , always_get_history_(false)
  , statistics_enabled_(false)
  , raw_latency_buffer_size_(0)
  , raw_latency_buffer_type_(DataCollector<double>::KeepOldest)
  , transport_disabled_(false)
  , mb_alloc_(DEFAULT_TRANSPORT_RECEIVE_BUFFERS)
{
  reactor_ = TheServiceParticipant->timer();

  liveliness_changed_status_.alive_count = 0;
  liveliness_changed_status_.not_alive_count = 0;
  liveliness_changed_status_.alive_count_change = 0;
  liveliness_changed_status_.not_alive_count_change = 0;
  liveliness_changed_status_.last_publication_handle =
      DDS::HANDLE_NIL;

  requested_deadline_missed_status_.total_count = 0;
  requested_deadline_missed_status_.total_count_change = 0;
  requested_deadline_missed_status_.last_instance_handle =
      DDS::HANDLE_NIL;

  requested_incompatible_qos_status_.total_count = 0;
  requested_incompatible_qos_status_.total_count_change = 0;
  requested_incompatible_qos_status_.last_policy_id = 0;
  requested_incompatible_qos_status_.policies.length(0);

  subscription_match_status_.total_count = 0;
  subscription_match_status_.total_count_change = 0;
  subscription_match_status_.current_count = 0;
  subscription_match_status_.current_count_change = 0;
  subscription_match_status_.last_publication_handle =
      DDS::HANDLE_NIL;

  sample_lost_status_.total_count = 0;
  sample_lost_status_.total_count_change = 0;

  sample_rejected_status_.total_count = 0;
  sample_rejected_status_.total_count_change = 0;
  sample_rejected_status_.last_reason = DDS::NOT_REJECTED;
  sample_rejected_status_.last_instance_handle = DDS::HANDLE_NIL;

  this->budget_exceeded_status_.total_count = 0;
  this->budget_exceeded_status_.total_count_change = 0;
  this->budget_exceeded_status_.last_instance_handle = DDS::HANDLE_NIL;

  monitor_.reset(TheServiceParticipant->monitor_factory_->create_data_reader_monitor(this));
  periodic_monitor_.reset(TheServiceParticipant->monitor_factory_->create_data_reader_periodic_monitor(this));
}

// This method is called when there are no longer any reference to the
// the servant.
DataReaderImpl::~DataReaderImpl()
{
  DBG_ENTRY_LVL("DataReaderImpl", "~DataReaderImpl", 6);

  deadline_task_->cancel();

#ifndef OPENDDS_SAFETY_PROFILE
  RcHandle<DomainParticipantImpl> participant = participant_servant_.lock();
  if (participant) {
    XTypes::TypeLookupService_rch type_lookup_service = participant->get_type_lookup_service();
    if (type_lookup_service) {
      type_lookup_service->remove_guid_from_dynamic_map(subscription_id_);
    }
  }
#endif
}

// this method is called when delete_datareader is called.
void
DataReaderImpl::cleanup()
{
  // As first step set our listener to nill which will prevent us from calling
  // back onto the listener at the moment the related DDS entity has been
  // deleted
  set_listener(0, NO_STATUS_MASK);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  OwnershipManagerPtr owner_manager = this->ownership_manager();
  if (owner_manager) {
    owner_manager->unregister_reader(topic_servant_->type_name(), this);
  }
#endif

  // Acquire the sample lock since these pointers are read under it.
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

  topic_servant_ = 0;

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  {
    ACE_Guard<ACE_Thread_Mutex> guard_cft(content_filtered_topic_mutex_);
    content_filtered_topic_ = 0;
  }
#endif

#ifndef OPENDDS_NO_MULTI_TOPIC
  multi_topic_ = 0;
#endif

}

void DataReaderImpl::init(
    TopicDescriptionImpl* topic_desc,
    const DDS::DataReaderQos &qos,
    DDS::DataReaderListener_ptr listener,
    const DDS::StatusMask & mask,
    DomainParticipantImpl* participant,
    SubscriberImpl* subscriber)
{
  topic_desc_ = DDS::TopicDescription::_duplicate(topic_desc);
  if (TopicImpl* topic = dynamic_cast<TopicImpl*>(topic_desc)) {
    topic_servant_ = topic;
    type_support_ = dynamic_cast<TypeSupportImpl*>(topic->get_type_support());
    topic_id_ = topic->get_id();
  }

#ifndef DDS_HAS_MINIMUM_BIT
  CORBA::String_var topic_name = topic_desc->get_name();
  CORBA::String_var topic_type_name = topic_desc->get_type_name();
  is_bit_ = topicIsBIT(topic_name, topic_type_name);
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  qos_ = qos;
  passed_qos_ = qos;

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  is_exclusive_ownership_ = this->qos_.ownership.kind == ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
#endif

  set_listener(listener, mask);

  // Only store the participant pointer, since it is our "grand"
  // parent, we will exist as long as it does
  participant_servant_ = *participant;

  domain_id_ = participant->get_domain_id();

  subscriber_servant_ = rchandle_from(subscriber);

  if (subscriber->get_qos(this->subqos_) != ::DDS::RETCODE_OK) {
    ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::init() - ")
        ACE_TEXT("failed to get SubscriberQos\n")));
  }
}

DDS::InstanceHandle_t
DataReaderImpl::get_instance_handle()
{
  const RcHandle<DomainParticipantImpl> participant = participant_servant_.lock();
  return get_entity_instance_handle(subscription_id_, participant);
}

void
DataReaderImpl::set_subscription_id(const GUID_t& guid)
{
  OPENDDS_ASSERT(subscription_id_ == GUID_UNKNOWN);
  OPENDDS_ASSERT(guid != GUID_UNKNOWN);
  subscription_id_ = guid;
  TransportClient::set_guid(guid);
}

void
DataReaderImpl::add_association(const WriterAssociation& writer,
                                bool active)
{
  if (DCPS_debug_level) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderImpl::add_association - ")
        ACE_TEXT("bit %d local %C remote %C\n"), is_bit_,
        LogGuid(subscription_id_).c_str(),
        LogGuid(writer.writerId).c_str()));
  }

  if (get_deleted()) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderImpl::add_association")
          ACE_TEXT(" This is a deleted datareader, ignoring add.\n")));
    }
    return;
  }

  // For each writer in the list of writers to associate with, we
  // create a WriterInfo and a WriterStats object and store them in
  // our internal maps.
  //
  {
    ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, writers_lock_);

    const GUID_t& writer_id = writer.writerId;
    WriterInfo_rch info = make_rch<WriterInfo>(rchandle_from<WriterInfoListener>(this), writer_id, writer.writerQos, qos_.liveliness.lease_duration);
    std::pair<WriterMapType::iterator, bool> bpair = writers_.insert(
        // This insertion is idempotent.
        WriterMapType::value_type(
          writer_id,
          info));

    // Schedule timer if necessary
    //   - only need to check reader qos - we know the writer must be >= reader
    if (this->qos_.durability.kind > DDS::VOLATILE_DURABILITY_QOS) {
      info->waiting_for_end_historic_samples(true);
    }

    {
      ACE_Guard<ACE_Recursive_Thread_Mutex> guard(statistics_lock_);
      statistics_.insert(
        StatsMapType::value_type(
          writer_id,
          WriterStats(raw_latency_buffer_size_, raw_latency_buffer_type_)));
    }

    // If this is a durable reader
    if (this->qos_.durability.kind > DDS::VOLATILE_DURABILITY_QOS) {
      // TODO schedule timer for removing flag from writers
    }

    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
          "(%P|%t) DataReaderImpl::add_association: "
          "inserted writer %C.return %d\n",
          LogGuid(writer_id).c_str(), bpair.second));

      WriterMapType::iterator iter = writers_.find(writer_id);
      if (iter != writers_.end()) {
        // This may not be an error since it could happen that the sample
        // is delivered to the datareader after the write is dis-associated
        // with this datareader.
        ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DataReaderImpl::add_association: ")
            ACE_TEXT("reader %C is associated with writer %C.\n"),
            LogGuid(get_guid()).c_str(),
            LogGuid(writer_id).c_str()));
      }
    }
  }

  // Propagate the add_associations processing down into the Transport
  // layer here.  This will establish the transport support and reserve
  // usage of an existing connection or initiate creation of a new
  // connection if no suitable connection is available.
  AssociationData data;
  data.remote_id_ = writer.writerId;
  data.remote_data_ = writer.writerTransInfo;
  data.discovery_locator_ = writer.writerDiscInfo;
  data.participant_discovered_at_ = writer.participantDiscoveredAt;
  data.remote_transport_context_ = writer.transportContext;
  data.publication_transport_priority_ =
      writer.writerQos.transport_priority.value;
  data.remote_reliable_ =
      (writer.writerQos.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS);
  data.remote_durable_ =
      (writer.writerQos.durability.kind > DDS::VOLATILE_DURABILITY_QOS);

  if (associate(data, active)) {
    const Observer_rch observer = get_observer(Observer::e_ASSOCIATED);
    if (observer) {
      observer->on_associated(this, data.remote_id_);
    }
  } else {
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) DataReaderImpl::add_association: ")
                 ACE_TEXT("ERROR: transport layer failed to associate local reader %C remote writer %C\n"),
                 LogGuid(get_guid()).c_str(),
                 LogGuid(writer.writerId).c_str()));
    }
  }
}

void
DataReaderImpl::transport_assoc_done(int flags, const GUID_t& remote_id)
{
  if (!(flags & ASSOC_OK)) {
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) DataReaderImpl::transport_assoc_done: ")
                 ACE_TEXT("ERROR: transport layer failed to associate local reader %C remote writer %C\n"),
                 LogGuid(get_guid()).c_str(),
                 LogGuid(remote_id).c_str()));
    }
    return;
  }

  const RcHandle<DomainParticipantImpl> participant = participant_servant_.lock();

  if (!participant)
    return;

  const DDS::InstanceHandle_t handle = participant->assign_handle(remote_id);

  if (!is_bit_) {
    // We acquire the publication_handle_lock_ for the remainder of our
    // processing.
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, publication_handle_lock_);

      // This insertion is idempotent.
      publication_id_to_handle_map_.insert(RepoIdToHandleMap::value_type(remote_id, handle));

      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DataReaderImpl::transport_assoc_done: ")
            ACE_TEXT("id_to_handle_map_[ %C] = 0x%x.\n"),
            LogGuid(remote_id).c_str(),
            handle));
      }

      // We need to adjust these after the insertions have all completed
      // since insertions are not guaranteed to increase the number of
      // currently matched publications.
      const int matchedPublications = static_cast<int>(publication_id_to_handle_map_.size());
      subscription_match_status_.current_count_change =
          matchedPublications - subscription_match_status_.current_count;
      subscription_match_status_.current_count = matchedPublications;

      ++subscription_match_status_.total_count;
      ++subscription_match_status_.total_count_change;

      subscription_match_status_.last_publication_handle = handle;

      set_status_changed_flag(DDS::SUBSCRIPTION_MATCHED_STATUS, true);

      DDS::DataReaderListener_var listener =
          listener_for(DDS::SUBSCRIPTION_MATCHED_STATUS);

      if (!CORBA::is_nil(listener)) {
        listener->on_subscription_matched(this, subscription_match_status_);

        // TBD - why does the spec say to change this but not change
        //       the ChangeFlagStatus after a listener call?

        // Client will look at it so next time it looks the change should be 0
        subscription_match_status_.total_count_change = 0;
        subscription_match_status_.current_count_change = 0;
      }

      notify_status_condition();
    }

    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
      ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, writers_lock_);

      WriterMapType::iterator pos = writers_.find(remote_id);
      if (pos == writers_.end()) {
        return;
      }
      pos->second->handle(handle);
      pos->second->start_liveliness_timer();
    }
  }

  if (monitor_) {
    monitor_->report();
  }
}

void
DataReaderImpl::remove_associations(const WriterIdSeq& writers,
    bool notify_lost)
{
  DBG_ENTRY_LVL("DataReaderImpl", "remove_associations", 6);

  if (writers.length() == 0) {
    return;
  }

  const Observer_rch observer = get_observer(Observer::e_DISASSOCIATED);
  if (observer) {
    for (CORBA::ULong i = 0; i < writers.length(); ++i) {
      observer->on_disassociated(this, writers[i]);
    }
  }

  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations: ")
        ACE_TEXT("bit %d local %C remote %C num remotes %d\n"),
        is_bit_,
        LogGuid(get_guid()).c_str(),
        LogGuid(writers[0]).c_str(),
        writers.length()));
  }
  if (!get_deleted()) {
    // stop pending associations for these writer ids
    this->stop_associating(writers.get_buffer(), writers.length());

    {
      CORBA::ULong wr_len = writers.length();
      ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, this->writers_lock_);

      for (CORBA::ULong i = 0; i < wr_len; i++) {
        const GUID_t writer_id = writers[i];
        {
          ACE_Guard<ACE_Recursive_Thread_Mutex> guard(statistics_lock_);
          statistics_.erase(writer_id);
        }
      }
    }
  }

  remove_associations_i(writers, notify_lost);
}

void
DataReaderImpl::remove_associations_i(const WriterIdSeq& writers,
    bool notify_lost)
{
  DBG_ENTRY_LVL("DataReaderImpl", "remove_associations_i", 6);

  if (writers.length() == 0) {
    return;
  }

  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations_i: ")
        ACE_TEXT("bit %d local %C remote %C num remotes %d\n"),
        is_bit_,
        LogGuid(get_guid()).c_str(),
        LogGuid(writers[0]).c_str(),
        writers.length()));
  }
  DDS::InstanceHandleSeq handles;

  CORBA::ULong wr_len = writers.length();

  // Flush historic samples and/or allow in-progress delivery of historic samples to complete
  for (CORBA::ULong i = 0; i < wr_len; i++) {
    resume_sample_processing(writers[i]);
  }

  // This is used to hold the list of writers which were actually
  // removed, which is a proper subset of the writers which were
  // requested to be removed.
  WriterIdSeq updated_writers;
  WriterMapType removed_writers;

  //Remove the writers from writer list. If the supplied writer
  //is not in the cached writers list then it is already removed.
  //We just need remove the writers in the list that have not been
  //removed.
  {
    ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, this->writers_lock_);

    for (CORBA::ULong i = 0; i < wr_len; i++) {
      const GUID_t writer_id = writers[i];

      WriterMapType::iterator it = this->writers_.find(writer_id);

      if (it != this->writers_.end()) {
        removed_writers.insert(*it);
        it->second->cancel_historic_samples_timer();
      }

      if (this->writers_.erase(writer_id) == 0) {
        if (DCPS_debug_level >= 1) {
          ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations_i: ")
              ACE_TEXT("the writer local %C was already removed.\n"),
              LogGuid(writer_id).c_str()));
        }

      } else {
        push_back(updated_writers, writer_id);
      }
    }
  }

  for (WriterMapType::iterator it = removed_writers.begin(); it != removed_writers.end(); ++it) {
    it->second->removed();
  }
  removed_writers.clear();

  wr_len = updated_writers.length();

  // Return now if the supplied writers have been removed already.
  if (wr_len == 0) {
    return;
  }

  if (!is_bit_) {
    // The writer should be in the id_to_handle map at this time.
    this->lookup_instance_handles(updated_writers, handles);

    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(publication_handle_lock_);

    for (CORBA::ULong i = 0; i < wr_len; ++i) {
      publication_id_to_handle_map_.erase(updated_writers[i]);
    }
  }

  for (CORBA::ULong i = 0; i < updated_writers.length(); ++i) {
    {
      this->disassociate(updated_writers[i]);
    }
  }

  // Mirror the add_associations SUBSCRIPTION_MATCHED_STATUS processing.
  if (!this->is_bit_) {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publication_handle_lock_);

    // Derive the change in the number of publications writing to this reader.
    int matchedPublications = static_cast<int>(this->publication_id_to_handle_map_.size());
    this->subscription_match_status_.current_count_change
    = matchedPublications - this->subscription_match_status_.current_count;

    // Only process status if the number of publications has changed.
    if (this->subscription_match_status_.current_count_change != 0) {
      this->subscription_match_status_.current_count = matchedPublications;

      /// Section 7.1.4.1: total_count will not decrement.

      /// @TODO: Reconcile this with the verbiage in section 7.1.4.1
      this->subscription_match_status_.last_publication_handle
      = handles[ wr_len - 1];

      set_status_changed_flag(DDS::SUBSCRIPTION_MATCHED_STATUS, true);

      DDS::DataReaderListener_var listener
      = listener_for(DDS::SUBSCRIPTION_MATCHED_STATUS);

      if (!CORBA::is_nil(listener.in())) {
        listener->on_subscription_matched(this, this->subscription_match_status_);

        // Client will look at it so next time it looks the change should be 0
        this->subscription_match_status_.total_count_change = 0;
        this->subscription_match_status_.current_count_change = 0;
      }
      notify_status_condition();
    }
  }

  // If this remove_association is invoked when the InfoRepo
  // detects a lost writer then make a callback to notify
  // subscription lost.
  if (notify_lost) {
    this->notify_subscription_lost(handles);
  }

  if (this->monitor_) {
    this->monitor_->report();
  }
}

void
DataReaderImpl::remove_all_associations()
{
  DBG_ENTRY_LVL("DataReaderImpl","remove_all_associations",6);
  stop_associating();

  OpenDDS::DCPS::WriterIdSeq writers;
  int size;

  {
    ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);

    size = static_cast<int>(writers_.size());
    writers.length(size);

    WriterMapType::iterator curr_writer = writers_.begin();
    WriterMapType::iterator end_writer = writers_.end();

    int i = 0;

    while (curr_writer != end_writer) {
      writers[i++] = curr_writer->first;
      ++curr_writer;
    }
  }

  try {
    if (0 < size) {
      remove_associations(writers, false);
    }
  } catch (const CORBA::Exception&) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::remove_all_associations() - ")
               ACE_TEXT("caught exception from remove_associations.\n")));
  }

  if (!transport_disabled_) {
    transport_stop();
  }
}

void
DataReaderImpl::update_incompatible_qos(const IncompatibleQosStatus& status)
{
  DDS::DataReaderListener_var listener =
      listener_for(DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS);

  if (this->requested_incompatible_qos_status_.total_count == status.total_count) {
    // This test should make the method idempotent.
    return;
  }

  set_status_changed_flag(DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS,
      true);

  // copy status and increment change
  requested_incompatible_qos_status_.total_count = status.total_count;
  requested_incompatible_qos_status_.total_count_change +=
      status.count_since_last_send;
  requested_incompatible_qos_status_.last_policy_id =
      status.last_policy_id;
  requested_incompatible_qos_status_.policies = status.policies;

  if (!CORBA::is_nil(listener.in())) {
    listener->on_requested_incompatible_qos(this, requested_incompatible_qos_status_);

    // TBD - why does the spec say to change total_count_change but not
    // change the ChangeFlagStatus after a listener call?

    // client just looked at it so next time it looks the
    // change should be 0
    requested_incompatible_qos_status_.total_count_change = 0;
  }

  notify_status_condition();
}

void
DataReaderImpl::signal_liveliness(const GUID_t& remote_participant)
{
  GUID_t prefix = remote_participant;
  prefix.entityId = EntityId_t();

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

  typedef std::pair<GUID_t, WriterInfo_rch> RepoWriterPair;
  typedef OPENDDS_VECTOR(RepoWriterPair) WriterSet;
  WriterSet writers;

  {
    ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);
    for (WriterMapType::iterator pos = writers_.lower_bound(prefix),
           limit = writers_.end();
         pos != limit && equal_guid_prefixes(pos->first, prefix);
         ++pos) {
      writers.push_back(std::make_pair(pos->first, pos->second));
    }
  }

  const MonotonicTimePoint when = MonotonicTimePoint::now();
  for (WriterSet::iterator pos = writers.begin(), limit = writers.end();
       pos != limit;
       ++pos) {
    pos->second->received_activity(when);
  }

  if (!writers.empty()) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_);
    for (WriterSet::iterator pos = writers.begin(), limit = writers.end();
         pos != limit;
         ++pos) {
      for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
           iter != instances_.end();
           ++iter) {
        SubscriptionInstance_rch ptr = iter->second;
        ptr->instance_state_->lively(pos->first);
      }
    }
  }
}

DDS::ReadCondition_ptr DataReaderImpl::create_readcondition(
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_, 0);
  DDS::ReadCondition_var rc = new ReadConditionImpl(this, sample_states,
      view_states, instance_states);
  read_conditions_.insert(rc);
  return rc._retn();
}

#ifndef OPENDDS_NO_QUERY_CONDITION
DDS::QueryCondition_ptr DataReaderImpl::create_querycondition(
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states,
    const char* query_expression,
    const DDS::StringSeq& query_parameters)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_, 0);
  try {
    DDS::QueryCondition_var qc = new QueryConditionImpl(this, sample_states,
        view_states, instance_states, query_expression);
    if (qc->set_query_parameters(query_parameters) != DDS::RETCODE_OK) {
      return 0;
    }
    DDS::ReadCondition_var rc = DDS::ReadCondition::_duplicate(qc);
    read_conditions_.insert(rc);
    return qc._retn();
  } catch (const std::exception& e) {
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ")
          ACE_TEXT("DataReaderImpl::create_querycondition - %C\n"),
          e.what()));
    }
  }
  return 0;
}
#endif

bool DataReaderImpl::has_readcondition(DDS::ReadCondition_ptr a_condition)
{
  //sample lock already held
  DDS::ReadCondition_var rc = DDS::ReadCondition::_duplicate(a_condition);
  return read_conditions_.find(rc) != read_conditions_.end();
}

DDS::ReturnCode_t DataReaderImpl::delete_readcondition(
    DDS::ReadCondition_ptr a_condition)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
      DDS::RETCODE_OUT_OF_RESOURCES);
  DDS::ReadCondition_var rc = DDS::ReadCondition::_duplicate(a_condition);
  return read_conditions_.erase(rc)
      ? DDS::RETCODE_OK : DDS::RETCODE_PRECONDITION_NOT_MET;
}

DDS::ReturnCode_t DataReaderImpl::delete_contained_entities()
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
      DDS::RETCODE_OUT_OF_RESOURCES);
  read_conditions_.clear();
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DataReaderImpl::set_qos(const DDS::DataReaderQos& qos)
{
  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);

  DDS::DataReaderQos new_qos = qos;
  new_qos.representation.value = qos_.representation.value;
  if (Qos_Helper::valid(new_qos) && Qos_Helper::consistent(new_qos)) {

    if (qos_ == new_qos)
      return DDS::RETCODE_OK;

    if (enabled_) {
      if (!Qos_Helper::changeable(qos_, new_qos)) {
        return DDS::RETCODE_IMMUTABLE_POLICY;

      } else {
        Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
        DDS::SubscriberQos subscriberQos;

        RcHandle<SubscriberImpl> subscriber = get_subscriber_servant();
        bool status = false;
        if (subscriber) {
          subscriber->get_qos(subscriberQos);
          status =
            disco->update_subscription_qos(
              domain_id_,
              dp_id_,
              subscription_id_,
              new_qos,
              subscriberQos);
        }
        if (!status) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DataReaderImpl::set_qos, ")
                            ACE_TEXT("qos not updated.\n")),
                            DDS::RETCODE_ERROR);
        }
      }
    }

    qos_change(new_qos);
    qos_ = new_qos;
    passed_qos_ = qos;

    const Observer_rch observer = get_observer(Observer::e_QOS_CHANGED);
    if (observer) {
      observer->on_qos_changed(this);
    }

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

void DataReaderImpl::qos_change(const DDS::DataReaderQos & qos)
{
  // Reset the deadline timer if the period has changed.
  if (qos_.deadline.period.sec != qos.deadline.period.sec ||
      qos_.deadline.period.nanosec != qos.deadline.period.nanosec) {
    if (qos_.deadline.period.sec == DDS::DURATION_INFINITE_SEC &&
        qos_.deadline.period.nanosec == DDS::DURATION_INFINITE_NSEC) {
      deadline_period_ = TimeDuration(qos.deadline.period);
      deadline_queue_enabled_ = true;
    } else if (qos.deadline.period.sec == DDS::DURATION_INFINITE_SEC &&
               qos.deadline.period.nanosec == DDS::DURATION_INFINITE_NSEC) {
      cancel_all_deadlines();
      deadline_queue_enabled_ = false;
    } else {
      reset_deadline_period(TimeDuration(qos.deadline.period));
    }
  }
}

DDS::ReturnCode_t
DataReaderImpl::get_qos(
    DDS::DataReaderQos & qos)
{
  qos = passed_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DataReaderImpl::set_listener(
    DDS::DataReaderListener_ptr a_listener,
    DDS::StatusMask mask)
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = DDS::DataReaderListener::_duplicate(a_listener);
  return DDS::RETCODE_OK;
}

DDS::DataReaderListener_ptr DataReaderImpl::get_listener()
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  return DDS::DataReaderListener::_duplicate(listener_.in());
}

DataReaderListener_ptr DataReaderImpl::get_ext_listener()
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  return DataReaderListener::_narrow(listener_.in());
}

DDS::TopicDescription_ptr DataReaderImpl::get_topicdescription()
{
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  {
    ACE_Guard<ACE_Thread_Mutex> guard(content_filtered_topic_mutex_);
    if (content_filtered_topic_) {
      return DDS::TopicDescription::_duplicate(content_filtered_topic_.get());
    }
  }
#endif
  return DDS::TopicDescription::_duplicate(topic_desc_.in());
}

DDS::Subscriber_ptr DataReaderImpl::get_subscriber()
{
  return get_subscriber_servant()._retn();
}

DDS::ReturnCode_t
DataReaderImpl::get_sample_rejected_status(
    DDS::SampleRejectedStatus & status)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(this->sample_lock_);

  set_status_changed_flag(DDS::SAMPLE_REJECTED_STATUS, false);
  status = sample_rejected_status_;
  sample_rejected_status_.total_count_change = 0;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::get_liveliness_changed_status(
    DDS::LivelinessChangedStatus & status)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(this->sample_lock_);

  set_status_changed_flag(DDS::LIVELINESS_CHANGED_STATUS,
      false);
  status = liveliness_changed_status_;

  liveliness_changed_status_.alive_count_change = 0;
  liveliness_changed_status_.not_alive_count_change = 0;

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::get_requested_deadline_missed_status(
    DDS::RequestedDeadlineMissedStatus & status)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(this->sample_lock_);

  set_status_changed_flag(DDS::REQUESTED_DEADLINE_MISSED_STATUS,
      false);

  this->requested_deadline_missed_status_.total_count_change =
      this->requested_deadline_missed_status_.total_count
      - this->last_deadline_missed_total_count_;

  // DDS::RequestedDeadlineMissedStatus::last_instance_handle field
  // is updated by the RequestedDeadlineWatchdog.

  // Update for next status check.
  this->last_deadline_missed_total_count_ =
      this->requested_deadline_missed_status_.total_count;

  status = requested_deadline_missed_status_;

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::get_requested_incompatible_qos_status(
    DDS::RequestedIncompatibleQosStatus & status)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publication_handle_lock_);

  set_status_changed_flag(DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS, false);
  status = requested_incompatible_qos_status_;
  requested_incompatible_qos_status_.total_count_change = 0;

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::get_subscription_matched_status(
    DDS::SubscriptionMatchedStatus & status)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publication_handle_lock_);

  set_status_changed_flag(DDS::SUBSCRIPTION_MATCHED_STATUS, false);
  status = subscription_match_status_;
  subscription_match_status_.total_count_change = 0;
  subscription_match_status_.current_count_change = 0;

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::get_sample_lost_status(
    DDS::SampleLostStatus & status)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(this->sample_lock_);

  set_status_changed_flag(DDS::SAMPLE_LOST_STATUS, false);
  status = sample_lost_status_;
  sample_lost_status_.total_count_change = 0;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::wait_for_historical_data(
    const DDS::Duration_t & /* max_wait */)
{
  // Add your implementation here
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::get_matched_publications(
    DDS::InstanceHandleSeq & publication_handles)
{
  if (!enabled_) {
    ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::get_matched_publications: ")
        ACE_TEXT(" Entity is not enabled.\n")),
        DDS::RETCODE_NOT_ENABLED);
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      guard,
      publication_handle_lock_,
      DDS::RETCODE_ERROR);

  // Copy out the handles for the current set of publications.
  int index = 0;
  publication_handles.length(static_cast<CORBA::ULong>(this->publication_id_to_handle_map_.size()));

  for (RepoIdToHandleMap::iterator
      current = this->publication_id_to_handle_map_.begin();
      current != this->publication_id_to_handle_map_.end();
      ++current, ++index) {
    publication_handles[index] = current->second;
  }

  return DDS::RETCODE_OK;
}

#if !defined (DDS_HAS_MINIMUM_BIT)
DDS::ReturnCode_t
DataReaderImpl::get_matched_publication_data(
    DDS::PublicationBuiltinTopicData & publication_data,
    DDS::InstanceHandle_t publication_handle)
{
  if (!enabled_) {
    ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::")
        ACE_TEXT("get_matched_publication_data: ")
        ACE_TEXT("Entity is not enabled.\n")),
        DDS::RETCODE_NOT_ENABLED);
  }

  RcHandle<DomainParticipantImpl> participant = this->participant_servant_.lock();

  if (!participant)
    return DDS::RETCODE_ERROR;

  DDS::PublicationBuiltinTopicDataSeq data;
  const DDS::ReturnCode_t ret = instance_handle_to_bit_data<DDS::PublicationBuiltinTopicDataDataReader_var>(
                                  participant.in(),
                                  BUILT_IN_PUBLICATION_TOPIC,
                                  publication_handle,
                                  data);

  if (ret == DDS::RETCODE_OK) {
    publication_data = data[0];
  }

  return ret;
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)

DDS::ReturnCode_t
DataReaderImpl::enable()
{
  // According to spec:
  // - Calling enable on an already enabled Entity has no effect and returns OK.
  // - Calling enable on an Entity whose factory is not enabled will fail
  //   and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  RcHandle<SubscriberImpl> subscriber = get_subscriber_servant();
  if (!subscriber) {
    return DDS::RETCODE_ERROR;
  }

  if (!subscriber->is_enabled()) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  if (topic_servant_ && !topic_servant_->is_enabled()) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  RcHandle<DomainParticipantImpl> participant = participant_servant_.lock();
  if (participant) {
    dp_id_ = participant->get_id();
  }

  if (topic_servant_) {
    set_reader_effective_data_rep_qos(qos_.representation.value);
    if (!topic_servant_->check_data_representation(qos_.representation.value, false)) {
      return DDS::RETCODE_ERROR;
    }
  }

  if (qos_.history.kind == DDS::KEEP_ALL_HISTORY_QOS) {
    // The spec says qos_.history.depth is "has no effect"
    // when history.kind = KEEP_ALL so use max_samples_per_instance
    depth_ = qos_.resource_limits.max_samples_per_instance;

  } else { // qos_.history.kind == DDS::KEEP_LAST_HISTORY_QOS
    depth_ = qos_.history.depth;
  }

  if (depth_ == DDS::LENGTH_UNLIMITED) {
    // DDS::LENGTH_UNLIMITED is negative so make it a positive
    // value that is, for all intents and purposes, unlimited
    // and we can use it for comparisons.
    // WARNING: The client risks running out of memory in this case.
    depth_ = ACE_INT32_MAX;
  }

  if (qos_.resource_limits.max_samples != DDS::LENGTH_UNLIMITED) {
    n_chunks_ = qos_.resource_limits.max_samples;
  }

  //else using value from Service_Participant

  // enable the type specific part of this DataReader
  this->enable_specific();

  //Note: the QoS used to set n_chunks_ is Changeable=No so
  // it is OK that we cannot change the size of our allocators.
  rd_allocator_.reset(new ReceivedDataAllocator(n_chunks_));

  if (DCPS_debug_level >= 2)
    ACE_DEBUG((LM_DEBUG,"(%P|%t) DataReaderImpl::enable"
        " Cached_Allocator_With_Overflow %x with %d chunks\n",
        rd_allocator_.get(), n_chunks_));

  // Setup the requested deadline watchdog if the configured deadline
  // period is not the default (infinite).
  DDS::Duration_t const deadline_period = this->qos_.deadline.period;

  if (!deadline_queue_enabled_
      && (deadline_period.sec != DDS::DURATION_INFINITE_SEC
          || deadline_period.nanosec != DDS::DURATION_INFINITE_NSEC)) {
    deadline_period_ = TimeDuration(qos_.deadline.period);
    deadline_queue_enabled_ = true;
  }

  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
  disco->pre_reader(this);

  this->set_enabled();

  if (topic_servant_ && !transport_disabled_) {
    try {
      this->enable_transport(this->qos_.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS,
                             this->qos_.durability.kind > DDS::VOLATILE_DURABILITY_QOS, participant.get());
    } catch (const Transport::Exception&) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::enable, ")
          ACE_TEXT("Transport Exception.\n")));
      return DDS::RETCODE_ERROR;
    }

    const DDS::ReturnCode_t setup_deserialization_result = setup_deserialization();
    if (setup_deserialization_result != DDS::RETCODE_OK) {
      return setup_deserialization_result;
    }

    const TransportLocatorSeq& trans_conf_info = connection_info();

    CORBA::String_var filterClassName = "";
    CORBA::String_var filterExpression = "";
    DDS::StringSeq exprParams;
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    {
      ACE_Guard<ACE_Thread_Mutex> guard(content_filtered_topic_mutex_);
      if (content_filtered_topic_) {
        filterClassName = content_filtered_topic_->get_filter_class_name();
        filterExpression = content_filtered_topic_->get_filter_expression();
        content_filtered_topic_->get_expression_parameters(exprParams);
      }
    }
#endif

    DDS::SubscriberQos sub_qos;
    subscriber->get_qos(sub_qos);

    TypeSupportImpl* const typesupport =
      dynamic_cast<TypeSupportImpl*>(topic_servant_->get_type_support());
    if (!typesupport) {
      return DDS::RETCODE_ERROR;
    }

    TypeInformation type_info;
    typesupport->to_type_info(type_info);

    XTypes::TypeLookupService_rch type_lookup_service = participant->get_type_lookup_service();
    typesupport->add_types(type_lookup_service);

    install_type_support(typesupport);

    const bool success =
      disco->add_subscription(domain_id_,
                              dp_id_,
                              topic_servant_->get_id(),
                              rchandle_from(this),
                              qos_,
                              trans_conf_info,
                              sub_qos,
                              filterClassName,
                              filterExpression,
                              exprParams,
                              type_info);

#if OPENDDS_CONFIG_SECURITY
    {
      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, DDS::RETCODE_ERROR);
      security_config_ = participant->get_security_config();
      dynamic_type_ = typesupport->get_type();
    }
#endif

    if (!success || subscription_id_ == GUID_UNKNOWN) {
      if (DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: DataReaderImpl::enable: "
          "add_subscription failed\n"));
      }
      return DDS::RETCODE_ERROR;
    }

    if (DCPS_debug_level >= 2) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReaderImpl::enable: "
        "got GUID %C, subscribed to topic name \"%C\" type \"%C\"\n",
        LogGuid(get_guid()).c_str(),
        topic_servant_->topic_name(), topic_servant_->type_name()));
    }
  }

  DDS::ReturnCode_t return_value = DDS::RETCODE_OK;
  if (topic_servant_) {
    const CORBA::String_var name = topic_servant_->get_name();
    return_value = subscriber->reader_enabled(name.in(), this);

    if (this->monitor_) {
      this->monitor_->report();
    }
  }

  if (return_value == DDS::RETCODE_OK) {
    const Observer_rch observer = get_observer(Observer::e_ENABLED);
    if (observer) {
      observer->on_enabled(this);
    }
  }

  return return_value;
}

void
DataReaderImpl::writer_activity(const DataSampleHeader& header)
{
  // caller should have the sample_lock_ !!!

  WriterInfo_rch writer;

  // The received_activity() has to be called outside the writers_lock_
  // because it probably acquire writers_lock_ read lock recursively
  // (in handle_timeout). This could cause deadlock when there are writers
  // waiting.
  {
    ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);
    WriterMapType::iterator iter = writers_.find(header.publication_id_);

    if (iter != writers_.end()) {
      writer = iter->second;

    } else if (DCPS_debug_level > 4) {
      // This may not be an error since it could happen that the sample
      // is delivered to the datareader after the write is dis-associated
      // with this datareader.
      ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::writer_activity: ")
          ACE_TEXT("reader %C is not associated with writer %C.\n"),
          LogGuid(get_guid()).c_str(),
          LogGuid(header.publication_id_).c_str()));
    }
  }

  if (!writer.is_nil()) {
    writer->received_activity(MonotonicTimePoint::now());

    if ((header.message_id_ == SAMPLE_DATA) ||
        (header.message_id_ == INSTANCE_REGISTRATION) ||
        (header.message_id_ == UNREGISTER_INSTANCE) ||
        (header.message_id_ == DISPOSE_INSTANCE) ||
        (header.message_id_ == DISPOSE_UNREGISTER_INSTANCE)) {

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
      if (header.coherent_change_) {
        writer->add_coherent_samples(header.sequence_);
      }
#endif
    }
  }
}

void
DataReaderImpl::data_received(const ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("DataReaderImpl","data_received",6);

  DDS::InstanceHandle_t publication_handle = DDS::HANDLE_NIL;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, publication_handle_lock_);
    RepoIdToHandleMap::const_iterator pos = publication_id_to_handle_map_.find(sample.header_.publication_id_);
    if (pos != publication_id_to_handle_map_.end()) {
      publication_handle = pos->second;
    }
  }

  // ensure some other thread is not changing the sample container
  // or statuses related to samples.
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

  if (get_deleted()) return;

  if (DCPS_debug_level > 9) {
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderImpl::data_received: ")
        ACE_TEXT("%C received sample: %C.\n"),
        LogGuid(get_guid()).c_str(),
        to_string(sample.header_).c_str()));
  }

  switch (sample.header_.message_id_) {
  case SAMPLE_DATA:
  case INSTANCE_REGISTRATION: {
    SubscriptionInstance_rch instance;
    if (!check_historic(sample)) break;

    DataSampleHeader const & header = sample.header_;

    this->writer_activity(header);

    // Verify data has not exceeded its lifespan.
    if (this->filter_sample(header)) break;

    // This adds the reader to the set/list of readers with data.
    RcHandle<SubscriberImpl> subscriber = get_subscriber_servant();
    if (subscriber) {
      subscriber->data_received(this);
    }

    // Only gather statistics about real samples, not registration data, etc.
    if (header.message_id_ == SAMPLE_DATA) {
      this->process_latency(sample);
    }

    // This also adds to the sample container and makes any callbacks
    // and condition modifications.

    bool is_new_instance = false;
    bool filtered = false;
    dds_demarshal(sample, publication_handle, instance, is_new_instance, filtered,
                  sample.header_.key_fields_only_ ? KEY_ONLY_MARSHALING : FULL_MARSHALING);

    // Per sample logging
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::data_received: reader %C writer %C ")
          ACE_TEXT("instance %d is_new_instance %d filtered %d\n"),
          LogGuid(get_guid()).c_str(),
          LogGuid(header.publication_id_).c_str(),
          instance ? instance->instance_handle_ : 0,
          is_new_instance, filtered));
    }

    if (filtered) break; // sample filtered from instance

    if (instance) accept_sample_processing(instance, header, is_new_instance);
  }
  break;

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  case END_COHERENT_CHANGES: {
    CoherentChangeControl control;

    this->writer_activity(sample.header_);

    Message_Block_Ptr payload(sample.data(&mb_alloc_));
    Serializer serializer(
        payload.get(), Encoding::KIND_UNALIGNED_CDR,
        sample.header_.byte_order_ ? ENDIAN_LITTLE : ENDIAN_BIG);
    if (!(serializer >> control)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DataReaderImpl::data_received ")
          ACE_TEXT("deserialization coherent change control failed.\n")));
      return;
    }

    if (DCPS_debug_level > 0) {
      std::stringstream buffer;
      buffer << control << std::endl;

      ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::data_received: ")
          ACE_TEXT("END_COHERENT_CHANGES %C\n"),
          buffer.str().c_str()));
    }

    WriterInfo_rch writer;
    {
      ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);

      WriterMapType::iterator it =
          this->writers_.find(sample.header_.publication_id_);

      if (it == this->writers_.end()) {
        ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::data_received() - ")
            ACE_TEXT(" subscription %C failed to find ")
            ACE_TEXT(" publication data for %C!\n"),
            LogGuid(get_guid()).c_str(),
            LogGuid(sample.header_.publication_id_).c_str()));
        return;
      }
      else {
        writer = it->second;
      }
      it->second->set_group_info(control);
    }

    if (this->verify_coherent_changes_completion(writer.in())) {
      this->notify_read_conditions();
    }
  }
  break;
#endif // OPENDDS_NO_OBJECT_MODEL_PROFILE

  case DATAWRITER_LIVELINESS: {
    if (DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataReaderImpl::data_received: ")
                 ACE_TEXT("reader %C got datawriter liveliness from writer %C\n"),
                 LogGuid(get_guid()).c_str(),
                 LogGuid(sample.header_.publication_id_).c_str()));
    }
    this->writer_activity(sample.header_);

    // tell all instances they got a liveliness message
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
      for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
          iter != instances_.end();
          ++iter) {
        if (iter->second->instance_state_->writes_instance(sample.header_.publication_id_)) {
          iter->second->instance_state_->lively(sample.header_.publication_id_);
        }
      }
    }

  }
  break;

  case DISPOSE_INSTANCE: {
    if (!check_historic(sample)) break;
    this->writer_activity(sample.header_);
    SubscriptionInstance_rch instance;

    if (deadline_queue_enabled_) {
      // Find the instance first for timer cancellation since
      // the instance may be deleted during dispose and can
      // not be accessed.
      ReceivedDataSample dup(sample);
      this->lookup_instance(dup, instance);
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      OwnershipManagerPtr owner_manager = this->ownership_manager();

      if (! this->is_exclusive_ownership_
          || (owner_manager
              && (instance)
              && (owner_manager->is_owner(instance->instance_handle_,
                  sample.header_.publication_id_)))) {
#endif
        cancel_deadline(instance);
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      }
#endif
    }
    instance.reset();
    this->dispose_unregister(sample, publication_handle, instance);
  }
  this->notify_read_conditions();
  break;

  case UNREGISTER_INSTANCE: {
    if (!check_historic(sample)) break;
    this->writer_activity(sample.header_);
    SubscriptionInstance_rch instance;

    if (deadline_queue_enabled_) {
      // Find the instance first for timer cancellation since
      // the instance may be deleted during dispose and can
      // not be accessed.
      ReceivedDataSample dup(sample);
      this->lookup_instance(dup, instance);
      if (instance) {
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
        if (! this->is_exclusive_ownership_
            || (this->is_exclusive_ownership_
                && instance->instance_state_->is_last(sample.header_.publication_id_))) {
#endif
          cancel_deadline(instance);
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
        }
#endif
      }
    }
    instance.reset();
    this->dispose_unregister(sample, publication_handle, instance);
  }
  this->notify_read_conditions();
  break;

  case DISPOSE_UNREGISTER_INSTANCE: {
    if (!check_historic(sample)) break;
    this->writer_activity(sample.header_);
    SubscriptionInstance_rch instance;

    if (deadline_queue_enabled_) {
      // Find the instance first for timer cancellation since
      // the instance may be deleted during dispose and can
      // not be accessed.
      ReceivedDataSample dup(sample);
      this->lookup_instance(dup, instance);
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      OwnershipManagerPtr owner_manager = this->ownership_manager();
      if (! this->is_exclusive_ownership_
          || (owner_manager
              && (instance)
              && (owner_manager->is_owner (instance->instance_handle_,
                  sample.header_.publication_id_)))
          || (is_exclusive_ownership_
              && (instance)
              && instance->instance_state_->is_last(sample.header_.publication_id_))) {
#endif
        if (instance) {
          cancel_deadline(instance);
        }
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      }
#endif
    }
    instance.reset();
    this->dispose_unregister(sample, publication_handle, instance);
  }
  this->notify_read_conditions();
  break;

  case END_HISTORIC_SAMPLES: {
    if (sample.header_.message_length_ >= sizeof(GUID_t)) {
      Message_Block_Ptr payload(sample.data(&mb_alloc_));
      Serializer ser(payload.get(), Encoding::KIND_UNALIGNED_CDR);
      GUID_t readerId = GUID_UNKNOWN;
      if (!(ser >> readerId)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DataReaderImpl::data_received ")
            ACE_TEXT("deserialization reader failed.\n")));
        return;
      }
      const GUID_t repo_id(get_guid());
      if (readerId != GUID_UNKNOWN && readerId != repo_id) {
        break; // not our message
      }
    }
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_INFO, "(%P|%t) Received END_HISTORIC_SAMPLES control message\n"));
    }
    // Going to acquire writers lock, release samples lock
    guard.release();
    resume_sample_processing(sample.header_.publication_id_);
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((
          LM_INFO,
          "(%P|%t) Resumed sample processing for durable writer %C\n",
          LogGuid(sample.header_.publication_id_).c_str()));
    }
    break;
  }

  default:
    ACE_ERROR((LM_ERROR,
        "(%P|%t) ERROR: DataReaderImpl::data_received"
        "unexpected message_id = %d\n",
        sample.header_.message_id_));
    break;
  }
}

RcHandle<EntityImpl>
DataReaderImpl::parent() const
{
  return subscriber_servant_.lock();
}

bool
DataReaderImpl::check_transport_qos(const TransportInst& ti)
{
  if (this->qos_.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS) {
    return ti.is_reliable();
  }
  return true;
}

void DataReaderImpl::notify_read_conditions()
{
  //sample lock is already held
  ReadConditionSet local_read_conditions = read_conditions_;
  ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_sample_lock_);

  for (ReadConditionSet::iterator it = local_read_conditions.begin(),
      end = local_read_conditions.end(); it != end; ++it) {
    ConditionImpl* ci = dynamic_cast<ConditionImpl*>(it->in());
    if (ci) {
      ci->signal_all();
    } else {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::notify_read_conditions: ")
        ACE_TEXT("Failed to obtain ConditionImpl - can't notify.\n")));
    }
  }
}

RcHandle<SubscriberImpl>
DataReaderImpl::get_subscriber_servant()
{
  return subscriber_servant_.lock();
}

bool DataReaderImpl::have_sample_states(
    DDS::SampleStateMask sample_states) const
{
  //!!!caller should have acquired sample_lock_
  ACE_Guard<ACE_Recursive_Thread_Mutex> instance_guard(instances_lock_);
  return lookup_matching_instances(sample_states, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE).size();
}

bool
DataReaderImpl::have_view_states(DDS::ViewStateMask view_states) const
{
  //!!!caller should have acquired sample_lock_
  ACE_Guard<ACE_Recursive_Thread_Mutex> instance_guard(instances_lock_);
  return lookup_matching_instances(DDS::ANY_SAMPLE_STATE, view_states, DDS::ANY_INSTANCE_STATE).size();
}

bool DataReaderImpl::have_instance_states(
    DDS::InstanceStateMask instance_states) const
{
  //!!!caller should have acquired sample_lock_
  ACE_Guard<ACE_Recursive_Thread_Mutex> instance_guard(instances_lock_);
  return lookup_matching_instances(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, instance_states).size();
}

/// Fold-in the three separate loops of have_sample_states(),
/// have_view_states(), and have_instance_states().  Takes the sample_lock_.
bool DataReaderImpl::contains_sample(DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> sample_guard(sample_lock_);
  ACE_Guard<ACE_Recursive_Thread_Mutex> instance_guard(instances_lock_);

  return lookup_matching_instances(sample_states, view_states, instance_states).size();
}

DDS::DataReaderListener_ptr
DataReaderImpl::listener_for(DDS::StatusKind kind)
{
  // per 2.1.4.3.1 Listener Access to Plain Communication Status
  // use this entities factory if listener is mask not enabled
  // for this kind.
  RcHandle<SubscriberImpl> subscriber = get_subscriber_servant();
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  if (subscriber && (CORBA::is_nil(listener_.in()) || (listener_mask_ & kind) == 0)) {
    g.release();
    return subscriber->listener_for(kind);

  } else {
    return DDS::DataReaderListener::_duplicate(listener_.in());
  }
}

void DataReaderImpl::sample_info(DDS::SampleInfo & sample_info,
    const ReceivedDataElement *ptr)
{

  sample_info.sample_rank = 0;

  // generation_rank =
  //    (MRSIC.disposed_generation_count +
  //     MRSIC.no_writers_generation_count)
  //  - (S.disposed_generation_count +
  //     S.no_writers_generation_count)
  //
  sample_info.generation_rank =
      (sample_info.disposed_generation_count +
          sample_info.no_writers_generation_count) -
          sample_info.generation_rank;

  // absolute_generation_rank =
  //     (MRS.disposed_generation_count +
  //      MRS.no_writers_generation_count)
  //   - (S.disposed_generation_count +
  //      S.no_writers_generation_count)
  //
  sample_info.absolute_generation_rank =
      (static_cast<CORBA::Long>(ptr->disposed_generation_count_) +
          static_cast<CORBA::Long>(ptr->no_writers_generation_count_)) -
          sample_info.absolute_generation_rank;

  sample_info.opendds_reserved_publication_seq = ptr->sequence_.getValue();
}

CORBA::Long DataReaderImpl::total_samples() const
{
  //!!!caller should have acquired sample_lock_
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_,0);

  CORBA::Long count(0);

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
      iter != instances_.end();
      ++iter) {
    SubscriptionInstance_rch ptr = iter->second;

    count += static_cast<CORBA::Long>(ptr->rcvd_samples_.size());
  }

  return count;
}

void
DataReaderImpl::release_instance(DDS::InstanceHandle_t handle)
{
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  OwnershipManagerPtr owner_manager = this->ownership_manager();
  if (owner_manager) {
    owner_manager->remove_writers(handle);
  }
#endif

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);
  SubscriptionInstance_rch instance = this->get_handle_instance(handle);

  if (!instance) {
    ACE_ERROR((LM_ERROR, "(%P|%t) DataReaderImpl::release_instance "
        "could not find the instance by handle 0x%x\n", handle));
    return;
  }

  this->purge_data(instance);

  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
    instances_.erase(handle);
  }

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (this->is_exclusive_ownership_ && instance->instance_state_->is_exclusive()) {
    if (owner_manager) {
      owner_manager->remove_writers(instance->instance_handle_);
    }

    for (RepoIdSet::const_iterator pos = instance->instance_state_->writers_begin(),
           limit = instance->instance_state_->writers_end(); pos != limit; ++pos) {
      WriterMapType::iterator iter = writers_.find(*pos);
      if (iter != writers_.end()) {
        iter->second->remove_instance(instance->instance_handle_);
      }
    }
  }
#endif

  this->release_instance_i(handle);
  if (this->monitor_) {
    this->monitor_->report();
  }
}

void
DataReaderImpl::state_updated(DDS::InstanceHandle_t handle)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
  state_updated_i(handle);
}

OpenDDS::DCPS::WriterStats::WriterStats(
    int amount,
    DataCollector<double>::OnFull type) : stats_(amount, type)
{
}

void OpenDDS::DCPS::WriterStats::add_stat(const TimeDuration& delay)
{
  double datum = static_cast<double>(delay.value().sec());
  datum += delay.value().usec() / 1000000.0;
  this->stats_.add(datum);
}

OpenDDS::DCPS::LatencyStatistics OpenDDS::DCPS::WriterStats::get_stats() const
{
  LatencyStatistics value;

  value.publication = GUID_UNKNOWN;
  value.n           = this->stats_.n();
  value.maximum     = this->stats_.maximum();
  value.minimum     = this->stats_.minimum();
  value.mean        = static_cast<double>(this->stats_.mean());
  value.variance    = static_cast<double>(this->stats_.var());

  return value;
}

void OpenDDS::DCPS::WriterStats::reset_stats()
{
  this->stats_.reset();
}

#ifndef OPENDDS_SAFETY_PROFILE
std::ostream& OpenDDS::DCPS::WriterStats::raw_data(std::ostream& str) const
{
  str << std::dec << this->stats_.size()
                              << " samples out of " << this->stats_.n() << std::endl;
  return str << this->stats_;
}
#endif //OPENDDS_SAFETY_PROFILE

void
DataReaderImpl::writer_removed(WriterInfo& info)
{
  const GUID_t info_writer_id = info.writer_id();

  if (DCPS_debug_level >= 5) {
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderImpl::writer_removed: ")
        ACE_TEXT("reader %C from writer %C.\n"),
        LogGuid(get_guid()).c_str(),
        LogGuid(info_writer_id).c_str()));
  }

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  OwnershipManagerPtr owner_manager = this->ownership_manager();
  if (owner_manager) {
    owner_manager->remove_writer(info_writer_id);
    info.clear_owner_evaluated();
  }
#endif

  {
    DDS::InstanceHandle_t publication_handle = DDS::HANDLE_NIL;
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, publication_handle_lock_);
      RepoIdToHandleMap::const_iterator pos = publication_id_to_handle_map_.find(info_writer_id);
      if (pos != publication_id_to_handle_map_.end()) {
        publication_handle = pos->second;
      }
    }

    bool liveliness_changed = false;

    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

    const WriterState info_state = info.state();

    if (info_state == ALIVE) {
      --liveliness_changed_status_.alive_count;
      --liveliness_changed_status_.alive_count_change;
      liveliness_changed = true;
    }

    if (info_state == DEAD) {
      --liveliness_changed_status_.not_alive_count;
      --liveliness_changed_status_.not_alive_count_change;
      liveliness_changed = true;
    }

    liveliness_changed_status_.last_publication_handle = info.handle();
    instances_liveliness_update(info_writer_id, publication_handle);

    if (liveliness_changed) {
      set_status_changed_flag(DDS::LIVELINESS_CHANGED_STATUS, true);
      this->notify_liveliness_change();
    }
  }
}

void
DataReaderImpl::writer_became_alive(WriterInfo& info,
                                    const MonotonicTimePoint& /* when */,
                                    WriterState previous_state)
{
  const GUID_t info_writer_id = info.writer_id();

  if (DCPS_debug_level >= 5) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderImpl::writer_became_alive: ")
               ACE_TEXT("reader %C from writer %C previous state %C.\n"),
               LogGuid(get_guid()).c_str(),
               LogGuid(info_writer_id).c_str(),
               get_state_str(previous_state)));
  }

  // NOTE: each instance will change to ALIVE_STATE when they receive a sample

  {
    bool liveliness_changed = false;

    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

    if (previous_state != ALIVE) {
      liveliness_changed_status_.alive_count++;
      liveliness_changed_status_.alive_count_change++;
      liveliness_changed = true;
    }

    if (previous_state == DEAD) {
      liveliness_changed_status_.not_alive_count--;
      liveliness_changed_status_.not_alive_count_change--;
    }

    if (liveliness_changed_status_.alive_count < 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_alive: ")
                 ACE_TEXT("invalid liveliness_changed_status alive count - %d.\n"),
                 liveliness_changed_status_.alive_count));
      return;
    }

    if (liveliness_changed_status_.not_alive_count < 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_alive: ")
                 ACE_TEXT("invalid liveliness_changed_status not alive count - %d.\n"),
                 liveliness_changed_status_.not_alive_count));
      return;
    }

    liveliness_changed_status_.last_publication_handle = info.handle();

    if (this->monitor_) {
      this->monitor_->report();
    }

    // Call listener only when there are liveliness status changes.
    if (liveliness_changed) {
      set_status_changed_flag(DDS::LIVELINESS_CHANGED_STATUS, true);
      this->notify_liveliness_change();
    }
  }
}

void
DataReaderImpl::writer_became_dead(WriterInfo& info,
                                   WriterState previous_state)
{
  const GUID_t info_writer_id = info.writer_id();

  if (DCPS_debug_level >= 5) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderImpl::writer_became_dead: ")
               ACE_TEXT("reader %C from writer %C previous state %C.\n"),
               LogGuid(get_guid()).c_str(),
               LogGuid(info_writer_id).c_str(),
               get_state_str(previous_state)));
  }

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  OwnershipManagerPtr owner_manager = this->ownership_manager();
  if (owner_manager) {
    owner_manager->remove_writer(info_writer_id);
    info.clear_owner_evaluated();
  }
#endif

  bool liveliness_changed = false;

  DDS::InstanceHandle_t publication_handle = DDS::HANDLE_NIL;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, publication_handle_lock_);
    RepoIdToHandleMap::const_iterator pos = publication_id_to_handle_map_.find(info_writer_id);
    if (pos != publication_id_to_handle_map_.end()) {
      publication_handle = pos->second;
    }
  }

  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

    if (previous_state != DEAD) {
      ++liveliness_changed_status_.not_alive_count;
      ++liveliness_changed_status_.not_alive_count_change;
      liveliness_changed = true;
    }

    if (previous_state == ALIVE) {
      --liveliness_changed_status_.alive_count;
      --liveliness_changed_status_.alive_count_change;
    }

    if (liveliness_changed_status_.alive_count < 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_dead: ")
                 ACE_TEXT("invalid liveliness_changed_status alive count - %d.\n"),
                 liveliness_changed_status_.alive_count));
      return;
    }

    if (liveliness_changed_status_.not_alive_count < 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_dead: ")
                 ACE_TEXT("invalid liveliness_changed_status not alive count - %d.\n"),
                 liveliness_changed_status_.not_alive_count));
      return;
    }

    liveliness_changed_status_.last_publication_handle = info.handle();

    if (this->monitor_) {
      this->monitor_->report();
    }

    instances_liveliness_update(info_writer_id, publication_handle);

    // Call listener only when there are liveliness status changes.
    if (liveliness_changed) {
      set_status_changed_flag(DDS::LIVELINESS_CHANGED_STATUS, true);
      this->notify_liveliness_change();
    }
  }
}

void
DataReaderImpl::instances_liveliness_update(const GUID_t& writer,
                                            DDS::InstanceHandle_t publication_handle)
{
  // sample_lock_ must be held.
  InstanceSet localinsts;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
    if (instances_.size() == 0) {
      return;
    }
    for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
         iter != instances_.end(); ++iter) {
      if (iter->second->instance_state_->writes_instance(writer)) {
        localinsts.insert(iter->first);
      }
    }
  }

  for (InstanceSet::iterator iter = localinsts.begin(); iter != localinsts.end(); ++iter) {
    set_instance_state_i(*iter, publication_handle, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, SystemTimePoint::now(), writer);
  }
}


void
DataReaderImpl::set_sample_lost_status(
    const DDS::SampleLostStatus& status)
{
  //!!!caller should have acquired sample_lock_
  sample_lost_status_ = status;
}

void
DataReaderImpl::set_sample_rejected_status(
    const DDS::SampleRejectedStatus& status)
{
  //!!!caller should have acquired sample_lock_
  sample_rejected_status_ = status;
}

void DataReaderImpl::dispose_unregister(const ReceivedDataSample&,
                                        DDS::InstanceHandle_t,
                                        SubscriptionInstance_rch&)
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReaderImpl::dispose_unregister()\n"));
  }
}

void DataReaderImpl::process_latency(const ReceivedDataSample& sample)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(statistics_lock_);
  StatsMapType::iterator location = this->statistics_.find(sample.header_.publication_id_);

  if (location != this->statistics_.end()) {
    const DDS::Duration_t zero = { DDS::DURATION_ZERO_SEC, DDS::DURATION_ZERO_NSEC };

    // Only when the user has specified a latency budget or statistics
    // are enabled we need to calculate our latency
    if ((this->statistics_enabled()) ||
        (this->qos_.latency_budget.duration > zero)) {
      const DDS::Time_t timestamp = {
        sample.header_.source_timestamp_sec_,
        sample.header_.source_timestamp_nanosec_
      };
      const TimeDuration latency = SystemTimePoint::now() - SystemTimePoint(timestamp);

      if (this->statistics_enabled()) {
        location->second.add_stat(latency);
      }

      if (DCPS_debug_level > 9) {
        ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DataReaderImpl::process_latency() - ")
            ACE_TEXT("measured latency of %C for current sample.\n"),
            latency.str().c_str()));
      }

      if (this->qos_.latency_budget.duration > zero) {
        // Check latency against the budget.
        if (latency > TimeDuration(this->qos_.latency_budget.duration)) {
          this->notify_latency(sample.header_.publication_id_);
        }
      }
    }
  } else if (DCPS_debug_level > 0) {
    /// NB: This message is generated contemporaneously with a similar
    ///     message from writer_activity().  That message is not marked
    ///     as an error, so we follow that lead and leave this as an
    ///     informational message, guarded by debug level.  This seems
    ///     to be due to late samples (samples delivered after an
    ///     association has been torn down).  We may want to promote this
    ///     to a warning if other conditions causing this symptom are
    ///     discovered.
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderImpl::process_latency() - ")
        ACE_TEXT("reader %C is not associated with writer %C (late sample?).\n"),
        LogGuid(get_guid()).c_str(),
        LogGuid(sample.header_.publication_id_).c_str()));
  }
}

void DataReaderImpl::notify_latency(GUID_t writer)
{
  // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
  // is given to this DataReader then narrow() fails.
  DataReaderListener_var listener = get_ext_listener();

  if (!CORBA::is_nil(listener.in())) {
    WriterIdSeq writerIds;
    writerIds.length(1);
    writerIds[ 0] = writer;

    DDS::InstanceHandleSeq handles;
    this->lookup_instance_handles(writerIds, handles);

    if (handles.length() >= 1) {
      this->budget_exceeded_status_.last_instance_handle = handles[ 0];

    } else {
      this->budget_exceeded_status_.last_instance_handle = -1;
    }

    ++this->budget_exceeded_status_.total_count;
    ++this->budget_exceeded_status_.total_count_change;

    listener->on_budget_exceeded(this, this->budget_exceeded_status_);

    this->budget_exceeded_status_.total_count_change = 0;
  }
}

#ifndef OPENDDS_SAFETY_PROFILE
void
DataReaderImpl::get_latency_stats(
    OpenDDS::DCPS::LatencyStatisticsSeq & stats)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(statistics_lock_);
  stats.length(static_cast<CORBA::ULong>(this->statistics_.size()));
  int index = 0;

  for (StatsMapType::const_iterator current = this->statistics_.begin();
      current != this->statistics_.end();
      ++current, ++index) {
    stats[ index] = current->second.get_stats();
    stats[ index].publication = current->first;
  }
}
#endif

void
DataReaderImpl::reset_latency_stats()
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(statistics_lock_);
  for (StatsMapType::iterator current = this->statistics_.begin();
      current != this->statistics_.end();
      ++current) {
    current->second.reset_stats();
  }
}

CORBA::Boolean
DataReaderImpl::statistics_enabled()
{
  return statistics_enabled_;
}

void
DataReaderImpl::statistics_enabled(
    CORBA::Boolean statistics_enabled)
{
  statistics_enabled_ = statistics_enabled;
}

void
DataReaderImpl::prepare_to_delete()
{
  const Observer_rch observer = get_observer(Observer::e_DELETED);
  if (observer) {
    observer->on_deleted(this);
  }

  this->set_deleted(true);
  this->stop_associating();
  if (!transport_disabled_) {
    this->send_final_acks();
  }
}

SubscriptionInstance_rch
DataReaderImpl::get_handle_instance(DDS::InstanceHandle_t handle)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_, SubscriptionInstance_rch());

  SubscriptionInstanceMapType::iterator iter = instances_.find(handle);
  if (iter == instances_.end()) {
    ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) WARNING: ")
        ACE_TEXT("DataReaderImpl::get_handle_instance: ")
        ACE_TEXT("lookup for 0x%x failed\n"),
        handle));
    return SubscriptionInstance_rch();
  } // if (0 != instances_.find(handle, instance))

  return iter->second;
}

DDS::InstanceHandle_t
DataReaderImpl::get_next_handle(const DDS::BuiltinTopicKey_t& key)
{
  RcHandle<DomainParticipantImpl> participant = this->participant_servant_.lock();
  if (!participant)
    return DDS::HANDLE_NIL;

  if (is_bit()) {
    const GUID_t id = bit_key_to_guid(key);
    return participant->assign_handle(id);

  } else {
    return participant->assign_handle();
  }
}

void DataReaderImpl::return_handle(DDS::InstanceHandle_t handle)
{
  const RcHandle<DomainParticipantImpl> participant = participant_servant_.lock();
  if (participant) {
    participant->return_handle(handle);
  }
}

void
DataReaderImpl::notify_subscription_disconnected(const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_disconnected",6);

  // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
  // is given to this DataReader then narrow() fails.
  DataReaderListener_var the_listener = get_ext_listener();

  if (!CORBA::is_nil(the_listener.in())) {
    SubscriptionLostStatus status;

    // Since this callback may come after remove_association which removes
    // the writer from id_to_handle map, we can ignore this error.
    this->lookup_instance_handles(pubids, status.publication_handles);
    the_listener->on_subscription_disconnected(this, status);
  }
}

void
DataReaderImpl::notify_subscription_reconnected(const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_reconnected",6);

  if (!this->is_bit_) {
    // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
    // is given to this DataReader then narrow() fails.
    DataReaderListener_var the_listener = get_ext_listener();

    if (!CORBA::is_nil(the_listener.in())) {
      SubscriptionLostStatus status;

      // If it's reconnected then the reader should be in id_to_handle
      this->lookup_instance_handles(pubids, status.publication_handles);

      the_listener->on_subscription_reconnected(this,  status);
    }
  }
}

void
DataReaderImpl::notify_subscription_lost(const DDS::InstanceHandleSeq& handles)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_lost",6);

  if (!this->is_bit_) {
    // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
    // is given to this DataReader then narrow() fails.
    DataReaderListener_var the_listener = get_ext_listener();

    if (!CORBA::is_nil(the_listener.in())) {
      SubscriptionLostStatus status;

      CORBA::ULong len = handles.length();
      status.publication_handles.length(len);

      for (CORBA::ULong i = 0; i < len; ++ i) {
        status.publication_handles[i] = handles[i];
      }

      the_listener->on_subscription_lost(this, status);
    }
  }
}

void
DataReaderImpl::notify_subscription_lost(const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_lost",6);

  // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
  // is given to this DataReader then narrow() fails.
  DataReaderListener_var the_listener = get_ext_listener();

  if (!CORBA::is_nil(the_listener.in())) {
    SubscriptionLostStatus status;

    // Since this callback may come after remove_association which removes
    // the writer from id_to_handle map, we can ignore this error.
    this->lookup_instance_handles(pubids, status.publication_handles);
    the_listener->on_subscription_lost(this, status);
  }
}


void
DataReaderImpl::lookup_instance_handles(const WriterIdSeq& ids,
    DDS::InstanceHandleSeq & hdls)
{
  CORBA::ULong const num_wrts = ids.length();

  if (DCPS_debug_level > 9) {
    const char* separator = "";
    OPENDDS_STRING guids;

    for (CORBA::ULong i = 0; i < num_wrts; ++i) {
      guids += separator;
      guids += LogGuid(ids[i]).conv_;
      separator = ", ";
    }

    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderImpl::lookup_instance_handles: ")
        ACE_TEXT("searching for handles for writer Ids: %C.\n"),
        guids.c_str()));
  }

  hdls.length(num_wrts);

  RcHandle<DomainParticipantImpl> participant = this->participant_servant_.lock();
  if (participant) {
    for (CORBA::ULong i = 0; i < num_wrts; ++i) {
      hdls[i] = participant->lookup_handle(ids[i]);
    }
  }
}

bool
DataReaderImpl::filter_sample(const DataSampleHeader& header)
{
  const SystemTimePoint now = SystemTimePoint::now();

  // Expire historic data if QoS indicates VOLATILE.
  if (!always_get_history_ && header.historic_sample_
      && qos_.durability.kind == DDS::VOLATILE_DURABILITY_QOS) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::filter_sample: ")
          ACE_TEXT("Discarded historic data.\n")));
    }

    return true; // Data filtered.
  }

  // The LIFESPAN_DURATION_FLAG is set when sample data is sent
  // with a non-default LIFESPAN duration value.
  if (header.lifespan_duration_) {
    // Finite lifespan.  Check if data has expired.

    const DDS::Time_t expiration_dds_time = {
      header.source_timestamp_sec_ + header.lifespan_duration_sec_,
      header.source_timestamp_nanosec_ + header.lifespan_duration_nanosec_
    };
    const SystemTimePoint expiration_time(expiration_dds_time);

    // We assume that the publisher host's clock and subcriber host's
    // clock are synchronized (allowed by the spec).
    if (now >= expiration_time) {
      if (DCPS_debug_level >= 8) {
        const TimeDuration diff(now - expiration_time);
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Received data ")
          ACE_TEXT("expired by %d seconds, %d microseconds.\n"),
          diff.value().sec(),
          diff.value().usec()));
      }

      return true; // Data filtered.
    }
  }

  return false;
}

bool
DataReaderImpl::ownership_filter_instance(const SubscriptionInstance_rch& instance,
  const GUID_t& pubid)
{
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (this->is_exclusive_ownership_) {

    ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex, write_guard, writers_lock_, true);
    WriterMapType::iterator iter = writers_.find(pubid);

    if (iter == writers_.end()) {
      if (DCPS_debug_level > 4) {
        // This may not be an error since it could happen that the sample
        // is delivered to the datareader after the write is dis-associated
        // with this datareader.
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataReaderImpl::ownership_filter_instance: ")
                   ACE_TEXT("reader %C is not associated with writer %C.\n"),
                   LogGuid(get_guid()).c_str(),
                   LogGuid(pubid).c_str()));
      }
      return true;
    }


    // Evaulate the owner of the instance if not selected and filter
    // current message if it's not from owner writer.
    if ( instance->instance_state_->get_owner() == GUID_UNKNOWN
        || ! iter->second->is_owner_evaluated(instance->instance_handle_)) {
      OwnershipManagerPtr owner_manager = this->ownership_manager();

      bool is_owner = owner_manager && owner_manager->select_owner (
        instance->instance_handle_,
        iter->second->writer_id(),
        iter->second->writer_qos_ownership_strength(),
        instance->instance_state_);
      iter->second->set_owner_evaluated(instance->instance_handle_, true);

      if (! is_owner) {
        if (DCPS_debug_level >= 1) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataReaderImpl::ownership_filter_instance: ")
                     ACE_TEXT("reader %C writer %C is not elected as owner %C\n"),
                     LogGuid(get_guid()).c_str(),
                     LogGuid(pubid).c_str(),
                     LogGuid(instance->instance_state_->get_owner()).c_str()));
        }
        return true;
      }
    }
    else if (! (instance->instance_state_->get_owner() == pubid)) {
      if (DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataReaderImpl::ownership_filter_instance: ")
                   ACE_TEXT("reader %C writer %C is not owner %C\n"),
                   LogGuid(get_guid()).c_str(),
                   LogGuid(pubid).c_str(),
                   LogGuid(instance->instance_state_->get_owner()).c_str()));
      }
      return true;
    }
  }
#else
  ACE_UNUSED_ARG(pubid);
  ACE_UNUSED_ARG(instance);
#endif
  return false;
}

bool
DataReaderImpl::time_based_filter_instance(const SubscriptionInstance_rch& instance,
                                           MonotonicTimePoint& now,
                                           MonotonicTimePoint& deadline)
{
  now = MonotonicTimePoint::now();
  const TimeDuration minimum_separation(qos_.time_based_filter.minimum_separation);

  // TIME_BASED_FILTER processing; expire data samples
  // if minimum separation is not met for instance.
  if (!minimum_separation.is_zero()) {
    if (now - instance->last_accepted_ < minimum_separation) {
      deadline = now + minimum_separation;
      return true; // Data filtered.
    }
  }

  instance->last_accepted_ = now;

  return false;
}

bool DataReaderImpl::is_bit() const
{
  return this->is_bit_;
}

bool
DataReaderImpl::has_zero_copies()
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      guard,
      this->sample_lock_,
      true /* assume we have loans */);
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_, true);

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
      iter != instances_.end();
      ++iter) {
    SubscriptionInstance_rch ptr = iter->second;

    if (ptr->rcvd_samples_.has_zero_copies()) {
      return true;
    }
  }

  return false;
}

void DataReaderImpl::notify_liveliness_change()
{
  // sample_lock_ must be held.
  // N.B. writers_lock_ should already be acquired when
  //      this method is called.

  DDS::DataReaderListener_var listener
  = listener_for(DDS::LIVELINESS_CHANGED_STATUS);

  if (!CORBA::is_nil(listener.in())) {
    const DDS::LivelinessChangedStatus status = liveliness_changed_status_;
    liveliness_changed_status_.alive_count_change = 0;
    liveliness_changed_status_.not_alive_count_change = 0;
    ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_sample_lock_);
    listener->on_liveliness_changed(this, status);
  }
  notify_status_condition();

  if (DCPS_debug_level > 9) {
    ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
    OPENDDS_STRING output_str;
    output_str += "subscription ";
    output_str += LogGuid(get_guid()).conv_;
    output_str += ", listener at: 0x";
    output_str += to_dds_string(this->listener_.in());

    for (WriterMapType::iterator current = this->writers_.begin();
         current != this->writers_.end();
         ++current) {
      const GUID_t id = current->first;
      output_str += "\n\tNOTIFY: writer[ ";
      output_str += LogGuid(id).conv_;
      output_str += "] == ";
      output_str += get_state_str(current->second->state());
    }

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::notify_liveliness_change: ")
               ACE_TEXT("listener at 0x%x, mask 0x%x.\n")
               ACE_TEXT("\tNOTIFY: %C\n"),
               listener.in(),
               listener_mask_,
               output_str.c_str()));
  }
}

void DataReaderImpl::post_read_or_take()
{
  set_status_changed_flag(DDS::DATA_AVAILABLE_STATUS, false);
  RcHandle<SubscriberImpl> subscriber = get_subscriber_servant();
  if (subscriber) {
    subscriber->set_status_changed_flag(
      DDS::DATA_ON_READERS_STATUS, false);
  }
}

ACE_Reactor_Timer_Interface*
DataReaderImpl::get_reactor()
{
  return this->reactor_;
}

OpenDDS::DCPS::GUID_t
DataReaderImpl::get_topic_id()
{
  return topic_id_;
}

OpenDDS::DCPS::GUID_t
DataReaderImpl::get_dp_id()
{
  return dp_id_;
}

void
DataReaderImpl::get_instance_handles(InstanceHandleVec& instance_handles)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
  ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_);

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin(),
      end = instances_.end(); iter != end; ++iter) {
    instance_handles.push_back(iter->first);
  }
}

void
DataReaderImpl::get_writer_states(WriterStatePairVec& writer_states)
{
  ACE_READ_GUARD(ACE_RW_Thread_Mutex,
      read_guard,
      this->writers_lock_);
  for (WriterMapType::iterator iter = writers_.begin();
      iter != writers_.end();
      ++iter) {
    writer_states.push_back(WriterStatePair(iter->first,
        iter->second->state()));
  }
}

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
void
DataReaderImpl::update_ownership_strength(const GUID_t& pub_id,
    const CORBA::Long& ownership_strength)
{
  ACE_READ_GUARD(ACE_RW_Thread_Mutex,
      read_guard,
      this->writers_lock_);
  for (WriterMapType::iterator iter = writers_.begin();
      iter != writers_.end();
      ++iter) {
    if (iter->second->writer_id() == pub_id) {
      if (ownership_strength != iter->second->writer_qos_ownership_strength()) {
        if (DCPS_debug_level >= 1) {
          ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataReaderImpl::update_ownership_strength - ")
              ACE_TEXT("local %C update remote %C strength from %d to %d\n"),
              LogGuid(get_guid()).c_str(),
              LogGuid(pub_id).c_str(),
              iter->second->writer_qos_ownership_strength(), ownership_strength));
        }
        iter->second->writer_qos_ownership_strength(ownership_strength);
        iter->second->clear_owner_evaluated();
      }
      break;
    }
  }
}
#endif

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
bool DataReaderImpl::verify_coherent_changes_completion(WriterInfo* writer)
{
  Coherent_State state = COMPLETED;
  bool accept_here = true;

  const GUID_t writer_id = writer->writer_id();
  const GUID_t publisher_id = writer->publisher_id();

  if (subqos_.presentation.access_scope != ::DDS::INSTANCE_PRESENTATION_QOS &&
      subqos_.presentation.coherent_access) {
    // verify current coherent changes from single writer
    state = writer->coherent_change_received();
    if (writer->group_coherent()) { // GROUP coherent any state
      RcHandle<SubscriberImpl> subscriber = get_subscriber_servant();
      if (subscriber && state != NOT_COMPLETED_YET) {
        // verify if all readers received complete coherent changes in a group.
        subscriber->coherent_change_received(publisher_id, this, state);
        accept_here = false; // coherent_change_received does that itself
      }
    } else if (state != NOT_COMPLETED_YET) { // TOPIC coherent with final state
      if (state == REJECTED) {
        reject_coherent(writer_id, publisher_id);
      }
      writer->reset_coherent_info();
    }
  }

  if (state == COMPLETED && accept_here) {
    accept_coherent(writer_id, publisher_id);
    coherent_changes_completed(this);
  }

  return state == COMPLETED;
}


void DataReaderImpl::accept_coherent(const GUID_t& writer_id,
    const GUID_t& publisher_id)
{
  if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderImpl::accept_coherent()")
        ACE_TEXT(" reader %C writer %C publisher %C\n"),
        LogGuid(get_guid()).c_str(),
        LogGuid(writer_id).c_str(),
        LogGuid(publisher_id).c_str()));
  }
  SubscriptionInstanceSet localsubs;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_);
    for (SubscriptionInstanceMapType::iterator iter = this->instances_.begin();
         iter != this->instances_.end(); ++iter) {
      localsubs.insert(iter->second);
    }
  }
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
  for (SubscriptionInstanceSet::iterator iter = localsubs.begin();
       iter != localsubs.end(); iter++) {
    (*iter)->rcvd_strategy_->accept_coherent(writer_id, publisher_id);
  }
}


void DataReaderImpl::reject_coherent(const GUID_t& writer_id,
    const GUID_t& publisher_id)
{
  if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderImpl::reject_coherent()")
        ACE_TEXT(" reader %C writer %C publisher %C\n"),
        LogGuid(get_guid()).c_str(),
        LogGuid(writer_id).c_str(),
        LogGuid(publisher_id).c_str()));
  }

  SubscriptionInstanceSet localsubs;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_);
    for (SubscriptionInstanceMapType::iterator iter = this->instances_.begin();
         iter != this->instances_.end(); ++iter) {
      localsubs.insert(iter->second);
    }
  }
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
  for (SubscriptionInstanceSet::iterator iter = localsubs.begin();
       iter != localsubs.end(); iter++) {
    (*iter)->rcvd_strategy_->reject_coherent(writer_id, publisher_id);
  }
  this->reset_coherent_info(writer_id, publisher_id);
}


void DataReaderImpl::reset_coherent_info(const GUID_t& writer_id,
    const GUID_t& publisher_id)
{
  ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);

  WriterMapType::iterator itEnd = this->writers_.end();
  for (WriterMapType::iterator it = this->writers_.begin();
      it != itEnd; ++it) {
    if (it->second->writer_id() == writer_id
        && it->second->publisher_id() == publisher_id) {
      it->second->reset_coherent_info();
    }
  }
}


void
DataReaderImpl::coherent_change_received(const GUID_t& publisher_id, Coherent_State& result)
{
  ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);

  result = COMPLETED;
  for (WriterMapType::iterator iter = writers_.begin();
      iter != writers_.end();
      ++iter) {

    if (iter->second->publisher_id() == publisher_id) {
      const Coherent_State state = iter->second->coherent_change_received();
      if (state == NOT_COMPLETED_YET) {
        result = NOT_COMPLETED_YET;
        break;
      }
      else if (state == REJECTED) {
        result = REJECTED;
      }
    }
  }
}


void
DataReaderImpl::coherent_changes_completed(DataReaderImpl* reader)
{
  RcHandle<SubscriberImpl> subscriber = get_subscriber_servant();
  if (!subscriber) {
    return;
  }

  subscriber->set_status_changed_flag(::DDS::DATA_ON_READERS_STATUS, true);
  this->set_status_changed_flag(::DDS::DATA_AVAILABLE_STATUS, true);

  ::DDS::SubscriberListener_var sub_listener =
      subscriber->listener_for(::DDS::DATA_ON_READERS_STATUS);
  if (!CORBA::is_nil(sub_listener.in()))
  {
    if (!is_bit()) {
      this->set_status_changed_flag(::DDS::DATA_AVAILABLE_STATUS, false);
      subscriber->set_status_changed_flag(::DDS::DATA_ON_READERS_STATUS, false);
      if (reader == this) {
        // Release the sample_lock before listener callback.
        ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_sample_lock_);
        sub_listener->on_data_on_readers(subscriber.in());
      }
    } else {
      TheServiceParticipant->job_queue()->enqueue(make_rch<OnDataOnReaders>(subscriber, sub_listener, rchandle_from(this), reader == this, true));
    }
  }
  else
  {
    subscriber->notify_status_condition();

    ::DDS::DataReaderListener_var listener =
        this->listener_for (::DDS::DATA_AVAILABLE_STATUS);

    if (!CORBA::is_nil(listener.in()))
    {
      if (!is_bit()) {
        set_status_changed_flag(::DDS::DATA_AVAILABLE_STATUS, false);
        subscriber->set_status_changed_flag(::DDS::DATA_ON_READERS_STATUS, false);
        if (reader == this) {
          // Release the sample_lock before listener callback.
          ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_sample_lock_);
          listener->on_data_available(this);
        } else {
          listener->on_data_available(this);
        }
      } else {
        TheServiceParticipant->job_queue()->enqueue(make_rch<OnDataAvailable>(listener, rchandle_from(this), reader == this, true, true));
      }
    }
    else
    {
      this->notify_status_condition();
    }
  }
}


void DataReaderImpl::begin_access()
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
  this->coherent_ = true;
}


void DataReaderImpl::end_access()
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
  this->coherent_ = false;
  this->group_coherent_ordered_data_.reset();
  this->post_read_or_take();
}


void DataReaderImpl::get_ordered_data(GroupRakeData& data,
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states)
{
  SubscriptionInstanceSet localsubs;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
    for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
         iter != instances_.end(); ++iter) {
      localsubs.insert(iter->second);
    }
  }

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

  for (SubscriptionInstanceSet::iterator iter = localsubs.begin(); iter != localsubs.end(); ++iter) {
    const SubscriptionInstance_rch inst = *iter;
    if (inst->instance_state_->match(view_states, instance_states)) {
      size_t i(0);
      for (ReceivedDataElement* item = inst->rcvd_samples_.get_next_match(sample_states, 0);
           item; item = inst->rcvd_samples_.get_next_match(sample_states, item)) {
          data.insert_sample(item, &inst->rcvd_samples_, *iter, ++i);
          group_coherent_ordered_data_.insert_sample(item, &inst->rcvd_samples_, *iter, ++i);
      }
    }
  }
}

#endif // OPENDDS_NO_OBJECT_MODEL_PROFILE

void
DataReaderImpl::set_subscriber_qos(
    const DDS::SubscriberQos & qos)
{
  this->subqos_ = qos;
}

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
void
DataReaderImpl::enable_filtering(ContentFilteredTopicImpl* cft)
{
  cft->add_reader(*this);
  {
    ACE_Guard<ACE_Thread_Mutex> guard(content_filtered_topic_mutex_);
    content_filtered_topic_ = cft;
  }
}

DDS::ContentFilteredTopic_ptr
DataReaderImpl::get_cf_topic() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(content_filtered_topic_mutex_);
  return DDS::ContentFilteredTopic::_duplicate(content_filtered_topic_.get());
}
#endif

#ifndef OPENDDS_NO_MULTI_TOPIC
void
DataReaderImpl::enable_multi_topic(MultiTopicImpl* mt)
{
  multi_topic_ = mt;
}
#endif

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

void
DataReaderImpl::update_subscription_params(const DDS::StringSeq& params) const
{
  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
  disco->update_subscription_params(domain_id_,
      dp_id_,
      subscription_id_,
      params);
}
#endif

void
DataReaderImpl::reset_ownership(::DDS::InstanceHandle_t instance)
{
  ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, this->writers_lock_);
  for (WriterMapType::iterator iter = writers_.begin();
      iter != writers_.end();
      ++iter) {
    iter->second->set_owner_evaluated(instance, false);
  }
}

void
DataReaderImpl::resume_sample_processing(const GUID_t& pub_id)
{
  WriterInfo_rch info;
  {
    ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, this->writers_lock_);
    WriterMapType::iterator where = writers_.find(pub_id);
    if (writers_.end() != where) {
      info = where->second;
    }
  }

  if (info) {
    resume_sample_processing(*info);
  }
}

void
DataReaderImpl::resume_sample_processing(WriterInfo& info)
{
  OPENDDS_MAP(SequenceNumber, ReceivedDataSample) to_deliver;
  // Stop filtering these
  if (info.check_end_historic_samples(to_deliver)) {
    deliver_historic(to_deliver);
    info.finished_delivering_historic();
  }
}

bool DataReaderImpl::check_historic(const ReceivedDataSample& sample)
{
  ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex, write_guard, writers_lock_, true);
  WriterMapType::iterator iter = writers_.find(sample.header_.publication_id_);
  if (iter != writers_.end()) {
    const SequenceNumber& seq = sample.header_.sequence_;
    SequenceNumber last_historic_seq;
    if (iter->second->check_historic(seq, sample, last_historic_seq)) {
      return false;
    }
    if (last_historic_seq != SequenceNumber::SEQUENCENUMBER_UNKNOWN()
        && !sample.header_.historic_sample_
        && seq <= last_historic_seq) {
      // this sample must have been seen before the END_HISTORIC_SAMPLES control msg
      return false;
    }
  }
  return true;
}

void DataReaderImpl::deliver_historic(OPENDDS_MAP(SequenceNumber, ReceivedDataSample)& samples)
{
  typedef OPENDDS_MAP(SequenceNumber, ReceivedDataSample)::iterator iter_t;
  const iter_t end = samples.end();
  for (iter_t iter = samples.begin(); iter != end; ++iter) {
    iter->second.header_.historic_sample_ = true;
    data_received(iter->second);
  }
}

void
DataReaderImpl::add_link(const DataLink_rch& link, const GUID_t& peer)
{
  if (this->qos_.durability.kind > DDS::VOLATILE_DURABILITY_QOS) {

    ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, writers_lock_);

    WriterMapType::iterator it = writers_.find(peer);
    if (it != writers_.end()) {
      // Schedule timer if necessary
      //   - only need to check reader qos - we know the writer must be >= reader
      it->second->schedule_historic_samples_timer();
    }
  }
  TransportClient::add_link(link, peer);

  if (!link->uses_end_historic_control_messages()) {
    resume_sample_processing(peer);
  }
}

void DataReaderImpl::get_flexible_types(const char* key, XTypes::TypeInformation& type_info)
{
  type_support_->get_flexible_types(key, type_info);
}

void
DataReaderImpl::register_for_writer(const GUID_t& participant,
                                    const GUID_t& readerid,
                                    const GUID_t& writerid,
                                    const TransportLocatorSeq& locators,
                                    DiscoveryListener* listener)
{
  TransportClient::register_for_writer(participant, readerid, writerid, locators, listener);
}

void
DataReaderImpl::unregister_for_writer(const GUID_t& participant,
                                      const GUID_t& readerid,
                                      const GUID_t& writerid)
{
  TransportClient::unregister_for_writer(participant, readerid, writerid);
}

void
DataReaderImpl::update_locators(const GUID_t& writerId,
                                const TransportLocatorSeq& locators)
{
  {
    ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, writers_lock_);
    WriterMapType::const_iterator iter = writers_.find(writerId);
    if (iter == writers_.end()) {
      return;
    }
  }
  TransportClient::update_locators(writerId, locators);
}

WeakRcHandle<ICE::Endpoint>
DataReaderImpl::get_ice_endpoint()
{
  return TransportClient::get_ice_endpoint();
}

DDS::ReturnCode_t DataReaderImpl::setup_deserialization()
{
  bool xcdr1_mutable = false;
  bool illegal_unaligned = false;
  for (CORBA::ULong i = 0; i < qos_.representation.value.length(); ++i) {
    Encoding::Kind encoding_kind;
    if (repr_to_encoding_kind(qos_.representation.value[i], encoding_kind)) {
      if (encoding_kind == Encoding::KIND_XCDR1 && type_support_->max_extensibility() == MUTABLE) {
        xcdr1_mutable = true;
      } else if (encoding_kind == Encoding::KIND_UNALIGNED_CDR && cdr_encapsulation()) {
        illegal_unaligned = true;
      } else {
        decoding_modes_.insert(encoding_kind);
      }
    } else if (DCPS_debug_level) {
      ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: "
                 "DataReaderImpl::setup_deserialization: "
                 "Encountered unsupported or unknown data representation: %C\n",
                 repr_to_string(qos_.representation.value[i]).c_str()));
    }
  }
  if (decoding_modes_.empty()) {
    if (DCPS_debug_level) {
      DCPS::String error_message;
      if (xcdr1_mutable) {
        error_message = " Unsupported combination of XCDR1 and mutable";
      } else if (illegal_unaligned) {
        error_message = " Unaligned CDR is not allowed in rtps_udp transport";
      }
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
                 "DataReaderImpl::setup_deserialization: "
                 "Could not find a valid data representation.%C\n",
                 error_message.c_str()));
    }
    return DDS::RETCODE_ERROR;
  }
  if (DCPS_debug_level >= 2) {
    OPENDDS_STRING encodings;
    EncodingKinds::iterator it = decoding_modes_.begin();
    for (; it != decoding_modes_.end(); ++it) {
      if (!encodings.empty()) {
        encodings += ", ";
      }
      encodings += Encoding::kind_to_string(*it);
    }
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReaderImpl::setup_deserialization: "
               "Setup successfully with the following data representation%C: %C\n",
               encodings.size() != 1 ? "s" : "",
               encodings.c_str()));
  }

  return DDS::RETCODE_OK;
}

void DataReaderImpl::accept_sample_processing(const SubscriptionInstance_rch& instance,
                                              const DataSampleHeader& header,
                                              bool is_new_instance)
{
  bool accepted = true;
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  bool verify_coherent = false;
#endif
  WriterInfo_rch writer;

  if (header.publication_id_.entityId.entityKind != ENTITYKIND_OPENDDS_NIL_WRITER) {
    ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, writers_lock_);

    WriterMapType::iterator where = writers_.find(header.publication_id_);

    if (where != writers_.end()) {
      if (header.coherent_change_) {

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
        // Received coherent change
        where->second->coherent_change(header.group_coherent_, header.publisher_id_);
        verify_coherent = true;
#endif
        writer = where->second;
      }
    }
    else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::accept_sample_processing - ")
        ACE_TEXT("subscription %C failed to find ")
        ACE_TEXT("publication data for %C.\n"),
        LogGuid(get_guid()).c_str(),
        LogGuid(header.publication_id_).c_str()));
    }
  }

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (verify_coherent) {
    accepted = verify_coherent_changes_completion(writer.in());
  }
#endif

  if (instance && deadline_queue_enabled_) {
    instance->last_sample_tv_ = instance->cur_sample_tv_;
    instance->cur_sample_tv_.set_to_now();

    if (is_new_instance) {
      schedule_deadline(instance, false);
    } else {
      process_deadline(instance, MonotonicTimePoint::now(), false);
    }
  }

  if (accepted) {
    notify_read_conditions();
  }
}

#if OPENDDS_CONFIG_SECURITY
DDS::Security::ParticipantCryptoHandle DataReaderImpl::get_crypto_handle() const
{
  RcHandle<DomainParticipantImpl> participant = participant_servant_.lock();
  return participant ? participant->crypto_handle() : DDS::HANDLE_NIL;
}
#endif

void DataReaderImpl::transport_discovery_change()
{
  RcHandle<DomainParticipantImpl> participant = participant_servant_.lock();
  populate_connection_info(participant.get());
  const TransportLocatorSeq& trans_conf_info = connection_info();
  const GUID_t dp_id_copy = dp_id_;
  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
  disco->update_subscription_locators(domain_id_,
                                      dp_id_copy,
                                      get_guid(),
                                      trans_conf_info);
}

void DataReaderImpl::OnDataOnReaders::execute()
{
  RcHandle<SubscriberImpl> subscriber = subscriber_.lock();
  RcHandle<DataReaderImpl> data_reader = data_reader_.lock();
  if (!subscriber || !data_reader) {
    return;
  }

  if (set_reader_status_) {
    data_reader->set_status_changed_flag(::DDS::DATA_AVAILABLE_STATUS, false);
  }
  subscriber->set_status_changed_flag(::DDS::DATA_ON_READERS_STATUS, false);

  if (call_) {
    sub_listener_->on_data_on_readers(subscriber.in());
  }
}

void DataReaderImpl::OnDataAvailable::execute()
{
  RcHandle<DataReaderImpl> data_reader = data_reader_.lock();

  if (data_reader && set_reader_status_) {
    data_reader->set_status_changed_flag(::DDS::DATA_AVAILABLE_STATUS, false);
  }

  if (data_reader && set_subscriber_status_) {
    RcHandle<SubscriberImpl> subscriber = data_reader->get_subscriber_servant();
    if (subscriber) {
      subscriber->set_status_changed_flag(::DDS::DATA_ON_READERS_STATUS, false);
    }
  }

  if (call_ && data_reader) {
    listener_->on_data_available(data_reader.in());
  }
}

void DataReaderImpl::initialize_lookup_maps()
{
  // These all start at 1 (0 mask is bogus) and include the full mask (any)
  for (CORBA::ULong is = 1; is <= MAX_SAMPLE_STATE_MASK; ++is) {
    for (CORBA::ULong iv = 1; iv <= MAX_VIEW_STATE_MASK; ++iv) {
      for (CORBA::ULong ii = 1; ii <= MAX_INSTANCE_STATE_MASK; ++ii) {
        combined_state_lookup_[to_combined_states(is, iv, ii)] = HandleSet();
      }
    }
  }
  // catch-all for "bogus" lookups
  combined_state_lookup_[0] = HandleSet();
}

void DataReaderImpl::update_lookup_maps(const SubscriptionInstanceMapType::iterator& input)
{
  for (LookupMap::iterator it = combined_state_lookup_.begin(); it != combined_state_lookup_.end(); ++it) {
    if (it->first == 0) continue;
    CORBA::ULong sample_states, view_states, instance_states;
    split_combined_states(it->first, sample_states, view_states, instance_states);
    if (input->second->matches(sample_states, view_states, instance_states)) {
      it->second.insert(input->first);
    } else {
      it->second.erase(input->first);
    }
  }
}

void DataReaderImpl::remove_from_lookup_maps(DDS::InstanceHandle_t handle)
{
  for (LookupMap::iterator it = combined_state_lookup_.begin(), the_end = combined_state_lookup_.end(); it != the_end; ++it) {
    if (it->first == 0) continue;
    it->second.erase(handle);
  }
}

const DataReaderImpl::HandleSet& DataReaderImpl::lookup_matching_instances(CORBA::ULong sample_states, CORBA::ULong view_states, CORBA::ULong instance_states) const
{
  const CORBA::ULong combined_states = to_combined_states(sample_states, view_states, instance_states);
  LookupMap::const_iterator ci = combined_state_lookup_.find(combined_states);
  OPENDDS_ASSERT(ci != combined_state_lookup_.end());
  return ci->second;
}

void DataReaderImpl::schedule_deadline(SubscriptionInstance_rch instance,
                                       bool timer_called)
{
  // Should be called with sample_lock_.
  if (instance->deadline_ == MonotonicTimePoint::zero_value) {
    instance->deadline_ = MonotonicTimePoint::now() + deadline_period_;
    const bool schedule = deadline_queue_.empty();
    deadline_queue_.insert(std::make_pair(instance->deadline_, instance));
    if (!timer_called) {
      if (schedule) {
        deadline_task_->schedule(deadline_period_);
      } else if (deadline_queue_.begin()->second == instance) {
        // Moved to front.
        deadline_task_->cancel();
        deadline_task_->schedule(deadline_period_);
      }
    }
  }
}

void DataReaderImpl::cancel_deadline(SubscriptionInstance_rch instance)
{
  // Should be called with sample_lock_.
  if (instance->deadline_ != MonotonicTimePoint::zero_value) {
    for (DeadlineQueue::iterator pos = deadline_queue_.lower_bound(instance->deadline_), limit = deadline_queue_.upper_bound(instance->deadline_); pos != limit; ++pos) {
      if (pos->second == instance) {
        deadline_queue_.erase(pos);
        break;
      }
    }
    instance->deadline_ = MonotonicTimePoint::zero_value;
  }
}

void DataReaderImpl::process_deadline(SubscriptionInstance_rch instance,
                                      const MonotonicTimePoint& now,
                                      bool timer_called)
{
  // Should be called with sample_lock_.

  if (instance->deadline_ != MonotonicTimePoint::zero_value) {
    bool missed = false;

    if (instance->cur_sample_tv_.is_zero()) { // not received any sample.
      missed = true;

    } else if (timer_called) { // handle_timeout is called
      missed = (now - instance->cur_sample_tv_) >= deadline_period_;

    } else { // upon receiving sample.
      missed = (instance->cur_sample_tv_ - instance->last_sample_tv_) > deadline_period_;
    }

    if (missed) {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, monitor, sample_lock_);
      // Only update the status upon timer is called and not
      // when receiving a sample after the interval.
      // Otherwise the counter is doubled.
      if (timer_called) {
        ++requested_deadline_missed_status_.total_count;
        requested_deadline_missed_status_.total_count_change =
          requested_deadline_missed_status_.total_count - last_deadline_missed_total_count_;
        requested_deadline_missed_status_.last_instance_handle = instance->instance_handle_;

        set_status_changed_flag(DDS::REQUESTED_DEADLINE_MISSED_STATUS, true);

        DDS::DataReaderListener_var listener = listener_for(DDS::REQUESTED_DEADLINE_MISSED_STATUS);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
        if (instance->instance_state_->is_exclusive()) {
          DataReaderImpl::OwnershipManagerPtr owner_manager = ownership_manager();
          if (owner_manager)
            owner_manager->remove_writers (instance->instance_handle_);
        }
#endif

        if (!CORBA::is_nil(listener.in())) {
          // Copy before releasing the lock.
          DDS::RequestedDeadlineMissedStatus const status = requested_deadline_missed_status_;

          // Release the lock during the upcall.
          ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_sample_lock_);
          // @todo Will this operation ever throw?  If so we may want to
          //       catch all exceptions, and act accordingly.
          listener->on_requested_deadline_missed(this, status);

          // We need to update the last total count value to our current total
          // so that the next time we will calculate the correct total_count_change;
          last_deadline_missed_total_count_ = requested_deadline_missed_status_.total_count;
        }

        notify_status_condition();
      }
    }

    // This next part is without status_lock_ held to avoid reactor deadlock.
    if (timer_called) {
      instance->deadline_ = MonotonicTimePoint::zero_value;
      schedule_deadline(instance, timer_called);
    } else {
      cancel_deadline(instance);
      schedule_deadline(instance, timer_called);
    }
  }
}

void DataReaderImpl::cancel_all_deadlines()
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
  deadline_queue_.clear();
  deadline_task_->cancel();
}

void DataReaderImpl::reset_deadline_period(const TimeDuration& deadline_period)
{
  if (deadline_period_ != deadline_period) {
    deadline_period_ = deadline_period;

    if (deadline_queue_enabled_) {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_);
      const MonotonicTimePoint now = MonotonicTimePoint::now();
      for (SubscriptionInstanceMapType::iterator iter = this->instances_.begin();
           iter != this->instances_.end();
           ++iter) {
        if (iter->second->deadline_ != MonotonicTimePoint::zero_value) {
          reschedule_deadline(iter->second, now);
        }
      }
    }
  }
}

void DataReaderImpl::reschedule_deadline(SubscriptionInstance_rch instance,
                                         const MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

  // So the datareader can call back into us.
  if (instance->deadline_ != MonotonicTimePoint::zero_value) {

    // Remove.
    for (DeadlineQueue::iterator pos = deadline_queue_.lower_bound(instance->deadline_), limit = deadline_queue_.upper_bound(instance->deadline_); pos != limit; ++pos) {
      if (pos->second == instance) {
        deadline_queue_.erase(pos);
        break;
      }
    }

    instance->deadline_ = now + (deadline_period_ - (instance->deadline_ - now));

    const bool schedule = deadline_queue_.empty();
    deadline_queue_.insert(std::make_pair(instance->deadline_, instance));
    if (schedule) {
      deadline_task_->schedule(deadline_period_);
    } else if (deadline_queue_.begin()->second == instance) {
      // Moved to front.
      deadline_task_->cancel();
      deadline_task_->schedule(deadline_period_);
    }
  }
}

void DataReaderImpl::deadline_task(const MonotonicTimePoint& now)
{
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
  for (DeadlineQueue::iterator pos = deadline_queue_.begin(), limit = deadline_queue_.end(); pos != limit && pos->first <= now;) {
    SubscriptionInstance_rch instance = pos->second;
    deadline_queue_.erase(pos++);
    // pos is no longer valid.
    process_deadline(instance, now, true);
  }

  if (!deadline_queue_.empty()) {
    deadline_task_->schedule(deadline_queue_.begin()->first - now);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
