// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "OfferedDeadlineWatchdog.h"
#include "DataWriterImpl.h"
#include "Qos_Helper.h"
#include "ace/Recursive_Thread_Mutex.h"


OpenDDS::DCPS::OfferedDeadlineWatchdog::OfferedDeadlineWatchdog (
  ACE_Reactor * reactor,
  OpenDDS::DCPS::OfferedDeadlineWatchdog::lock_type & lock,
  ::DDS::DeadlineQosPolicy qos,
  OpenDDS::DCPS::DataWriterImpl * writer_impl,
  ::DDS::DataWriter_ptr writer,
  ::DDS::OfferedDeadlineMissedStatus & status,
  CORBA::Long & last_total_count)
  : Watchdog (reactor,
              duration_to_time_value (qos.period))
  , lock_ (lock)
  , reverse_lock_ (lock)
  , signaled_ (false)
  , writer_impl_ (writer_impl)
  , writer_ (::DDS::DataWriter::_duplicate (writer))
  , status_ (status)
  , last_total_count_ (last_total_count)
{
}

OpenDDS::DCPS::OfferedDeadlineWatchdog::~OfferedDeadlineWatchdog ()
{
}

void
OpenDDS::DCPS::OfferedDeadlineWatchdog::execute ()
{
  ACE_GUARD (ACE_Recursive_Thread_Mutex, monitor, this->lock_);

  if (!this->signaled_)
  {
    ++this->status_.total_count;
    this->status_.total_count_change =
      this->status_.total_count - this->last_total_count_;

    this->writer_impl_->set_status_changed_flag (
      ::DDS::OFFERED_DEADLINE_MISSED_STATUS, true);

    ::DDS::DataWriterListener * const listener =
        this->writer_impl_->listener_for (
          ::DDS::OFFERED_DEADLINE_MISSED_STATUS);

    if (listener != 0)
    {
      // Copy before releasing the lock.
      ::DDS::OfferedDeadlineMissedStatus const status = this->status_;

      // Release the lock during the upcall.
      ACE_GUARD (reverse_lock_type, reverse_monitor, this->reverse_lock_);

      // @todo Will this operation ever throw?  If so we may want to
      //       catch all exceptions, and act accordingly.
      listener->on_offered_deadline_missed (this->writer_.in (),
                                            status);
    }
    this->writer_impl_->notify_status_condition ();
  }
  else
  {
    // Require another signal the next time around.
    this->signaled_ = false;
  }
}

void
OpenDDS::DCPS::OfferedDeadlineWatchdog::signal ()
{
  ACE_GUARD (ACE_Recursive_Thread_Mutex, monitor, this->lock_);

  this->signaled_ = true;
}
