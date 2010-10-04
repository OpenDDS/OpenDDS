/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "OfferedDeadlineWatchdog.h"
#include "DataWriterImpl.h"
#include "Qos_Helper.h"
#include "ace/Recursive_Thread_Mutex.h"

OpenDDS::DCPS::OfferedDeadlineWatchdog::OfferedDeadlineWatchdog(
  ACE_Reactor * reactor,
  lock_type & lock,
  DDS::DeadlineQosPolicy qos,
  OpenDDS::DCPS::DataWriterImpl * writer_impl,
  DDS::DataWriter_ptr writer,
  DDS::OfferedDeadlineMissedStatus & status,
  CORBA::Long & last_total_count)
  : Watchdog(reactor, duration_to_time_value(qos.period))
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
OpenDDS::DCPS::OfferedDeadlineWatchdog::schedule_timer(OpenDDS::DCPS::PublicationInstance* instance)
{
  if (instance->deadline_timer_id_ == -1) {
    instance->deadline_timer_id_ = Watchdog::schedule_timer((void*)instance, this->interval_);
  }
}

void
OpenDDS::DCPS::OfferedDeadlineWatchdog::cancel_timer(OpenDDS::DCPS::PublicationInstance* instance)
{
  if (instance->deadline_timer_id_ != -1) {
    Watchdog::cancel_timer(instance->deadline_timer_id_);
    instance->deadline_timer_id_ = -1;
  }
}

void
OpenDDS::DCPS::OfferedDeadlineWatchdog::execute(void const * act, bool timer_called)
{
  OpenDDS::DCPS::PublicationInstance * instance = (OpenDDS::DCPS::PublicationInstance *)act;

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

        DDS::DataWriterListener * const listener =
          this->writer_impl_->listener_for(
            DDS::OFFERED_DEADLINE_MISSED_STATUS);

        if (listener != 0) {
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
      if (!timer_called) {
        this->cancel_timer(instance);
        this->schedule_timer(instance);
      }
    }

  } else {
    ACE_ERROR((LM_ERROR, "(%P|%t)OfferedDeadlineWatchdog::execute: "
               "the current timer should not be invalid for instance %X\n",
               instance));
  }
}

void
OpenDDS::DCPS::OfferedDeadlineWatchdog::reschedule_deadline()
{
  this->writer_impl_->reschedule_deadline();
}
