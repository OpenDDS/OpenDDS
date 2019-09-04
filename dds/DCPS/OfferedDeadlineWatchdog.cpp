/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "OfferedDeadlineWatchdog.h"
#include "DataWriterImpl.h"
#include "Time_Helper.h"
#include "ace/Recursive_Thread_Mutex.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::OfferedDeadlineWatchdog::OfferedDeadlineWatchdog(
  lock_type & lock,
  DDS::DeadlineQosPolicy qos,
  OpenDDS::DCPS::DataWriterImpl & writer_impl,
  DDS::OfferedDeadlineMissedStatus & status,
  CORBA::Long & last_total_count)
  : Watchdog(TimeDuration(qos.period))
  , status_lock_(lock)
  , reverse_status_lock_(status_lock_)
  , writer_impl_(writer_impl)
  , status_(status)
  , last_total_count_(last_total_count)
{
}

OpenDDS::DCPS::OfferedDeadlineWatchdog::~OfferedDeadlineWatchdog()
{
}

void
OpenDDS::DCPS::OfferedDeadlineWatchdog::schedule_timer(OpenDDS::DCPS::PublicationInstance_rch instance)
{
  if (instance->deadline_timer_id_ == -1) {
    intptr_t handle = instance->instance_handle_;
    instance->deadline_timer_id_ = Watchdog::schedule_timer(reinterpret_cast<const void*>(handle), interval_);
  }
}

void
OpenDDS::DCPS::OfferedDeadlineWatchdog::cancel_timer(OpenDDS::DCPS::PublicationInstance_rch instance)
{
  if (instance->deadline_timer_id_ != -1) {
    Watchdog::cancel_timer(instance->deadline_timer_id_);
    instance->deadline_timer_id_ = -1;
  }
}

int
OpenDDS::DCPS::OfferedDeadlineWatchdog::handle_timeout(const ACE_Time_Value&, const void* act)
{
  DDS::InstanceHandle_t handle = static_cast<DDS::InstanceHandle_t>(reinterpret_cast<intptr_t>(act));
  RcHandle<DataWriterImpl> writer = writer_impl_.lock();
  if (writer) {
    OpenDDS::DCPS::PublicationInstance_rch instance =
      writer->get_handle_instance(handle);
    if (instance)
      execute(*writer, instance, true);
  }
  return 0;
}


void
OpenDDS::DCPS::OfferedDeadlineWatchdog::execute(
  DataWriterImpl& writer,
  PublicationInstance_rch instance,
  bool timer_called)
{
  if (instance->deadline_timer_id_ != -1) {
    bool missed = false;

    if (instance->cur_sample_tv_.is_zero()) { // not write any sample.
      missed = true;

    } else if (timer_called) { // handle_timeout is called
      missed = MonotonicTimePoint::now() - instance->cur_sample_tv_ >= interval_;

    } else if (!instance->last_sample_tv_.is_zero()) { // upon writing sample.
      missed = instance->cur_sample_tv_ - instance->last_sample_tv_ > interval_;
    }

    if (missed) {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, monitor, this->status_lock_);

      if (timer_called) {
        ++this->status_.total_count;
        this->status_.total_count_change =
          this->status_.total_count - this->last_total_count_;
        this->status_.last_instance_handle = instance->instance_handle_;

        writer.set_status_changed_flag(
          DDS::OFFERED_DEADLINE_MISSED_STATUS, true);

        DDS::DataWriterListener_var listener =
          writer.listener_for(
            DDS::OFFERED_DEADLINE_MISSED_STATUS);

        if (listener) {
          // Copy before releasing the lock.
          DDS::OfferedDeadlineMissedStatus const status = this->status_;

          // Release the lock during the upcall.
          ACE_GUARD(reverse_lock_type, reverse_monitor, this->reverse_status_lock_);

          // @todo Will this operation ever throw?  If so we may want to
          //       catch all exceptions, and act accordingly.
          listener->on_offered_deadline_missed(&writer, status);

          // We need to update the last total count value to our current total
          // so that the next time we will calculate the correct total_count_change;
          this->last_total_count_ = this->status_.total_count;
        }

        writer.notify_status_condition();
      }
    }

    // This next part is without status_lock_ held to avoid reactor deadlock.
    if (!timer_called) {
      this->cancel_timer(instance);
      this->schedule_timer(instance);
    }

  } else {
    // not an error - timer is scheduled asynchronously so we can get here
    // via WriteDataContainer::enqueue() before schedule_timer() is done
  }
}

void
OpenDDS::DCPS::OfferedDeadlineWatchdog::reschedule_deadline()
{
  RcHandle<DataWriterImpl> writer = writer_impl_.lock();
  if (writer)
    writer->reschedule_deadline();
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
