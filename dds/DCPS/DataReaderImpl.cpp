/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataReaderImpl.h"
#include "tao/ORB_Core.h"
#include "SubscriptionInstance.h"
#include "ReceivedDataElementList.h"
#include "DomainParticipantImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "RepoIdConverter.h"
#include "RepoIdBuilder.h"
#include "TopicImpl.h"
#include "Serializer.h"
#include "SubscriberImpl.h"
#include "Transient_Kludge.h"
#include "Util.h"
#include "RequestedDeadlineWatchdog.h"
#include "QueryConditionImpl.h"
#include "MonitorFactory.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#if !defined (DDS_HAS_MINIMUM_BIT)
#include "BuiltInTopicUtils.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "ace/Reactor.h"
#include "ace/Auto_Ptr.h"

#include <sstream>

#if !defined (__ACE_INLINE__)
# include "DataReaderImpl.inl"
#endif /* !__ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

DataReaderImpl::DataReaderImpl()
  : rd_allocator_(0),
    qos_(TheServiceParticipant->initial_DataReaderQos()),
    reverse_sample_lock_(sample_lock_),
    participant_servant_(0),  
    topic_servant_(0),
    is_exclusive_ownership_ (false),
    owner_manager_ (0),
    topic_desc_(0),
    listener_mask_(DEFAULT_STATUS_MASK),
    fast_listener_(0),
    domain_id_(0),
    subscriber_servant_(0),
    subscription_id_(GUID_UNKNOWN),
    n_chunks_(TheServiceParticipant->n_chunks()),
    reactor_(0),
    liveliness_lease_duration_(ACE_Time_Value::zero),
    liveliness_timer_id_(-1),
    last_deadline_missed_total_count_(0),
    watchdog_(),
    is_bit_(false),
    initialized_(false),
    always_get_history_(false),
    statistics_enabled_(false),
    raw_latency_buffer_size_(0),
    raw_latency_buffer_type_(DataCollector<double>::KeepOldest),
    monitor_(0),
    periodic_monitor_(0)
{
  CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
  reactor_ = orb->orb_core()->reactor();

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

  monitor_ = TheServiceParticipant->monitor_factory_->create_data_reader_monitor(this);
  periodic_monitor_ = TheServiceParticipant->monitor_factory_->create_data_reader_periodic_monitor(this);
}

// This method is called when there are no longer any reference to the
// the servant.
DataReaderImpl::~DataReaderImpl()
{
  DBG_ENTRY_LVL("DataReaderImpl","~DataReaderImpl",6);

  for (WriterMapType::iterator iter = writers_.begin();
       iter != writers_.end();
       ++iter) {
    delete iter->second;
  }

  if (initialized_) {
    delete rd_allocator_;
  }
}

// this method is called when delete_datawriter is called.
void
DataReaderImpl::cleanup()
{
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex,
              guard,
              this->sample_lock_);

    if (liveliness_timer_id_ != -1) {
      (void) reactor_->cancel_timer(this);
    }
  }

  topic_servant_->remove_entity_ref();
  topic_servant_->_remove_ref();
  dr_local_objref_ = DDS::DataReader::_nil();
  deactivate_remote_object(dr_remote_objref_.in());
  dr_remote_objref_ = DataReaderRemote::_nil();
}

void DataReaderImpl::init(
  TopicImpl*         a_topic,
  const DDS::DataReaderQos &  qos,
  const DataReaderQosExt &      ext_qos,
  DDS::DataReaderListener_ptr a_listener,
  const DDS::StatusMask &     mask,
  DomainParticipantImpl*        participant,
  SubscriberImpl*               subscriber,
  DDS::DataReader_ptr         dr_objref,
  OpenDDS::DCPS::DataReaderRemote_ptr dr_remote_objref)
ACE_THROW_SPEC((CORBA::SystemException))
{
  topic_servant_ = a_topic;
  topic_servant_->_add_ref();

  topic_servant_->add_entity_ref();

  CORBA::String_var topic_name = topic_servant_->get_name();

#if !defined (DDS_HAS_MINIMUM_BIT)
  is_bit_ = ACE_OS::strcmp(topic_name.in(), BUILT_IN_PARTICIPANT_TOPIC) == 0
            || ACE_OS::strcmp(topic_name.in(), BUILT_IN_TOPIC_TOPIC) == 0
            || ACE_OS::strcmp(topic_name.in(), BUILT_IN_SUBSCRIPTION_TOPIC) == 0
            || ACE_OS::strcmp(topic_name.in(), BUILT_IN_PUBLICATION_TOPIC) == 0;
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  qos_ = qos;
  is_exclusive_ownership_ = this->qos_.ownership.kind == ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
  always_get_history_ = ext_qos.durability.always_get_history;

  listener_ = DDS::DataReaderListener::_duplicate(a_listener);

  if (!CORBA::is_nil(listener_.in())) {
    fast_listener_ = listener_.in();
  }

  listener_mask_ = mask;

  // Only store the participant pointer, since it is our "grand"
  // parent, we will exist as long as it does
  participant_servant_ = participant;
  
  if (is_exclusive_ownership_) {
    owner_manager_ = participant_servant_->ownership_manager ();
  }
  
  domain_id_ = participant_servant_->get_domain_id();
  topic_desc_ =
    participant_servant_->lookup_topicdescription(topic_name.in());

  // Only store the subscriber pointer, since it is our parent, we
  // will exist as long as it does.
  subscriber_servant_ = subscriber;
  dr_local_objref_    = DDS::DataReader::_duplicate(dr_objref);
  dr_remote_objref_   =
    OpenDDS::DCPS::DataReaderRemote::_duplicate(dr_remote_objref);

  initialized_ = true;
}

DDS::InstanceHandle_t
DataReaderImpl::get_instance_handle()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return this->participant_servant_->get_handle(subscription_id_);
}

void DataReaderImpl::add_associations(OpenDDS::DCPS::RepoId yourId,
                                      const OpenDDS::DCPS::WriterAssociationSeq & writers)
