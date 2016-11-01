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
#include "Qos_Helper.h"
#include "DomainParticipantImpl.h"

#if !defined (__ACE_INLINE__)
# include "InstanceState.inl"
#endif /* !__ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::InstanceState::InstanceState(DataReaderImpl* reader,
                                            ACE_Recursive_Thread_Mutex& lock,
                                            DDS::InstanceHandle_t handle)
  : lock_(lock),
    instance_state_(0),
    view_state_(0),
    disposed_generation_count_(0),
    no_writers_generation_count_(0),
    empty_(true),
    release_pending_(false),
    release_timer_id_(-1),
    reader_(reader),
    handle_(handle),
    owner_(GUID_UNKNOWN),
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    exclusive_(reader->qos_.ownership.kind == ::DDS::EXCLUSIVE_OWNERSHIP_QOS),
#endif
    registered_ (false)
{}

OpenDDS::DCPS::InstanceState::~InstanceState()
{
  cancel_release();
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (registered_) {
    OwnershipManager* om = reader_->ownership_manager();
    if (om) om->remove_instance(this);
  }
#endif
}

void OpenDDS::DCPS::InstanceState::sample_info(DDS::SampleInfo& si,
                                               const ReceivedDataElement* de)
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
  si.publication_handle = this->reader_->participant_servant_->id_to_handle(de->pub_);
  si.valid_data = de->registered_data_ != 0;
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

int
OpenDDS::DCPS::InstanceState::handle_timeout(const ACE_Time_Value& /* current_time */,
                                             const void* /* arg */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_NOTICE,
               ACE_TEXT("(%P|%t) NOTICE:")
               ACE_TEXT(" InstanceState::handle_timeout:")
               ACE_TEXT(" autopurging samples with instance handle 0x%x!\n"),
               this->handle_));
  }
  this->release();

  return 0;
}

bool
OpenDDS::DCPS::InstanceState::dispose_was_received(const PublicationId& writer_id)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard, this->lock_, false);

  writers_.erase(writer_id);

  //
  // Manage the instance state on disposal here.
  //
  // If disposed by owner then the owner is not re-elected, it can
  // resume if the writer sends message again.
  if (this->instance_state_ & DDS::ALIVE_INSTANCE_STATE) {
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    if (! this->exclusive_
      || this->reader_->owner_manager_->is_owner (this->handle_, writer_id)) {
#endif
      this->instance_state_ = DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE;
      schedule_release();
      return true;
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    }
#endif
  }

  return false;
}

bool
OpenDDS::DCPS::InstanceState::unregister_was_received(const PublicationId& writer_id)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard, this->lock_, false);
  writers_.erase(writer_id);
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (this->exclusive_) {
    // If unregistered by owner then the ownership should be transferred to another
    // writer.
    (void) this->reader_->owner_manager_->remove_writer (
             this->handle_, writer_id);
  }
#endif

  if (writers_.empty() && (this->instance_state_ & DDS::ALIVE_INSTANCE_STATE)) {
    this->instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
    schedule_release();
    return true;
  }

  return false;
}

void
OpenDDS::DCPS::InstanceState::writer_became_dead(
  const PublicationId&  writer_id,
  int                   /*num_alive_writers*/,
  const ACE_Time_Value& /* when */)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard, this->lock_);
  writers_.erase(writer_id);

  if (writers_.empty() && this->instance_state_ & DDS::ALIVE_INSTANCE_STATE) {
    this->instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
    schedule_release();
  }
}

void
OpenDDS::DCPS::InstanceState::schedule_pending()
{
  this->release_pending_ = true;
}

void
OpenDDS::DCPS::InstanceState::schedule_release()
{
  DDS::DataReaderQos qos;
  this->reader_->get_qos(qos);

  DDS::Duration_t delay;

  switch (this->instance_state_) {
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
               this->instance_state_));
    return;
  }

  if (delay.sec != DDS::DURATION_INFINITE_SEC &&
      delay.nanosec != DDS::DURATION_INFINITE_NSEC) {
    cancel_release();

    ACE_Reactor_Timer_Interface* reactor = this->reader_->get_reactor();

    this->release_timer_id_ =
      reactor->schedule_timer(this, 0, duration_to_time_value(delay));

    if (this->release_timer_id_ == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: InstanceState::schedule_release:")
                 ACE_TEXT(" Unable to schedule timer!\n")));
    }

  } else {
    // N.B. instance transitions are always followed by a non-valid
    // sample being queued to the ReceivedDataElementList; marking
    // the release as pending prevents this sample from being lost
    // if all samples have been already removed from the instance.
    schedule_pending();
  }
}

void
OpenDDS::DCPS::InstanceState::cancel_release()
{
  this->release_pending_ = false;

  if (this->release_timer_id_ != -1) {
    ACE_Reactor_Timer_Interface* reactor = this->reader_->get_reactor();
    reactor->cancel_timer(this->release_timer_id_);

    this->release_timer_id_ = -1;
  }
}

bool
OpenDDS::DCPS::InstanceState::release_if_empty()
{
  bool released = false;
  if (this->empty_ && this->writers_.empty()) {
    release();
    released = true;
  } else {
    schedule_pending();
  }
  return released;
}

void
OpenDDS::DCPS::InstanceState::release()
{
  this->reader_->release_instance(this->handle_);
}

void
OpenDDS::DCPS::InstanceState::set_owner (const PublicationId& owner)
{
  this->owner_ = owner;
}

OpenDDS::DCPS::PublicationId&
OpenDDS::DCPS::InstanceState::get_owner ()
{
  return this->owner_;
}

bool
OpenDDS::DCPS::InstanceState::is_exclusive () const
{
  return this->exclusive_;
}

bool
OpenDDS::DCPS::InstanceState::registered()
{
  bool ret = this->registered_;
  this->registered_ = true;
  return ret;
}

void
OpenDDS::DCPS::InstanceState::registered (bool flag)
{
  this->registered_ = flag;
}

void
OpenDDS::DCPS::InstanceState::reset_ownership (::DDS::InstanceHandle_t instance)
{
  this->owner_ = GUID_UNKNOWN;
  this->registered_ = false;

  this->reader_->reset_ownership(instance);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
