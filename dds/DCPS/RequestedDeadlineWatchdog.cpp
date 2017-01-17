/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "RequestedDeadlineWatchdog.h"
#include "DataReaderImpl.h"
#include "DomainParticipantImpl.h"
#include "Qos_Helper.h"

#include "ace/Recursive_Thread_Mutex.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::RequestedDeadlineWatchdog::RequestedDeadlineWatchdog(
  lock_type & lock,
  DDS::DeadlineQosPolicy qos,
  OpenDDS::DCPS::DataReaderImpl * reader_impl,
  DDS::DataReader_ptr reader,
  DDS::RequestedDeadlineMissedStatus & status,
  CORBA::Long & last_total_count)
  : Watchdog(duration_to_time_value(qos.period))
  , status_lock_(lock)
  , reverse_status_lock_(status_lock_)
  , reader_impl_(reader_impl)
  , reader_(DDS::DataReader::_duplicate(reader))
  , status_(status)
  , last_total_count_(last_total_count)
{
}

OpenDDS::DCPS::RequestedDeadlineWatchdog::~RequestedDeadlineWatchdog()
{
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::schedule_timer(
  OpenDDS::DCPS::SubscriptionInstance_rch instance)
{
  if (instance->deadline_timer_id_ == -1) {
    intptr_t handle = instance->instance_handle_;
    instance->deadline_timer_id_ = Watchdog::schedule_timer(reinterpret_cast<const void*>(handle), this->interval_);
  }
  if (instance->deadline_timer_id_ == -1) {
    ACE_ERROR((LM_ERROR,
               "ERROR Timer for instance %X should be scheduled, but is %d\n",
               instance.in(), instance->deadline_timer_id_));
  } else if (DCPS_debug_level > 5) {
    ACE_DEBUG((LM_INFO, "Timer for instance %X scheduled \n", instance.in()));
  }
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::cancel_timer(
  OpenDDS::DCPS::SubscriptionInstance_rch instance)
{
  if (instance->deadline_timer_id_ != -1) {
    Watchdog::cancel_timer(instance->deadline_timer_id_);
    instance->deadline_timer_id_ = -1;
    if (DCPS_debug_level > 5) {
      ACE_DEBUG((LM_INFO, "Timer for instance %X cancelled \n", instance.in()));
    }
  }
}

int
OpenDDS::DCPS::RequestedDeadlineWatchdog::handle_timeout(const ACE_Time_Value&, const void* act)
{
  DDS::InstanceHandle_t handle = static_cast<DDS::InstanceHandle_t>(reinterpret_cast<intptr_t>(act));
  SubscriptionInstance_rch instance = this->reader_impl_->get_handle_instance(handle);
  if (instance)
    execute(instance, true);
  return 0;
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::execute(SubscriptionInstance_rch instance, bool timer_called)
{
  if (instance->deadline_timer_id_ != -1) {
    bool missed = false;

    if (instance->cur_sample_tv_  == ACE_Time_Value::zero) { // not received any sample.
      missed = true;

    } else if (timer_called) { // handle_timeout is called
      ACE_Time_Value diff = ACE_OS::gettimeofday() - instance->cur_sample_tv_;
      missed = diff >= this->interval_;

    } else { // upon receiving sample.
      ACE_Time_Value diff = instance->cur_sample_tv_ - instance->last_sample_tv_;
      missed = diff > this->interval_;
    }

    if (missed) {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, monitor, this->status_lock_);
      // Only update the status upon timer is called and not
      // when receiving a sample after the interval.
      // Otherwise the counter is doubled.
      if (timer_called) {
        ++this->status_.total_count;
        this->status_.total_count_change =
          this->status_.total_count - this->last_total_count_;
        this->status_.last_instance_handle = instance->instance_handle_;

        this->reader_impl_->set_status_changed_flag(
          DDS::REQUESTED_DEADLINE_MISSED_STATUS, true);

        DDS::DataReaderListener_var listener =
          this->reader_impl_->listener_for(
            DDS::REQUESTED_DEADLINE_MISSED_STATUS);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
        if (instance->instance_state_.is_exclusive()) {
          reader_impl_->owner_manager_->remove_writers (instance->instance_handle_);
        }
#endif

        if (!CORBA::is_nil(listener.in())) {
          // Copy before releasing the lock.
          DDS::RequestedDeadlineMissedStatus const status = this->status_;

          // Release the lock during the upcall.
          ACE_GUARD(reverse_lock_type, reverse_monitor, this->reverse_status_lock_);
          // @todo Will this operation ever throw?  If so we may want to
          //       catch all exceptions, and act accordingly.
          listener->on_requested_deadline_missed(this->reader_.in(),
                                                status);
        }

        this->reader_impl_->notify_status_condition();
      }
    }

    // This next part is without status_lock_ held to avoid reactor deadlock.
    if (!timer_called) {
      this->cancel_timer(instance);
      this->schedule_timer(instance);
    }

  } else {
    // not an error - timer is scheduled asynchronously so we can get here
    // via DataReaderImpl::data_received() before schedule_timer() is done
  }
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::reschedule_deadline()
{
  this->reader_impl_->reschedule_deadline();
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
