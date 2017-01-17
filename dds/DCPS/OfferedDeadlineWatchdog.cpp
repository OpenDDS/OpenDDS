/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "OfferedDeadlineWatchdog.h"
#include "DataWriterImpl.h"
#include "Qos_Helper.h"
#include "ace/Recursive_Thread_Mutex.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::OfferedDeadlineWatchdog::OfferedDeadlineWatchdog(
  lock_type & lock,
  DDS::DeadlineQosPolicy qos,
  OpenDDS::DCPS::DataWriterImpl * writer_impl,
  DDS::DataWriter_ptr writer,
  DDS::OfferedDeadlineMissedStatus & status,
  CORBA::Long & last_total_count)
  : Watchdog(duration_to_time_value(qos.period))
  , status_lock_(lock)
  , reverse_status_lock_(status_lock_)
  , writer_impl_(writer_impl)
  , writer_(DDS::DataWriter::_duplicate(writer))
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
    instance->deadline_timer_id_ = Watchdog::schedule_timer(reinterpret_cast<const void*>(handle), this->interval_);
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
  OpenDDS::DCPS::PublicationInstance_rch instance =
    writer_impl_->get_handle_instance(handle);
  if (instance)
    execute(instance, true);
  return 0;
}


void
OpenDDS::DCPS::OfferedDeadlineWatchdog::execute(OpenDDS::DCPS::PublicationInstance_rch instance, bool timer_called)
{
  if (instance->deadline_timer_id_ != -1) {
    bool missed = false;

    if (instance->cur_sample_tv_  == ACE_Time_Value::zero) { // not write any sample.
      missed = true;

    } else if (timer_called) { // handle_timeout is called
      ACE_Time_Value diff = ACE_OS::gettimeofday() - instance->cur_sample_tv_;
      missed = diff >= this->interval_;

    } else if (instance->last_sample_tv_ != ACE_Time_Value::zero) { // upon writing sample.
      ACE_Time_Value diff = instance->cur_sample_tv_ - instance->last_sample_tv_;
      missed = diff > this->interval_;
    }

    if (missed) {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, monitor, this->status_lock_);

      if (timer_called) {
        ++this->status_.total_count;
        this->status_.total_count_change =
          this->status_.total_count - this->last_total_count_;
        this->status_.last_instance_handle = instance->instance_handle_;

        this->writer_impl_->set_status_changed_flag(
          DDS::OFFERED_DEADLINE_MISSED_STATUS, true);

        DDS::DataWriterListener_var listener =
          this->writer_impl_->listener_for(
            DDS::OFFERED_DEADLINE_MISSED_STATUS);

        if (! CORBA::is_nil(listener.in())) {
          // Copy before releasing the lock.
          DDS::OfferedDeadlineMissedStatus const status = this->status_;

          // Release the lock during the upcall.
          ACE_GUARD(reverse_lock_type, reverse_monitor, this->reverse_status_lock_);

          // @todo Will this operation ever throw?  If so we may want to
          //       catch all exceptions, and act accordingly.
          listener->on_offered_deadline_missed(this->writer_.in(),
                                              status);
        }

        this->writer_impl_->notify_status_condition();
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
  this->writer_impl_->reschedule_deadline();
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