ACE_THROW_SPEC((CORBA::SystemException))
{
  //
  // The following block is for diagnostic purposes only.
  //
  if (DCPS_debug_level >= 1) {
    RepoIdConverter reader_converter(yourId);
    RepoIdConverter writer_converter(writers[0].writerId);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::add_associations - ")
               ACE_TEXT("bit %d local %C remote %C num remotes %d \n"),
               is_bit_,
               std::string(reader_converter).c_str(),
               std::string(writer_converter).c_str(),
               writers.length()));
  }

  //
  // This block prevents adding associations to deleted readers.
  // Presumably this is a "good thing(tm)".
  //
  if (entity_deleted_ == true) {
    if (DCPS_debug_level >= 1)
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataReaderImpl::add_associations")
                 ACE_TEXT(" This is a deleted datareader, ignoring add.\n")));

    return;
  }

  //
  // We are being called back from the repository before we are done
  // processing after our call to the repository that caused this call
  // (from the repository) to be made.
  //
  if (GUID_UNKNOWN == subscription_id_) {
    // add_associations was invoked before DCSPInfoRepo::add_subscription() returned.
    subscription_id_ = yourId;
  }

  CORBA::ULong wr_len = writers.length();
  WriterInfo ** infos = new WriterInfo*[wr_len];

  //
  // We do the following while holding the publication_handle_lock_.
  //
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

    //
    // For each writer in the list of writers to associate with, we
    // create a WriterInfo and a WriterStats object and store them in
    // our internal maps.
    //
    {
      ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, this->writers_lock_);

      for (CORBA::ULong i = 0; i < wr_len; i++) {
        PublicationId writer_id = writers[i].writerId;
        infos[i] = new WriterInfo(this, writer_id, writers[i].writerQos);
        this->writers_.insert(
          // This insertion is idempotent.
          WriterMapType::value_type(
            writer_id,
            infos[i]));
        this->statistics_.insert(
          StatsMapType::value_type(
            writer_id,
            WriterStats(
              this->raw_latency_buffer_size_,
              this->raw_latency_buffer_type_)));

        if (DCPS_debug_level > 4) {
          RepoIdConverter converter(writer_id);
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) DataReaderImpl::add_associations: "
                     "inserted writer %C.\n",
                     std::string(converter).c_str()));
        }
      }
    }

    //
    // Propagate the add_associations processing down into the Transport
    // layer here.  This will establish the transport support and reserve
    // usage of an existing connection or initiate creation of a new
    // connection if no suitable connection is available.
    //
    this->subscriber_servant_->add_associations(writers, this, qos_);

    // Check if any publications have already sent a REQUEST_ACK message.
    {
      ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);

      for (unsigned int index = 0; index < wr_len; ++index) {
        WriterMapType::iterator where
        = this->writers_.find(writers[ index].writerId);

        if (where != this->writers_.end()) {
          ACE_Time_Value now = ACE_OS::gettimeofday();

          ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

          if (where->second->should_ack(now)) {
            SequenceNumber sequence = where->second->ack_sequence();
            DDS::Time_t timenow = time_value_to_time(now);
            bool result = this->send_sample_ack(
                            writers[ index].writerId,
                            sequence.value_,
                            timenow);

            if (result) {
              where->second->clear_acks(sequence);
            }
          }
        }
      }
    }

    //
    // LIVELINESS policy timers are managed here.
    //
    if (liveliness_lease_duration_  != ACE_Time_Value::zero) {
      // this call will start the timer if it is not already set
      ACE_Time_Value now = ACE_OS::gettimeofday();

      if (DCPS_debug_level >= 5) {
        RepoIdConverter converter(subscription_id_);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataReaderImpl::add_associations: ")
                   ACE_TEXT("starting/resetting liveliness timer for reader %C\n"),
                   std::string(converter).c_str()));
      }

      this->handle_timeout(now, this);
    }

    // else - no timer needed when LIVELINESS.lease_duration is INFINITE

  }
  //
  // We no longer hold the publication_handle_lock_.
  //

  //
  // We only do the following processing for readers that are *not*
  // readers of Builtin Topics.
  //
  if (!is_bit_) {
    //
    // We derive a list of writer Id values corresponding to the writer
    // argument list.
    //
    WriterIdSeq wr_ids;
    wr_ids.length(wr_len);

    for (CORBA::ULong i = 0; i < wr_len; ++i) {
      wr_ids[i] = writers[i].writerId;
    }

    //
    // Here we convert the list of writer Id values to local handle
    // values.
    //
    DDS::InstanceHandleSeq handles;

    if (this->lookup_instance_handles(wr_ids, handles) == false
        || handles.length() != wr_len)
      return;

    //
    // We acquire the publication_handle_lock_ for the remainder of our
    // processing.
    //
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

      for (unsigned int index = 0; index < wr_len; ++index) {
        // This insertion is idempotent.
        this->id_to_handle_map_.insert(
          RepoIdToHandleMap::value_type(wr_ids[ index], handles[ index]));

        if (DCPS_debug_level > 4) {
          RepoIdConverter converter(wr_ids[index]);
          ACE_DEBUG((LM_WARNING,
                     ACE_TEXT("(%P|%t) DataReaderImpl::add_associations: ")
                     ACE_TEXT("id_to_handle_map_[ %C] = 0x%x.\n"),
                     std::string(converter).c_str(),
                     handles[index]));
        }
      }

      // We need to adjust these after the insertions have all completed
      // since insertions are not guaranteed to increase the number of
      // currently matched publications.
      int matchedPublications = static_cast<int>(this->id_to_handle_map_.size());
      this->subscription_match_status_.current_count_change
      = matchedPublications - this->subscription_match_status_.current_count;
      this->subscription_match_status_.current_count = matchedPublications;

      ++this->subscription_match_status_.total_count;
      ++this->subscription_match_status_.total_count_change;

      this->subscription_match_status_.last_publication_handle
      = handles[ wr_len - 1];

      set_status_changed_flag(DDS::SUBSCRIPTION_MATCHED_STATUS, true);

      DDS::DataReaderListener* listener
      = listener_for(DDS::SUBSCRIPTION_MATCHED_STATUS);

      if (listener != 0) {
        listener->on_subscription_matched(
          dr_local_objref_.in(),
          this->subscription_match_status_);

        // TBD - why does the spec say to change this but not change
        //       the ChangeFlagStatus after a listener call?

        // Client will look at it so next time it looks the change should be 0
        this->subscription_match_status_.total_count_change = 0;
        this->subscription_match_status_.current_count_change = 0;
      }

      notify_status_condition();
    }

    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

      for (unsigned int index = 0; index < wr_len; ++index) {
        infos[index]->handle_ = handles[index];
      }
    }
  }

  if (this->monitor_) {
    this->monitor_->report();
  }

  delete []infos;
}

void DataReaderImpl::remove_associations(
  const OpenDDS::DCPS::WriterIdSeq & writers,
  CORBA::Boolean notify_lost)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DBG_ENTRY_LVL("DataReaderImpl","remove_associations",6);

  if (DCPS_debug_level >= 1) {
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(writers[0]);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations: ")
               ACE_TEXT("bit %d local %C remote %C num remotes %d \n"),
               is_bit_,
               std::string(reader_converter).c_str(),
               std::string(writer_converter).c_str(),
               writers.length()));
  }

  DDS::InstanceHandleSeq handles;

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

  // This is used to hold the list of writers which were actually
  // removed, which is a proper subset of the writers which were
  // requested to be removed.
  WriterIdSeq updated_writers;

  CORBA::ULong wr_len;

  //Remove the writers from writer list. If the supplied writer
  //is not in the cached writers list then it is already removed.
  //We just need remove the writers in the list that have not been
  //removed.
  {
    ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, this->writers_lock_);

    wr_len = writers.length();

    for (CORBA::ULong i = 0; i < wr_len; i++) {
      PublicationId writer_id = writers[i];

      WriterMapType::iterator it = this->writers_.find(writer_id);

      if (it != this->writers_.end()) {
        it->second->removed();
      }

      delete it->second;

      if (this->writers_.erase(writer_id) == 0) {
        if (DCPS_debug_level >= 1) {
          RepoIdConverter converter(writer_id);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations: ")
                     ACE_TEXT("the writer local %C was already removed.\n"),
                     std::string(converter).c_str()));
        }

      } else {
        CORBA::Long len = updated_writers.length();
        updated_writers.length(len + 1);
        updated_writers[len] = writer_id;
      }
    }
  }

  wr_len = updated_writers.length();

  // Return now if the supplied writers have been removed already.
  if (wr_len == 0) {
    return;
  }

  if (!is_bit_) {
    // The writer should be in the id_to_handle map at this time.  Note
    // it if it not there.
    if (this->lookup_instance_handles(updated_writers, handles) == false) {
      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations: ")
                   ACE_TEXT("lookup_instance_handles failed.\n")));
      }
    }

    for (CORBA::ULong i = 0; i < wr_len; ++i) {
      id_to_handle_map_.erase(updated_writers[i]);
    }
  }

  this->subscriber_servant_->remove_associations(updated_writers, this->subscription_id_);

  // Mirror the add_associations SUBSCRIPTION_MATCHED_STATUS processing.
  if (!this->is_bit_) {
    // Derive the change in the number of publications writing to this reader.
    int matchedPublications = static_cast<int>(this->id_to_handle_map_.size());
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

      DDS::DataReaderListener* listener
      = listener_for(DDS::SUBSCRIPTION_MATCHED_STATUS);

      if (listener != 0) {
        listener->on_subscription_matched(
          dr_local_objref_.in(),
          this->subscription_match_status_);

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

void DataReaderImpl::remove_all_associations()
{
  DBG_ENTRY_LVL("DataReaderImpl","remove_all_associations",6);

  OpenDDS::DCPS::WriterIdSeq writers;
  int size;

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

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
    CORBA::Boolean dont_notify_lost = 0;

    if (0 < size) {
      remove_associations(writers, dont_notify_lost);
    }

  } catch (const CORBA::Exception&) {
  }
}

void DataReaderImpl::update_incompatible_qos(
  const OpenDDS::DCPS::IncompatibleQosStatus & status)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DDS::DataReaderListener* listener =
    listener_for(DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS);

  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            this->publication_handle_lock_);

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

  if (listener != 0) {
    listener->on_requested_incompatible_qos(dr_local_objref_.in(),
                                            requested_incompatible_qos_status_);

    // TBD - why does the spec say to change total_count_change but not
    // change the ChangeFlagStatus after a listener call?

    // client just looked at it so next time it looks the
    // change should be 0
    requested_incompatible_qos_status_.total_count_change = 0;
  }

  notify_status_condition();
}

DDS::ReadCondition_ptr DataReaderImpl::create_readcondition(
  DDS::SampleStateMask sample_states,
  DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_, 0);
  DDS::ReadCondition_var rc = new ReadConditionImpl(this, sample_states,
                                                    view_states, instance_states);
  read_conditions_.insert(rc);
  return rc._retn();
}

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
DDS::QueryCondition_ptr DataReaderImpl::create_querycondition(
  DDS::SampleStateMask sample_states,
  DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states,
  const char* query_expression,
  const DDS::StringSeq& query_parameters)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_, 0);
  try {
    DDS::QueryCondition_var qc = new QueryConditionImpl(this, sample_states,
      view_states, instance_states, query_expression, query_parameters);
    DDS::ReadCondition_var rc = DDS::ReadCondition::_duplicate(qc);
    read_conditions_.insert(rc);
    return qc._retn();
  } catch (const std::exception& e) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ")
                 ACE_TEXT("DataReaderImpl::create_querycondition - %C\n"),
                 e.what()));
    }
    return 0;
  }
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  DDS::ReadCondition_var rc = DDS::ReadCondition::_duplicate(a_condition);
  return read_conditions_.erase(rc)
         ? DDS::RETCODE_OK : DDS::RETCODE_PRECONDITION_NOT_MET;
}

