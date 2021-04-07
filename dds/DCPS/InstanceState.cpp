/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"
#include "InstanceState.h"
#include "DataReaderImpl.h"
#include "SubscriptionInstance.h"
#include "ReceivedDataElementList.h"
#include "Time_Helper.h"
#include "DomainParticipantImpl.h"
#include "GuidConverter.h"

#if !defined (__ACE_INLINE__)
# include "InstanceState.inl"
#endif /* !__ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

InstanceState::InstanceState(DataReaderImpl* reader,
                             ACE_Recursive_Thread_Mutex& lock,
                             DDS::InstanceHandle_t handle)
  : ReactorInterceptor(TheServiceParticipant->reactor(),
                       TheServiceParticipant->reactor_owner()),
    lock_(lock),
    instance_state_(0),
    view_state_(0),
    disposed_generation_count_(0),
    no_writers_generation_count_(0),
    empty_(true),
    release_pending_(false),
    release_timer_id_(-1),
    reader_(*reader),
    handle_(handle),
    owner_(GUID_UNKNOWN),
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    exclusive_(reader->qos_.ownership.kind == DDS::EXCLUSIVE_OWNERSHIP_QOS),
#endif
    registered_(false)
{}

InstanceState::~InstanceState()
{
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (registered_) {
    RcHandle<DataReaderImpl> reader = reader_.lock();
    if (reader) {
      DataReaderImpl::OwnershipManagerPtr om = reader->ownership_manager();
      if (om) om->remove_instance(this);
    }
  }
#endif
}

void InstanceState::sample_info(DDS::SampleInfo& si, const ReceivedDataElement* de)
{
  si.sample_state = de->sample_state_;
  si.view_state = view_state_;
  si.instance_state = instance_state_;
  si.disposed_generation_count =
    static_cast<CORBA::Long>(disposed_generation_count_);
  si.no_writers_generation_count =
    static_cast<CORBA::Long>(no_writers_generation_count_);
  si.source_timestamp = de->source_timestamp_;
  si.instance_handle = handle_;
  RcHandle<DataReaderImpl> reader = reader_.lock();
  if (reader) {
    RcHandle<DomainParticipantImpl> participant = reader->participant_servant_.lock();
    si.publication_handle = participant ? participant->id_to_handle(de->pub_) : DDS::HANDLE_NIL;
  } else {
    si.publication_handle = DDS::HANDLE_NIL;
  }
  si.valid_data = de->valid_data_;
  /*
   * These are actually calculated later...
   */
  si.sample_rank = 0;

  // these aren't the real value, they're being saved
  // for a later calculation. the actual value is
  // calculated in DataReaderImpl::sample_info using
  // these values.
  si.generation_rank =
    static_cast<CORBA::Long>(de->disposed_generation_count_ +
                             de->no_writers_generation_count_);
  si.absolute_generation_rank =
    static_cast<CORBA::Long>(de->disposed_generation_count_ +
                             de->no_writers_generation_count_);

  si.opendds_reserved_publication_seq = de->sequence_.getValue();
}

// cannot ACE_INLINE because of #include loop

int InstanceState::handle_timeout(const ACE_Time_Value&, const void*)
{
  if (DCPS_debug_level) {
    ACE_DEBUG((LM_NOTICE,
               ACE_TEXT("(%P|%t) NOTICE:")
               ACE_TEXT(" InstanceState::handle_timeout:")
               ACE_TEXT(" autopurging samples with instance handle 0x%x!\n"),
               handle_));
  }
  release();

  return 0;
}

bool InstanceState::dispose_was_received(const PublicationId& writer_id)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
  writers_.erase(writer_id);

  //
  // Manage the instance state on disposal here.
  //
  // If disposed by owner then the owner is not re-elected, it can
  // resume if the writer sends message again.
  if (instance_state_ & DDS::ALIVE_INSTANCE_STATE) {
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    RcHandle<DataReaderImpl> reader = reader_.lock();
    if (reader) {
      DataReaderImpl::OwnershipManagerPtr owner_manager = reader->ownership_manager();
      if (! exclusive_
        || (owner_manager && owner_manager->is_owner (handle_, writer_id))) {
#endif
        instance_state_ = DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE;
        schedule_release();
        return true;
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      }
    }
#endif
  }

  return false;
}

bool InstanceState::unregister_was_received(const PublicationId& writer_id)
{
  if (DCPS_debug_level > 1) {
    GuidConverter conv(writer_id);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) InstanceState::unregister_was_received on %C\n"),
      OPENDDS_STRING(conv).c_str()
    ));
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
  writers_.erase(writer_id);
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (exclusive_) {
    // If unregistered by owner then the ownership should be transferred to another
    // writer.
    RcHandle<DataReaderImpl> reader = reader_.lock();
    if (reader) {
      DataReaderImpl::OwnershipManagerPtr owner_manager = reader->ownership_manager();
      if (owner_manager)
        owner_manager->remove_writer (handle_, writer_id);
    }
  }
#endif

  if (writers_.empty() && (instance_state_ & DDS::ALIVE_INSTANCE_STATE)) {
    instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
    schedule_release();
    return true;
  }

  return false;
}

