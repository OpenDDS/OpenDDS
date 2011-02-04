/*
 * $Id$
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

OpenDDS::DCPS::RequestedDeadlineWatchdog::RequestedDeadlineWatchdog(
  ACE_Reactor * reactor,
  lock_type & lock,
  DDS::DeadlineQosPolicy qos,
  OpenDDS::DCPS::DataReaderImpl * reader_impl,
  DDS::DataReader_ptr reader,
  DDS::RequestedDeadlineMissedStatus & status,
  CORBA::Long & last_total_count)
  : Watchdog(reactor,
               duration_to_time_value(qos.period))
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
  OpenDDS::DCPS::SubscriptionInstance* instance)
{
  if (instance->deadline_timer_id_ == -1) {
    instance->deadline_timer_id_ = Watchdog::schedule_timer((void*)instance, this->interval_);
  }
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::cancel_timer(
  OpenDDS::DCPS::SubscriptionInstance* instance)
{
  if (instance->deadline_timer_id_ != -1) {
    Watchdog::cancel_timer(instance->deadline_timer_id_);
    instance->deadline_timer_id_ = -1;
  }
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::execute(void const * act, bool timer_called)
{
  SubscriptionInstance * instance = (SubscriptionInstance *)act;

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

        DDS::DataReaderListener * const listener =
          this->reader_impl_->listener_for(
            DDS::REQUESTED_DEADLINE_MISSED_STATUS);

        if (instance->instance_state_.is_exclusive()) {
          reader_impl_->owner_manager_->remove_writers (instance->instance_handle_);
        }

        if (listener != 0) {
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

      if (!timer_called) {
        this->cancel_timer(instance);
        this->schedule_timer(instance);
      }
    }

  } else {
    ACE_ERROR((LM_ERROR, "(%P|%t) RequestedDeadlineWatchdog::execute: "
               "the current timer should not be invalid for instance %X\n",
               instance));
  }
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::reschedule_deadline()
{
  this->reader_impl_->reschedule_deadline();
}