DDS::ReturnCode_t DataReaderImpl::delete_contained_entities()
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (DCPS_debug_level >= 2)
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ")
               ACE_TEXT("DataReaderImpl::delete_contained_entities\n")));

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DataReaderImpl::set_qos(
  const DDS::DataReaderQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    if (!Qos_Helper::changeable(qos_, qos) && enabled_ == true) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      try {
        DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
        DDS::SubscriberQos subscriberQos;
        this->subscriber_servant_->get_qos(subscriberQos);
        repo->update_subscription_qos(this->participant_servant_->get_domain_id(),
                                      this->participant_servant_->get_id(),
                                      this->subscription_id_,
                                      qos,
                                      subscriberQos);

      } catch (const CORBA::SystemException& sysex) {
        sysex._tao_print_exception(
          "ERROR: System Exception"
          " in DataReaderImpl::set_qos");
        return DDS::RETCODE_ERROR;

      } catch (const CORBA::UserException& userex) {
        userex._tao_print_exception(
          "ERROR:  Exception"
          " in DataReaderImpl::set_qos");
        return DDS::RETCODE_ERROR;
      }
    }

    // Reset the deadline timer if the period has changed.
    if (qos_.deadline.period.sec != qos.deadline.period.sec
        || qos_.deadline.period.nanosec != qos.deadline.period.nanosec) {
      if (qos_.deadline.period.sec == DDS::DURATION_INFINITE_SEC
          && qos_.deadline.period.nanosec == DDS::DURATION_INFINITE_NSEC) {
        ACE_auto_ptr_reset(this->watchdog_,
                           new RequestedDeadlineWatchdog(
                             this->reactor_,
                             this->sample_lock_,
                             qos.deadline,
                             this,
                             this->dr_local_objref_.in(),
                             this->requested_deadline_missed_status_,
                             this->last_deadline_missed_total_count_));

      } else if (qos.deadline.period.sec == DDS::DURATION_INFINITE_SEC
                 && qos.deadline.period.nanosec == DDS::DURATION_INFINITE_NSEC) {
        this->watchdog_->cancel_all();
        this->watchdog_.reset();

      } else {
        this->watchdog_->reset_interval(
          duration_to_time_value(qos.deadline.period));
      }
    }

    qos_ = qos;

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
DataReaderImpl::get_qos(
  DDS::DataReaderQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
{
  qos = qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DataReaderImpl::set_listener(
  DDS::DataReaderListener_ptr a_listener,
  DDS::StatusMask mask)
ACE_THROW_SPEC((CORBA::SystemException))
{
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = DDS::DataReaderListener::_duplicate(a_listener);
  fast_listener_ = listener_.in();
  return DDS::RETCODE_OK;
}

DDS::DataReaderListener_ptr DataReaderImpl::get_listener()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return DDS::DataReaderListener::_duplicate(listener_.in());
}

DDS::TopicDescription_ptr DataReaderImpl::get_topicdescription()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return DDS::TopicDescription::_duplicate(topic_desc_.in());
}

DDS::Subscriber_ptr DataReaderImpl::get_subscriber()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return DDS::Subscriber::_duplicate(subscriber_servant_);
}

DDS::ReturnCode_t
DataReaderImpl::get_sample_rejected_status(
  DDS::SampleRejectedStatus & status)
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(
    this->publication_handle_lock_);

  set_status_changed_flag(DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS,
                          false);
  status = requested_incompatible_qos_status_;
  requested_incompatible_qos_status_.total_count_change = 0;

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::get_subscription_matched_status(
  DDS::SubscriptionMatchedStatus & status)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(
    this->publication_handle_lock_);

  set_status_changed_flag(DDS::SUBSCRIPTION_MATCHED_STATUS, false);
  status = subscription_match_status_;
  subscription_match_status_.total_count_change = 0;
  subscription_match_status_.current_count_change = 0;

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DataReaderImpl::get_sample_lost_status(
  DDS::SampleLostStatus & status)
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  // Add your implementation here
  return 0;
}

DDS::ReturnCode_t
DataReaderImpl::get_matched_publications(
  DDS::InstanceHandleSeq & publication_handles)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::get_matched_publications: ")
                      ACE_TEXT(" Entity is not enabled. \n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->publication_handle_lock_,
                   DDS::RETCODE_ERROR);

  // Copy out the handles for the current set of publications.
  int index = 0;
  publication_handles.length(static_cast<CORBA::ULong>(this->id_to_handle_map_.size()));

  for (RepoIdToHandleMap::iterator
       current = this->id_to_handle_map_.begin();
       current != this->id_to_handle_map_.end();
       ++current, ++index) {
    publication_handles[ index] = current->second;
  }

  return DDS::RETCODE_OK;
}

#if !defined (DDS_HAS_MINIMUM_BIT)
DDS::ReturnCode_t
DataReaderImpl::get_matched_publication_data(
  DDS::PublicationBuiltinTopicData & publication_data,
  DDS::InstanceHandle_t publication_handle)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::")
                      ACE_TEXT("get_matched_publication_data: ")
                      ACE_TEXT("Entity is not enabled. \n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->publication_handle_lock_,
                   DDS::RETCODE_ERROR);

  BIT_Helper_1 < DDS::PublicationBuiltinTopicDataDataReader,
  DDS::PublicationBuiltinTopicDataDataReader_var,
  DDS::PublicationBuiltinTopicDataSeq > hh;

  DDS::PublicationBuiltinTopicDataSeq data;

  DDS::ReturnCode_t ret
  = hh.instance_handle_to_bit_data(participant_servant_,
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  if (this->subscriber_servant_->is_enabled() == false) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
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
    // value that is for all intents and purposes unlimited
    // and we can use it for comparisons.
    // use 2147483647L because that is the greatest value a signed
    // CORBA::Long can have.
    // WARNING: The client risks running out of memory in this case.
    depth_ = 2147483647L;
  }

  if (qos_.resource_limits.max_samples != DDS::LENGTH_UNLIMITED) {
    n_chunks_ = qos_.resource_limits.max_samples;
  }

  //else using value from Service_Participant

  // enable the type specific part of this DataReader
  this->enable_specific();

  //Note: the QoS used to set n_chunks_ is Changable=No so
  // it is OK that we cannot change the size of our allocators.
  rd_allocator_ = new ReceivedDataAllocator(n_chunks_);

  if (DCPS_debug_level >= 2)
    ACE_DEBUG((LM_DEBUG,"(%P|%t) DataReaderImpl::enable"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               rd_allocator_, n_chunks_));

  if ((qos_.liveliness.lease_duration.sec !=
       DDS::DURATION_INFINITE_SEC) &&
      (qos_.liveliness.lease_duration.nanosec !=
       DDS::DURATION_INFINITE_NSEC)) {
    liveliness_lease_duration_ =
      duration_to_time_value(qos_.liveliness.lease_duration);
  }

  // Setup the requested deadline watchdog if the configured deadline
  // period is not the default (infinite).
  DDS::Duration_t const deadline_period = this->qos_.deadline.period;

  if (this->watchdog_.get() == 0
      && (deadline_period.sec != DDS::DURATION_INFINITE_SEC
          || deadline_period.nanosec != DDS::DURATION_INFINITE_NSEC)) {
    ACE_auto_ptr_reset(this->watchdog_,
                       new RequestedDeadlineWatchdog(
                         this->reactor_,
                         this->sample_lock_,
                         this->qos_.deadline,
                         this,
                         this->dr_local_objref_.in(),
                         this->requested_deadline_missed_status_,
                         this->last_deadline_missed_total_count_));
  }

  if (this->monitor_) {
    this->monitor_->report();
  }

  this->set_enabled();

  CORBA::String_var name = topic_servant_->get_name();

  return subscriber_servant_->reader_enabled(
           dr_remote_objref_.in(),
           dr_local_objref_.in(),
           this,
           name.in(),
           topic_servant_->get_id());
}

void
DataReaderImpl::writer_activity(const DataSampleHeader& header)
{
  // caller should have the sample_lock_ !!!

  WriterInfo* writer = 0;

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
      RepoIdConverter reader_converter(subscription_id_);
      RepoIdConverter writer_converter(header.publication_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataReaderImpl::writer_activity: ")
                 ACE_TEXT("reader %C is not associated with writer %C.\n"),
                 std::string(reader_converter).c_str(),
                 std::string(writer_converter).c_str()));
    }
  }

  if (writer != 0) {
    ACE_Time_Value when = ACE_OS::gettimeofday();
    writer->received_activity(when);

    if (header.message_id_ == SAMPLE_DATA) {
      if (writer->seen_data_) {
        // Data samples should be acknowledged prior to any
        // reader-side filtering to ensure discontiguities
        // are not unintentionally introduced.
        writer->ack_sequence(header.sequence_);

      } else {
        // In order to properly track out of order delivery,
        // a baseline must be established based on the first
        // data sample received.
        writer->seen_data_ = true;
        writer->ack_sequence_.reset(header.sequence_);
      }
    }
  }
}