void InstanceState::writer_became_dead(const PublicationId& writer_id, int, const MonotonicTimePoint&)
{
  if (DCPS_debug_level > 1) {
    GuidConverter conv(writer_id);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) InstanceState::writer_became_dead on %C\n"),
      OPENDDS_STRING(conv).c_str()
    ));
  }

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, lock_);
  writers_.erase(writer_id);

  if (writers_.empty() && (instance_state_ & DDS::ALIVE_INSTANCE_STATE)) {
    instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
    schedule_release();
  }
}

void InstanceState::schedule_pending()
{
  release_pending_ = true;
}

void InstanceState::schedule_release()
{
  DDS::DataReaderQos qos;
  RcHandle<DataReaderImpl> reader = reader_.lock();
  if (reader) {
    reader->get_qos(qos);
  } else {
    cancel_release();
    return;
  }

  DDS::Duration_t delay;

  switch (instance_state_) {
  case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
    delay = qos.reader_data_lifecycle.autopurge_nowriter_samples_delay;
    break;

  case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    delay = qos.reader_data_lifecycle.autopurge_disposed_samples_delay;
    break;

  default:
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: InstanceState::schedule_release:")
               ACE_TEXT(" Unsupported instance state: %d!\n"),
               instance_state_));
    return;
  }

  if (delay.sec != DDS::DURATION_INFINITE_SEC &&
      delay.nanosec != DDS::DURATION_INFINITE_NSEC) {

    execute_or_enqueue(new ScheduleCommand(this, TimeDuration(delay)));

  } else {
    // N.B. instance transitions are always followed by a non-valid
    // sample being queued to the ReceivedDataElementList; marking
    // the release as pending prevents this sample from being lost
    // if all samples have been already removed from the instance.
    schedule_pending();
  }
}

void InstanceState::cancel_release()
{
  release_pending_ = false;
  execute_or_enqueue(new CancelCommand(this));
}

bool InstanceState::release_if_empty()
{
  bool released = false;
  if (empty_ && writers_.empty()) {
    release();
    released = true;
  } else {
    schedule_pending();
  }
  return released;
}

void InstanceState::release()
{
  RcHandle<DataReaderImpl> reader = reader_.lock();
  if (reader) {
    reader->release_instance(handle_);
  }
}

void InstanceState::set_owner(const PublicationId& owner)
{
  owner_ = owner;
}

PublicationId& InstanceState::get_owner()
{
  return owner_;
}

bool InstanceState::is_exclusive() const
{
  return exclusive_;
}

bool InstanceState::registered()
{
  const bool ret = registered_;
  registered_ = true;
  return ret;
}

void InstanceState::registered(bool flag)
{
  registered_ = flag;
}

void InstanceState::reset_ownership(DDS::InstanceHandle_t instance)
{
  owner_ = GUID_UNKNOWN;
  registered_ = false;

  RcHandle<DataReaderImpl> reader = reader_.lock();
  if (reader) {
    reader->reset_ownership(instance);
  }
}

bool InstanceState::most_recent_generation(ReceivedDataElement* item) const
{
  return item->disposed_generation_count_ == disposed_generation_count_
    && item->no_writers_generation_count_ == no_writers_generation_count_;
}

bool InstanceState::reactor_is_shut_down() const
{
  return TheServiceParticipant->is_shut_down();
}

void InstanceState::CancelCommand::execute()
{
  if (instance_state_->release_timer_id_ != -1) {
    instance_state_->reactor()->cancel_timer(instance_state_);
    instance_state_->release_timer_id_ = -1;
  }
}

void InstanceState::ScheduleCommand::execute()
{
  if (instance_state_->release_timer_id_ != -1) {
    instance_state_->reactor()->cancel_timer(instance_state_);
  }

  instance_state_->release_timer_id_ =
    instance_state_->reactor()->schedule_timer(instance_state_, 0, delay_.value());

  if (instance_state_->release_timer_id_ == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: InstanceState::ScheduleCommand::execute:")
               ACE_TEXT(" Unable to schedule timer!\n")));
  }
}

const char* InstanceState::instance_state_string(DDS::InstanceStateKind value)
{
  switch (value) {
  case DDS::ALIVE_INSTANCE_STATE:
    return "ALIVE_INSTANCE_STATE";
  case DDS::NOT_ALIVE_INSTANCE_STATE:
    return "NOT_ALIVE_INSTANCE_STATE";
  case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    return "NOT_ALIVE_DISPOSED_INSTANCE_STATE";
  case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
    return "NOT_ALIVE_NO_WRITERS_INSTANCE_STATE";
  case DDS::ANY_INSTANCE_STATE:
    return "ANY_INSTANCE_STATE";
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: InstanceState::instance_state_string: ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      value));

    return "Invalid instance state";
  }
}

OPENDDS_STRING InstanceState::instance_state_mask_string(DDS::InstanceStateMask mask)
{
  if (mask == DDS::ANY_INSTANCE_STATE) {
    return instance_state_string(DDS::ANY_INSTANCE_STATE);
  }
  if (mask == DDS::NOT_ALIVE_INSTANCE_STATE) {
    return instance_state_string(DDS::NOT_ALIVE_INSTANCE_STATE);
  }
  OPENDDS_STRING str;
  if (mask & DDS::ALIVE_INSTANCE_STATE) {
    str = instance_state_string(DDS::ALIVE_INSTANCE_STATE);
  }
  if (mask & DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
    if (!str.empty()) str += " | ";
    str += instance_state_string(DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  }
  if (mask & DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
    if (!str.empty()) str += " | ";
    str += instance_state_string(DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
  }
  return str;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
