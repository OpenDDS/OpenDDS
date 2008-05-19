// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "RequestedDeadlineWatchdog.h"
#include "Qos_Helper.h"


OpenDDS::DCPS::RequestedDeadlineWatchdog::RequestedDeadlineWatchdog (
  ACE_Reactor * reactor,
  OpenDDS::DCPS::RequestedDeadlineWatchdog::lock_type & lock,
  ::DDS::DeadlineQosPolicy qos,
  ::DDS::DataReaderListener_ptr listener,
  ::DDS::DataReader_ptr reader,
  ::DDS::RequestedDeadlineMissedStatus & status,
  CORBA::Long & last_total_count)
  : Watchdog (reactor,
              duration_to_time_value (qos.period))
  , lock_ (lock)
  , reverse_lock_ (lock)
  , signaled_ (false)
  , listener_ (::DDS::DataReaderListener::_duplicate (listener))
  , reader_ (::DDS::DataReader::_duplicate (reader))
  , status_ (status)
  , last_total_count_ (last_total_count)
{
}

OpenDDS::DCPS::RequestedDeadlineWatchdog::~RequestedDeadlineWatchdog ()
{
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::execute ()
{
  ACE_GUARD (ACE_Recursive_Thread_Mutex, monitor, this->lock_);

  if (!this->signaled_)
  {
    ++this->status_.total_count;
    this->status_.total_count_change =
      this->status_.total_count - this->last_total_count_;

    // Copy before releasing the lock.
    ::DDS::RequestedDeadlineMissedStatus const status = this->status_;

    // Release the lock during the upcall.

    ACE_GUARD (reverse_lock_type, reverse_monitor, this->reverse_lock_);

    if (!CORBA::is_nil (this->listener_.in ()))
    {
      // @todo Will this operation ever throw?  If so we may want to
      //       catch all exceptions, and act accordingly.
      this->listener_->on_requested_deadline_missed (this->reader_.in (),
                                                     status);
    }
  }
  else
  {
    // Require another signal the next time around.
    this->signaled_ = false;
  }
}

void
OpenDDS::DCPS::RequestedDeadlineWatchdog::signal ()
{
  ACE_GUARD (ACE_Recursive_Thread_Mutex, monitor, this->lock_);

  this->signaled_ = true;
}