void
DataReaderImpl::data_received(const ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("DataReaderImpl","data_received",6);

  // ensure some other thread is not changing the sample container
  // or statuses related to samples.
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << sample.header_ << std::ends;
    RepoIdConverter converter(subscription_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::data_received: ")
               ACE_TEXT("%C received sample: %C.\n"),
               std::string(converter).c_str(),
               buffer.str().c_str()));
  }

  switch (sample.header_.message_id_) {
  case SAMPLE_DATA:
  case INSTANCE_REGISTRATION: {
    DataSampleHeader const & header = sample.header_;

    this->writer_activity(header);

    // Verify data has not exceeded its lifespan.
    if (this->filter_sample(header)) break;

    // This adds the reader to the set/list of readers with data.
    this->subscriber_servant_->data_received(this);

    // Only gather statistics about real samples, not registration data, etc.
    if (header.message_id_ == SAMPLE_DATA) {
      this->process_latency(sample);
    }

    // This also adds to the sample container and makes any callbacks
    // and condition modifications.

    SubscriptionInstance* instance = 0;
    bool is_new_instance = false;
    bool filtered = false;
    dds_demarshal(sample, instance, is_new_instance, filtered);

    if (filtered) break; // sample filtered from instance

    {
      ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);

      WriterMapType::iterator where
      = this->writers_.find(header.publication_id_);

      if (where != this->writers_.end()) {
        if (header.coherent_change_) {
          // Received coherent change
          ++where->second->coherent_samples_;
        }

        ACE_Time_Value now = ACE_OS::gettimeofday();
        if (where->second->should_ack(now)) {
          DDS::Time_t timenow = time_value_to_time(now);
          bool result = this->send_sample_ack(
                          header.publication_id_,
                          header.sequence_,
                          timenow);

          if (result) {
            where->second->clear_acks(SequenceNumber(header.sequence_));
          }
        }

      } else {
        RepoIdConverter subscriptionBuffer(this->subscription_id_);
        RepoIdConverter publicationBuffer(header.publication_id_);
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::data_received() - ")
                   ACE_TEXT("subscription %C failed to find ")
                   ACE_TEXT("publication data for %C.\n"),
                   std::string(subscriptionBuffer).c_str(),
                   std::string(publicationBuffer).c_str()));
      }
    }

    if (this->watchdog_.get()) {
      instance->last_sample_tv_ = instance->cur_sample_tv_;
      instance->cur_sample_tv_ = ACE_OS::gettimeofday();

      if (is_new_instance) {
        this->watchdog_->schedule_timer(instance);

      } else {
        this->watchdog_->execute((void const *)instance, false);
      }
    }

    this->notify_read_conditions();
  }
  break;

  case REQUEST_ACK: {
    this->writer_activity(sample.header_);

    SequenceNumber ack;
    DDS::Duration_t delay;
    Serializer serializer(
      sample.sample_,
      sample.header_.byte_order_ != TAO_ENCAP_BYTE_ORDER);
    serializer >> ack.value_;
    serializer >> delay;

    if (DCPS_debug_level > 9) {
      RepoIdConverter debugConverter(sample.header_.publication_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataReaderImpl::data_received() - ")
                 ACE_TEXT("publication %C received REQUEST_ACK for sequence 0x%x ")
                 ACE_TEXT("valid for the next %d seconds.\n"),
                 std::string(debugConverter).c_str(),
                 ACE_UINT16(ack.value_),
                 delay.sec));
    }

    {
      ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);

      WriterMapType::iterator where
      = this->writers_.find(sample.header_.publication_id_);

      if (where != this->writers_.end()) {
        ACE_Time_Value now      = ACE_OS::gettimeofday();
        ACE_Time_Value deadline = now + duration_to_time_value(delay);

        where->second->ack_deadline(ack, deadline);

        if (where->second->should_ack(now)) {
          DDS::Time_t timenow = time_value_to_time(now);
          bool result = this->send_sample_ack(
                          sample.header_.publication_id_,
                          ack.value_,
                          timenow);

          if (result) {
            where->second->clear_acks(ack);
          }
        }

      } else {
        RepoIdConverter subscriptionBuffer(this->subscription_id_);
        RepoIdConverter publicationBuffer(sample.header_.publication_id_);
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::data_received() - ")
                   ACE_TEXT("subscription %C failed to find ")
                   ACE_TEXT("publication data for %C.\n"),
                   std::string(subscriptionBuffer).c_str(),
                   std::string(publicationBuffer).c_str()));
      }
    }

  }
  break;

  case END_COHERENT_CHANGES: {
    ACE_UINT32 coherent_samples;

    this->writer_activity(sample.header_);

    Serializer serializer(
      sample.sample_, sample.header_.byte_order_ != TAO_ENCAP_BYTE_ORDER);

    serializer >> coherent_samples;

    ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);
    {
      WriterMapType::iterator it =
        this->writers_.find(sample.header_.publication_id_);

      if (it == this->writers_.end()) {
        RepoIdConverter sub_id(this->subscription_id_);
        RepoIdConverter pub_id(sample.header_.publication_id_);
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::data_received()")
                   ACE_TEXT(" subscription %C failed to find ")
                   ACE_TEXT(" publication data for %C!\n"),
                   std::string(sub_id).c_str(),
                   std::string(pub_id).c_str()));
        return;
      }

      // Verify we have received all coherent samples
      // within the active change set. The following
      // assumes no out-of-order delivery!
      if (it->second->coherent_samples_ != coherent_samples) {
        RepoIdConverter sub_id(this->subscription_id_);
        RepoIdConverter pub_id(sample.header_.publication_id_);
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::data_received()")
                   ACE_TEXT(" subscription %C failed to receive ")
                   ACE_TEXT(" all coherent changes from publication %C!\n"),
                   std::string(sub_id).c_str(),
                   std::string(pub_id).c_str()));

        for (SubscriptionInstanceMapType::iterator current = this->instances_.begin();
             current != this->instances_.end(); ++current) {
          current->second->rcvd_strategy_->reject_coherent();
        }

        // Clear active change set count
        it->second->coherent_samples_ = 0;
        return;
      }

      for (SubscriptionInstanceMapType::iterator current = this->instances_.begin();
           current != this->instances_.end(); ++current) {
        current->second->rcvd_strategy_->accept_coherent();
      }

      // Clear active change set count
      it->second->coherent_samples_ = 0;
    }
    this->notify_read_conditions();
  }
  break;

  case DATAWRITER_LIVELINESS: {
    this->writer_activity(sample.header_);

    // tell all instances they got a liveliness message
    for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
         iter != instances_.end();
         ++iter) {
      SubscriptionInstance *ptr = iter->second;

      ptr->instance_state_.lively(sample.header_.publication_id_);
    }

  }
  break;

  case DISPOSE_INSTANCE: {
    this->writer_activity(sample.header_);
    SubscriptionInstance* instance = 0;
    this->dispose(sample, instance);
 
    if (this->watchdog_.get()) {
      if (! this->is_exclusive_ownership_ || instance != 0) {
        this->watchdog_->cancel_timer(instance);
      }
    }
  }
  this->notify_read_conditions();
  break;

  case UNREGISTER_INSTANCE: {
    this->writer_activity(sample.header_);
    SubscriptionInstance* instance = 0;
    this->unregister(sample, instance);

    if (this->watchdog_.get()) {
      if (! this->is_exclusive_ownership_ || instance != 0) {
        this->watchdog_->cancel_timer(instance);
      }
    }
  }
  this->notify_read_conditions();
  break;

  default:
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: DataReaderImpl::data_received"
               "unexpected message_id = %d\n",
               sample.header_.message_id_));
    break;
  }
}

bool
DataReaderImpl::send_sample_ack(
  const RepoId& publication,
  ACE_INT16 sequence,
  DDS::Time_t when)
{
  size_t dataSize = sizeof(sequence);
  dataSize += gen_find_size(publication);

  ACE_Message_Block* data;
  ACE_NEW_RETURN(data, ACE_Message_Block(dataSize), false);

  bool doSwap    = this->subscriber_servant_->swap_bytes();
  bool byteOrder = (doSwap? !TAO_ENCAP_BYTE_ORDER: TAO_ENCAP_BYTE_ORDER);

  Serializer serializer(data, doSwap);
  serializer << publication;
  serializer << sequence;

  DataSampleHeader outbound_header;
  outbound_header.message_id_               = SAMPLE_ACK;
  outbound_header.byte_order_               = byteOrder,
                                              outbound_header.coherent_change_          = 0;
  outbound_header.message_length_           = static_cast<ACE_UINT32>(data->total_length());
  outbound_header.sequence_                 = 0;
  outbound_header.source_timestamp_sec_     = when.sec;
  outbound_header.source_timestamp_nanosec_ = when.nanosec;
  outbound_header.publication_id_           = this->subscription_id_;

  ACE_Message_Block* sample_ack;
  ACE_NEW_RETURN(
    sample_ack,
    ACE_Message_Block(
      outbound_header.max_marshaled_size(),
      ACE_Message_Block::MB_DATA,
      data // cont
    ), false);
  sample_ack << outbound_header;

  if (DCPS_debug_level > 0) {
    RepoIdConverter subscriptionBuffer(this->subscription_id_);
    RepoIdConverter publicationBuffer(publication);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::send_sample_ack() - ")
               ACE_TEXT("%C sending SAMPLE_ACK message with sequence 0x%x ")
               ACE_TEXT("to publication %C.\n"),
               std::string(subscriptionBuffer).c_str(),
               ACE_UINT16(sequence),
               std::string(publicationBuffer).c_str()));
  }

  return this->subscriber_servant_->send_response(publication, sample_ack);
}

void DataReaderImpl::notify_read_conditions()
{
  //sample lock is already held
  ReadConditionSet local_read_conditions = read_conditions_;
  ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_sample_lock_);

  for (ReadConditionSet::iterator it = local_read_conditions.begin(),
       end = local_read_conditions.end(); it != end; ++it) {
    dynamic_cast<ConditionImpl*>(it->in())->signal_all();
  }
}

SubscriberImpl* DataReaderImpl::get_subscriber_servant()
{
  return subscriber_servant_;
}

RepoId DataReaderImpl::get_subscription_id() const
{
  return subscription_id_;
}

void DataReaderImpl::set_subscription_id(RepoId subscription_id)
{
  subscription_id_ = subscription_id;
}

char *
DataReaderImpl::get_topic_name() const
{
  return topic_servant_->get_name();
}

bool DataReaderImpl::have_sample_states(
  DDS::SampleStateMask sample_states) const
{
  //!!!caller should have acquired sample_lock_

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter) {
    SubscriptionInstance *ptr = iter->second;

    for (ReceivedDataElement *item = ptr->rcvd_samples_.head_;
         item != 0; item = item->next_data_sample_) {
      if (item->sample_state_ & sample_states) {
        return true;
      }
    }
  }

  return false;
}

bool
DataReaderImpl::have_view_states(DDS::ViewStateMask view_states) const
{
  //!!!caller should have acquired sample_lock_
  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter) {
    SubscriptionInstance *ptr = iter->second;

    if (ptr->instance_state_.view_state() & view_states) {
      return true;
    }
  }

  return false;
}

bool DataReaderImpl::have_instance_states(
  DDS::InstanceStateMask instance_states) const
{
  //!!!caller should have acquired sample_lock_
  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter) {
    SubscriptionInstance *ptr = iter->second;

    if (ptr->instance_state_.instance_state() & instance_states) {
      return true;
    }
  }

  return false;
}

/// Fold-in the three separate loops of have_sample_states(),
/// have_view_states(), and have_instance_states().  Takes the sample_lock_.
bool DataReaderImpl::contains_sample(DDS::SampleStateMask sample_states,
                                     DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, false);

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin(),
       end = instances_.end(); iter != end; ++iter) {
    SubscriptionInstance& inst = *iter->second;

    if ((inst.instance_state_.view_state() & view_states) &&
        (inst.instance_state_.instance_state() & instance_states)) {
      for (ReceivedDataElement* item = inst.rcvd_samples_.head_; item != 0;
           item = item->next_data_sample_) {
        if (item->sample_state_ & sample_states
            && !item->coherent_change_) {
          return true;
        }
      }
    }
  }

  return false;
}

DDS::DataReaderListener*
DataReaderImpl::listener_for(DDS::StatusKind kind)
{
  // per 2.1.4.3.1 Listener Access to Plain Communication Status
  // use this entities factory if listener is mask not enabled
  // for this kind.
  if (fast_listener_ == 0 || (listener_mask_ & kind) == 0) {
    return subscriber_servant_->listener_for(kind);

  } else {
    return fast_listener_;
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
}

CORBA::Long DataReaderImpl::total_samples() const
{
  //!!!caller should have acquired sample_lock_
  CORBA::Long count(0);

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter) {
    SubscriptionInstance *ptr = iter->second;

    count += static_cast<CORBA::Long>(ptr->rcvd_samples_.size_);
  }

  return count;
}

int
DataReaderImpl::handle_timeout(const ACE_Time_Value &tv,
                               const void * arg)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->sample_lock_,
                   DDS::RETCODE_ERROR);

  if (liveliness_timer_id_ != -1) {
    if (arg == this) {

      if (DCPS_debug_level >= 5) {
        RepoIdConverter converter(subscription_id_);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataReaderImpl::handle_timeout: ")
                   ACE_TEXT(" canceling timer for reader %C.\n"),
                   std::string(converter).c_str()));
      }

      // called from add_associations and there is already a timer
      // so cancel the existing timer.
      if (reactor_->cancel_timer(this->liveliness_timer_id_, &arg) == -1) {
        // this could fail because the reactor's call and
        // the add_associations' call to this could overlap
        // so it is not a failure.
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::handle_timeout: ")
                   ACE_TEXT(" %p. \n"), ACE_TEXT("cancel_timer")));
      }

      liveliness_timer_id_ = -1;
    }
  }

  ACE_Time_Value smallest(ACE_Time_Value::max_time);
  ACE_Time_Value next_absolute;
  int alive_writers = 0;

  // Iterate over each writer to this reader
  {
    ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,
                          read_guard,
                          this->writers_lock_,
                          0);

    for (WriterMapType::iterator iter = writers_.begin();
         iter != writers_.end();
         ++iter) {
      // deal with possibly not being alive or
      // tell when it will not be alive next (if no activity)
      next_absolute = iter->second->check_activity(tv);

      if (next_absolute != ACE_Time_Value::max_time) {
        alive_writers++;

        if (next_absolute < smallest) {
          smallest = next_absolute;
        }
      }
    }
  }

  if (DCPS_debug_level >= 5) {
    RepoIdConverter converter(subscription_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::handle_timeout: ")
               ACE_TEXT("reader %C has %d live writers; from_reactor=%d\n"),
               std::string(converter).c_str(),
               alive_writers,
               arg == this ? 0 : 1));
  }

  if (alive_writers) {
    ACE_Time_Value relative;
    ACE_Time_Value now = ACE_OS::gettimeofday();

    // compare the time now with the earliest(smallest) deadline we found
    if (now < smallest)
      relative = smallest - now;

    else
      relative = ACE_Time_Value(0,1); // ASAP

    liveliness_timer_id_ = reactor_->schedule_timer(this, 0, relative);

    if (liveliness_timer_id_ == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::handle_timeout: ")
                 ACE_TEXT(" %p. \n"), ACE_TEXT("schedule_timer")));
    }

  } else {
    // no live writers so no need to schedule a timer
    // but be sure we don't try to cancel the timer later.
    liveliness_timer_id_ = -1;
  }

  return 0;
}

void
DataReaderImpl::release_instance(DDS::InstanceHandle_t handle)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);
  SubscriptionInstance* instance = this->get_handle_instance(handle);

  if (instance == 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) DataReaderImpl::release_instance "
               "could not find the instance by handle 0x%x\n", handle));
    return;
  }

  this->purge_data(instance);

  this->instances_.erase(handle);
  this->release_instance_i(handle);
  if (this->monitor_) {
    this->monitor_->report();
  }
}

OpenDDS::DCPS::WriterInfo::WriterInfo()
  : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
    seen_data_(false),
    state_(NOT_SET),
    reader_(0),
    writer_id_(GUID_UNKNOWN),
    handle_(DDS::HANDLE_NIL),
    coherent_samples_(0),
    owner_evaluated_ (false)
{
}

OpenDDS::DCPS::WriterInfo::WriterInfo(OpenDDS::DCPS::DataReaderImpl* reader,
                                      const OpenDDS::DCPS::PublicationId& writer_id,
                                      const ::DDS::DataWriterQos&  writer_qos)
  : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
    seen_data_(false),
    state_(NOT_SET),
    reader_(reader),
    writer_id_(writer_id),
    writer_qos_(writer_qos),
    coherent_samples_(0),
    owner_evaluated_ (false)
{
  if (DCPS_debug_level >= 5) {
    RepoIdConverter writer_converter(writer_id);
    RepoIdConverter reader_converter(reader->subscription_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) WriterInfo::WriterInfo: ")
               ACE_TEXT("writer %C added to reader %C.\n"),
               std::string(writer_converter).c_str(),
               std::string(reader_converter).c_str()));
  }
}

ACE_Time_Value
OpenDDS::DCPS::WriterInfo::check_activity(const ACE_Time_Value& now)
{
  ACE_Time_Value expires_at = ACE_Time_Value::max_time;

  // We only need check the liveliness with the non-zero liveliness_lease_duration_.
  // if (state_ != DEAD && reader_->liveliness_lease_duration_ != ACE_Time_Value::zero)
  if (state_ == ALIVE && reader_->liveliness_lease_duration_ != ACE_Time_Value::zero) {
    expires_at = this->last_liveliness_activity_time_ +
                 reader_->liveliness_lease_duration_;

    if (expires_at <= now) {
      // let all instances know this write is not alive.
      reader_->writer_became_dead(*this, now);
      expires_at = ACE_Time_Value::max_time;
    }
  }

  return expires_at;
}

void OpenDDS::DCPS::WriterInfo::removed()
{
  reader_->writer_removed(*this);
}

void
OpenDDS::DCPS::WriterInfo::clear_acks(
  SequenceNumber sequence)
{
  // sample_lock_ is held by the caller.

  DeadlineList::iterator current
  = this->ack_deadlines_.begin();

  while (current != this->ack_deadlines_.end()) {
    if (current->first <= sequence) {
      current = this->ack_deadlines_.erase(current);

    } else {
      break;
    }
  }
}

bool
OpenDDS::DCPS::WriterInfo::should_ack(
  ACE_Time_Value now)
{
  // sample_lock_ is held by the caller.

  if (this->ack_deadlines_.size() == 0) {
    return false;
  }

  DeadlineList::iterator current
  = this->ack_deadlines_.begin();

  while (current != this->ack_deadlines_.end()) {
    if (current->second < now) {
      // Remove any expired response deadlines.
      current = this->ack_deadlines_.erase(current);

    } else {
      if (current->first <= this->ack_sequence_) {
        return true;
      }

      ++current;
    }
  }

  return false;
}

void
OpenDDS::DCPS::WriterInfo::ack_deadline(SequenceNumber sequence, ACE_Time_Value when)
{
  // sample_lock_ is held by the caller.

  if (this->ack_deadlines_.size() == 0) {
    this->ack_deadlines_.push_back(std::make_pair(sequence, when));
    return;
  }

  for (DeadlineList::iterator current
       = this->ack_deadlines_.begin();
       current != this->ack_deadlines_.end();
       ++current) {
    // Insertion sort.
    if (sequence < current->first) {
      this->ack_deadlines_.insert(
        current,
        std::make_pair(sequence, when));
      return;

    } else if (sequence == current->first) {
      // Only update the deadline to be *later* than any existing one.
      if (current->second < when) {
        current->second = when;
      }

      return;

    } else {
      break;
    }
  }
}

void
OpenDDS::DCPS::WriterInfo::ack_sequence(SequenceNumber value)
{
  // sample_lock_ is held by the caller.
  this->ack_sequence_.update(value);
}

SequenceNumber
OpenDDS::DCPS::WriterInfo::ack_sequence() const
{
  // sample_lock_ is held by the caller.
  return this->ack_sequence_;
}

OpenDDS::DCPS::WriterStats::WriterStats(
  int amount,
  DataCollector<double>::OnFull type) : stats_(amount, type)
{
}

void OpenDDS::DCPS::WriterStats::add_stat(const ACE_Time_Value& delay)
{
  double datum = static_cast<double>(delay.sec());
  datum += delay.usec() / 1000000.0;
  this->stats_.add(datum);
}

OpenDDS::DCPS::LatencyStatistics OpenDDS::DCPS::WriterStats::get_stats() const
{
  LatencyStatistics value;

  value.publication = GUID_UNKNOWN;
  value.n           = this->stats_.n();
  value.maximum     = this->stats_.maximum();
  value.minimum     = this->stats_.minimum();
  value.mean        = this->stats_.mean();
  value.variance    = this->stats_.var();

  return value;
}

void OpenDDS::DCPS::WriterStats::reset_stats()
{
  this->stats_.reset();
}

std::ostream& OpenDDS::DCPS::WriterStats::raw_data(std::ostream& str) const
{
  str << std::dec << this->stats_.size()
  << " samples out of " << this->stats_.n() << std::endl;
  return str << this->stats_;
}

void
DataReaderImpl::writer_removed(WriterInfo& info)
{
  if (DCPS_debug_level >= 5) {
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(info.writer_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::writer_removed: ")
               ACE_TEXT("reader %C from writer %C.\n"),
               std::string(reader_converter).c_str(),
               std::string(writer_converter).c_str()));
  }

  if (this->is_exclusive_ownership_) {
    this->owner_manager_->remove_writer (info.writer_id_);
    info.owner_evaluated_ = false;
  }
  
  bool liveliness_changed = false;

  if (info.state_ == WriterInfo::ALIVE) {
    -- liveliness_changed_status_.alive_count;
    -- liveliness_changed_status_.alive_count_change;
    liveliness_changed = true;
  }

  if (info.state_ == WriterInfo::DEAD) {
    -- liveliness_changed_status_.not_alive_count;
    -- liveliness_changed_status_.not_alive_count_change;
    liveliness_changed = true;
  }

  liveliness_changed_status_.last_publication_handle = info.handle_;

  if (liveliness_changed) {
    set_status_changed_flag(DDS::LIVELINESS_CHANGED_STATUS, true);
    this->notify_liveliness_change();
  }
}

void
DataReaderImpl::writer_became_alive(WriterInfo& info,
                                    const ACE_Time_Value& /* when */)
{
  if (DCPS_debug_level >= 5) {
    std::stringstream buffer;
    buffer << info.state_;
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(info.writer_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::writer_became_alive: ")
               ACE_TEXT("reader %C from writer %C state %C.\n"),
               std::string(reader_converter).c_str(),
               std::string(writer_converter).c_str(),
               buffer.str().c_str()));
  }

  // caller should already have the samples_lock_ !!!

  // NOTE: each instance will change to ALIVE_STATE when they receive a sample

  bool liveliness_changed = false;

  if (info.state_ != WriterInfo::ALIVE) {
    liveliness_changed_status_.alive_count++;
    liveliness_changed_status_.alive_count_change++;
    liveliness_changed = true;
  }

  if (info.state_ == WriterInfo::DEAD) {
    liveliness_changed_status_.not_alive_count--;
    liveliness_changed_status_.not_alive_count_change--;
    liveliness_changed = true;
  }

  liveliness_changed_status_.last_publication_handle = info.handle_;

  set_status_changed_flag(DDS::LIVELINESS_CHANGED_STATUS, true);

  if (liveliness_changed_status_.alive_count < 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_alive: ")
               ACE_TEXT(" invalid liveliness_changed_status alive count - %d.\n"),
               liveliness_changed_status_.alive_count));
    return;
  }

  if (liveliness_changed_status_.not_alive_count < 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_alive: ")
               ACE_TEXT(" invalid liveliness_changed_status not alive count - %d .\n"),
               liveliness_changed_status_.not_alive_count));
    return;
  }

  // Change the state to ALIVE since handle_timeout may call writer_became_dead
  // which need the current state info.
  info.state_ = WriterInfo::ALIVE;

  if (this->monitor_) {
    this->monitor_->report();
  }

  // Call listener only when there are liveliness status changes.
  if (liveliness_changed) {
    this->notify_liveliness_change();
  }

  // this call will start the livilness timer if it is not already set
  ACE_Time_Value now = ACE_OS::gettimeofday();
  this->handle_timeout(now, this);
}

void
DataReaderImpl::writer_became_dead(WriterInfo & info,
                                   const ACE_Time_Value& when)
{
  if (DCPS_debug_level >= 5) {
    std::stringstream buffer;
    buffer << info.state_;
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(info.writer_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::writer_became_dead: ")
               ACE_TEXT("reader %C from writer%C state %C.\n"),
               std::string(reader_converter).c_str(),
               std::string(writer_converter).c_str(),
               buffer.str().c_str()));
  }

  if (this->is_exclusive_ownership_) {
    this->owner_manager_->remove_writer (info.writer_id_); 
    info.owner_evaluated_ = false;   
  }
  
  // caller should already have the samples_lock_ !!!
  bool liveliness_changed = false;

  if (info.state_ == OpenDDS::DCPS::WriterInfo::NOT_SET) {
    liveliness_changed_status_.not_alive_count++;
    liveliness_changed_status_.not_alive_count_change++;
    liveliness_changed = true;
  }

  if (info.state_ == WriterInfo::ALIVE) {
    liveliness_changed_status_.alive_count--;
    liveliness_changed_status_.alive_count_change--;
    liveliness_changed_status_.not_alive_count++;
    liveliness_changed_status_.not_alive_count_change++;
    liveliness_changed = true;
  }

  liveliness_changed_status_.last_publication_handle = info.handle_;

  //update the state to DEAD.
  info.state_ = WriterInfo::DEAD;
  info.seen_data_ = false;

  if (this->monitor_) {
    this->monitor_->report();
  }

  if (liveliness_changed_status_.alive_count < 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_dead: ")
               ACE_TEXT(" invalid liveliness_changed_status alive count - %d.\n"),
               liveliness_changed_status_.alive_count));
    return;
  }

  if (liveliness_changed_status_.not_alive_count < 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_dead: ")
               ACE_TEXT(" invalid liveliness_changed_status not alive count - %d.\n"),
               liveliness_changed_status_.not_alive_count));
    return;
  }

  SubscriptionInstanceMapType::iterator iter = instances_.begin();
  SubscriptionInstanceMapType::iterator next = iter;

  while (iter != instances_.end()) {
    ++next;
    SubscriptionInstance *ptr = iter->second;

    ptr->instance_state_.writer_became_dead(
      info.writer_id_, liveliness_changed_status_.alive_count, when);

    iter = next;
  }

  // Call listener only when there are liveliness status changes.
  if (liveliness_changed) {
    set_status_changed_flag(DDS::LIVELINESS_CHANGED_STATUS, true);
    this->notify_liveliness_change();
  }
}

int
DataReaderImpl::handle_close(ACE_HANDLE,
                             ACE_Reactor_Mask)
{
  //this->_remove_ref ();
  return 0;
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

void DataReaderImpl::dispose(const ReceivedDataSample&,
                             SubscriptionInstance*&)
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReaderImpl::dispose()\n"));
  }
}

void DataReaderImpl::unregister(const ReceivedDataSample&,
                                SubscriptionInstance*&)
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReaderImpl::unregister()\n"));
  }
}

void DataReaderImpl::process_latency(const ReceivedDataSample& sample)
{
  StatsMapType::iterator location
  = this->statistics_.find(sample.header_.publication_id_);

  if (location != this->statistics_.end()) {
    // This starts as the current time.
    ACE_Time_Value  latency = ACE_OS::gettimeofday();

    // The time interval starts at the send end.
    DDS::Duration_t then = {
      sample.header_.source_timestamp_sec_,
      sample.header_.source_timestamp_nanosec_
    };

    // latency delay in ACE_Time_Value format.
    latency -= duration_to_time_value(then);

    if (this->statistics_enabled()) {
      location->second.add_stat(latency);
    }

    if (DCPS_debug_level > 9) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataReaderImpl::process_latency() - ")
                 ACE_TEXT("measured latency of %dS, %dmS for current sample.\n"),
                 latency.sec(),
                 latency.msec()));
    }

    // Check latency against the budget.
    if (time_value_to_duration(latency)
        > this->qos_.latency_budget.duration) {
      this->notify_latency(sample.header_.publication_id_);
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
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(sample.header_.publication_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::process_latency() - ")
               ACE_TEXT("reader %C is not associated with writer %C (late sample?).\n"),
               std::string(reader_converter).c_str(),
               std::string(writer_converter).c_str()));
  }
}

void DataReaderImpl::notify_latency(PublicationId writer)
{
  // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
  // is given to this DataReader then narrow() fails.
  DataReaderListener_var listener
  = DataReaderListener::_narrow(this->listener_.in());

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

    listener->on_budget_exceeded(
      this->dr_local_objref_.in(),
      this->budget_exceeded_status_);

    this->budget_exceeded_status_.total_count_change = 0;
  }
}

void
DataReaderImpl::get_latency_stats(
  OpenDDS::DCPS::LatencyStatisticsSeq & stats)
ACE_THROW_SPEC((CORBA::SystemException))
{
  stats.length(static_cast<CORBA::ULong>(this->statistics_.size()));
  int index = 0;

  for (StatsMapType::const_iterator current = this->statistics_.begin();
       current != this->statistics_.end();
       ++current, ++index) {
    stats[ index] = current->second.get_stats();
    stats[ index].publication = current->first;
  }
}

void
DataReaderImpl::reset_latency_stats()
ACE_THROW_SPEC((CORBA::SystemException))
{
  for (StatsMapType::iterator current = this->statistics_.begin();
       current != this->statistics_.end();
       ++current) {
    current->second.reset_stats();
  }
}

CORBA::Boolean
DataReaderImpl::statistics_enabled()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return this->statistics_enabled_;
}

void
DataReaderImpl::statistics_enabled(
  CORBA::Boolean statistics_enabled)
ACE_THROW_SPEC((CORBA::SystemException))
{
  this->statistics_enabled_ = statistics_enabled;
}

SubscriptionInstance*
DataReaderImpl::get_handle_instance(DDS::InstanceHandle_t handle)
{
  SubscriptionInstanceMapType::iterator iter = instances_.find(handle);

  if (iter == instances_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DataReaderImpl::get_handle_instance: ")
               ACE_TEXT("lookup for 0x%x failed\n"),
               handle));
    return 0;
  } // if (0 != instances_.find(handle, instance))

  return iter->second;
}

DDS::InstanceHandle_t
DataReaderImpl::get_next_handle(const DDS::BuiltinTopicKey_t& key)
{
  if (this->is_bit()) {
    OpenDDS::DCPS::RepoId id = OpenDDS::DCPS::RepoIdBuilder::create();
    OpenDDS::DCPS::RepoIdBuilder builder(id);
    builder.from_BuiltinTopicKey(key);
    return this->participant_servant_->get_handle(id);

  } else {
    return this->participant_servant_->get_handle();
  }
}

void
DataReaderImpl::notify_subscription_disconnected(const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_disconnected",6);

  // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
  // is given to this DataReader then narrow() fails.
  DataReaderListener_var the_listener
  = DataReaderListener::_narrow(this->listener_.in());

  if (!CORBA::is_nil(the_listener.in())) {
    SubscriptionLostStatus status;

    // Since this callback may come after remove_association which removes
    // the writer from id_to_handle map, we can ignore this error.
    this->lookup_instance_handles(pubids, status.publication_handles);
    the_listener->on_subscription_disconnected(this->dr_local_objref_.in(),
                                                status);
  }
}

void
DataReaderImpl::notify_subscription_reconnected(const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_reconnected",6);

  if (!this->is_bit_) {
    // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
    // is given to this DataReader then narrow() fails.
    DataReaderListener_var the_listener
    = DataReaderListener::_narrow(this->listener_.in());

    if (!CORBA::is_nil(the_listener.in())) {
      SubscriptionLostStatus status;

      // If it's reconnected then the reader should be in id_to_handle map otherwise
      // log with an error.
      if (this->lookup_instance_handles(pubids, status.publication_handles) == false) {
        ACE_ERROR((LM_ERROR, "(%P|%t) DataReaderImpl::notify_subscription_reconnected: "
                   "lookup_instance_handles failed.\n"));
      }

      the_listener->on_subscription_reconnected(this->dr_local_objref_.in(),
                                                status);
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
    DataReaderListener_var the_listener
    = DataReaderListener::_narrow(this->listener_.in());

    if (!CORBA::is_nil(the_listener.in())) {
      SubscriptionLostStatus status;

      CORBA::ULong len = handles.length();
      status.publication_handles.length(len);

      for (CORBA::ULong i = 0; i < len; ++ i) {
        status.publication_handles[i] = handles[i];
      }

      the_listener->on_subscription_lost(this->dr_local_objref_.in(),
                                         status);
    }
  }
}

void
DataReaderImpl::notify_subscription_lost(const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_lost",6);

  // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
  // is given to this DataReader then narrow() fails.
  DataReaderListener_var the_listener
  = DataReaderListener::_narrow(this->listener_.in());

  if (!CORBA::is_nil(the_listener.in())) {
    SubscriptionLostStatus status;

    // Since this callback may come after remove_association which removes
    // the writer from id_to_handle map, we can ignore this error.
    this->lookup_instance_handles(pubids, status.publication_handles);
    the_listener->on_subscription_lost(this->dr_local_objref_.in(),
                                        status);
  }
}

void
DataReaderImpl::notify_connection_deleted()
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_connection_deleted",6);

  // Narrow to DDS::DCPS::DataWriterListener. If a DDS::DataWriterListener
  // is given to this DataWriter then narrow() fails.
  DataReaderListener_var the_listener = DataReaderListener::_narrow(this->listener_.in());

  if (!CORBA::is_nil(the_listener.in()))
    the_listener->on_connection_deleted(this->dr_local_objref_.in());
}

bool
DataReaderImpl::lookup_instance_handles(const WriterIdSeq& ids,
                                        DDS::InstanceHandleSeq & hdls)
{
  if (DCPS_debug_level > 9) {
    CORBA::ULong const size = ids.length();
    const char* separator = "";
    std::stringstream buffer;

    for (unsigned long i = 0; i < size; ++i) {
      buffer << separator << RepoIdConverter(ids[i]);
      separator = ", ";
    }

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::lookup_instance_handles: ")
               ACE_TEXT("searching for handles for writer Ids: %C.\n"),
               buffer.str().c_str()));
  }

  CORBA::ULong const num_wrts = ids.length();
  hdls.length(num_wrts);

  for (CORBA::ULong i = 0; i < num_wrts; ++i) {
    hdls[i] = this->participant_servant_->get_handle(ids[i]);
  }

  return true;
}

bool
DataReaderImpl::filter_sample(const DataSampleHeader& header)
{
  ACE_Time_Value now(ACE_OS::gettimeofday());

  // Expire historic data if QoS indicates VOLATILE.
  if (!always_get_history_ && header.historic_sample_
      && qos_.durability.kind == DDS::VOLATILE_DURABILITY_QOS) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataReaderImpl::filter_sample: ")
                 ACE_TEXT("Discarded historic data.\n")));
    }

    return true;  // Data filtered.
  }

  // The LIFESPAN_DURATION_FLAG is set when sample data is sent
  // with a non-default LIFESPAN duration value.
  if (header.lifespan_duration_) {
    // Finite lifespan.  Check if data has expired.

    DDS::Time_t const tmp = {
      header.source_timestamp_sec_ + header.lifespan_duration_sec_,
      header.source_timestamp_nanosec_ + header.lifespan_duration_nanosec_
    };

    // We assume that the publisher host's clock and subcriber host's
    // clock are synchronized (allowed by the spec).
    ACE_Time_Value const expiration_time(
      OpenDDS::DCPS::time_to_time_value(tmp));

    if (now >= expiration_time) {
      if (DCPS_debug_level >= 8) {
        ACE_Time_Value const diff(now - expiration_time);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("OpenDDS (%P|%t) Received data ")
                   ACE_TEXT("expired by %d seconds, %d microseconds.\n"),
                   diff.sec(),
                   diff.usec()));
      }

      return true;  // Data filtered.
    }
  }


  return false;
}

bool
DataReaderImpl::filter_instance(SubscriptionInstance* instance, 
                                const PublicationId& pubid)
{
  if (this->is_exclusive_ownership_) {

    WriterMapType::iterator iter = writers_.find(pubid);
    
    if (iter == writers_.end()) {
      if (DCPS_debug_level > 4) {
        // This may not be an error since it could happen that the sample
        // is delivered to the datareader after the write is dis-associated
        // with this datareader.
        RepoIdConverter reader_converter(subscription_id_);
        RepoIdConverter writer_converter(pubid);
        ACE_DEBUG((LM_DEBUG,
                  ACE_TEXT("(%P|%t) DataReaderImpl::filter_instance: ")
                  ACE_TEXT("reader %C is not associated with writer %C.\n"),
                  std::string(reader_converter).c_str(),
                  std::string(writer_converter).c_str()));
      }
      return true;
    }
    
    // Evaulate the owner of the instance if not selected and filter
    // current message if it's not from owner writer.
    if ( instance->instance_state_.get_owner () == GUID_UNKNOWN 
      || ! iter->second->owner_evaluated_) {
      bool is_owner = this->owner_manager_->select_owner (
                        instance->instance_handle_, 
                        iter->second->writer_id_, 
                        iter->second->writer_qos_.ownership_strength.value,
                        &instance->instance_state_);
      iter->second->owner_evaluated_ = true;
      if (! is_owner) {
        return true;
      }
    }
    else if (! (instance->instance_state_.get_owner () == pubid)) {
      return true;
    }
  }
  
  ACE_Time_Value now(ACE_OS::gettimeofday());

  // TIME_BASED_FILTER processing; expire data samples
  // if minimum separation is not met for instance.
  const DDS::Duration_t zero = { DDS::DURATION_ZERO_SEC, DDS::DURATION_ZERO_NSEC };

  if (this->qos_.time_based_filter.minimum_separation > zero) {
    DDS::Duration_t separation =
      time_value_to_duration(now - instance->last_accepted_);

    if (separation < this->qos_.time_based_filter.minimum_separation) {
      return true;  // Data filtered.
    }
  }

  instance->last_accepted_ = now;

  return false;
}

bool DataReaderImpl::is_bit() const
{
  return this->is_bit_;
}

int
DataReaderImpl::num_zero_copies()
{
  int loans = 0;
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->sample_lock_,
                   1 /* assume we have loans */);

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter) {
    SubscriptionInstance *ptr = iter->second;

    for (OpenDDS::DCPS::ReceivedDataElement *item = ptr->rcvd_samples_.head_;
         item != 0; item = item->next_data_sample_) {
      loans += item->zero_copy_cnt_.value();
    }
  }

  return loans;
}

void DataReaderImpl::notify_liveliness_change()
{
  // N.B. writers_lock_ should already be acquired when
  //      this method is called.

  DDS::DataReaderListener* listener
  = listener_for(DDS::LIVELINESS_CHANGED_STATUS);

  if (listener != 0) {
    listener->on_liveliness_changed(dr_local_objref_.in(),
                                    liveliness_changed_status_);

    liveliness_changed_status_.alive_count_change = 0;
    liveliness_changed_status_.not_alive_count_change = 0;
  }

  notify_status_condition();

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << "subscription " << RepoIdConverter(subscription_id_);
    buffer << ", listener at: 0x" << std::hex << this->fast_listener_;

    for (WriterMapType::iterator current = this->writers_.begin();
         current != this->writers_.end();
         ++current) {
      RepoId id = current->first;
      buffer << std::endl << "\tNOTIFY: writer[ " << RepoIdConverter(id) << "] == ";
      buffer << current->second->get_state();
    }

    buffer << std::endl;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataReaderImpl::notify_liveliness_change: ")
               ACE_TEXT("listener at 0x%x, mask 0x%x.\n")
               ACE_TEXT("\tNOTIFY: %C\n"),
               listener,
               listener_mask_,
               buffer.str().c_str()));
  }
}

void DataReaderImpl::post_read_or_take()
{
  set_status_changed_flag(DDS::DATA_AVAILABLE_STATUS, false);
  get_subscriber_servant()->set_status_changed_flag(
    DDS::DATA_ON_READERS_STATUS, false);
}

void DataReaderImpl::reschedule_deadline()
{
  if (this->watchdog_.get() != 0) {
    for (SubscriptionInstanceMapType::iterator iter = this->instances_.begin();
         iter != this->instances_.end();
         ++iter) {
      if (iter->second->deadline_timer_id_ != -1) {
        if (this->watchdog_->reset_timer_interval(iter->second->deadline_timer_id_) == -1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t)DataReaderImpl::reschedule_deadline %p\n"),
                     ACE_TEXT("reset_timer_interval")));
        }
      }
    }
  }
}

ACE_Reactor*
DataReaderImpl::get_reactor()
{
  return this->reactor_;
}

OpenDDS::DCPS::RepoId
DataReaderImpl::get_topic_id()
{
  return this->topic_servant_->get_id();
}

OpenDDS::DCPS::RepoId
DataReaderImpl::get_dp_id()
{
  return this->participant_servant_->get_id();
}

void
DataReaderImpl::get_instance_handles(InstanceHandleVec& instance_handles)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

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
                                            iter->second->get_state()));
  }
}

void 
DataReaderImpl::update_ownership_strength (const PublicationId& pub_id,
                                  const CORBA::Long& ownership_strength)
{
  ACE_READ_GUARD(ACE_RW_Thread_Mutex,
    read_guard,
    this->writers_lock_);
  for (WriterMapType::iterator iter = writers_.begin();
    iter != writers_.end();
    ++iter) {
      if (iter->second->writer_id_ == pub_id) {
        if (ownership_strength != iter->second->writer_qos_.ownership_strength.value) {
          if (DCPS_debug_level >= 1) {
            RepoIdConverter reader_converter(this->subscription_id_);
            RepoIdConverter writer_converter(pub_id);
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataReaderImpl::update_ownership_strength - ")
              ACE_TEXT("local %C update remote %C strength from %d to %d \n"),
              std::string(reader_converter).c_str(),
              std::string(writer_converter).c_str(),
              iter->second->writer_qos_.ownership_strength, ownership_strength));
            }
            iter->second->writer_qos_.ownership_strength.value = ownership_strength;
            iter->second->owner_evaluated_ = false;
          }
        break;
      }       
  }
}

} // namespace DCPS
} // namespace OpenDDS

ostream& operator<<(ostream& str, OpenDDS::DCPS::WriterInfo::WriterState value)
{
  switch (value) {
  case OpenDDS::DCPS::WriterInfo::NOT_SET:
    return str << "NOT_SET";
  case OpenDDS::DCPS::WriterInfo::ALIVE:
    return str << "ALIVE";
  case OpenDDS::DCPS::WriterInfo::DEAD:
    return str << "DEAD";
  default:
    return str << "UNSPECIFIED(" << int(value) << ")";
  }
}
