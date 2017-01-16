/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_REQUESTED_DEADLINE_WATCHDOG_H
#define OPENDDS_REQUESTED_DEADLINE_WATCHDOG_H

#include "dds/DdsDcpsSubscriptionC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/Watchdog.h"
#include "dds/DCPS/SubscriptionInstance.h"

#include "ace/Reverse_Lock_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataReaderImpl;

/**
 * @class RequestedDeadlineWatchdog
 *
 * @brief Watchdog responsible calling the @c DataReaderListener
 *        when the deadline period expires.
 *
 * This watchdog object calls the
 * @c on_requested_deadline_missed() listener callback when the
 * configured finite deadline period expires.
 */
class RequestedDeadlineWatchdog : public Watchdog {
public:

  typedef ACE_Recursive_Thread_Mutex  lock_type;
  typedef ACE_Reverse_Lock<lock_type> reverse_lock_type;

  RequestedDeadlineWatchdog(
    lock_type & lock,
    DDS::DeadlineQosPolicy qos,
    OpenDDS::DCPS::DataReaderImpl * reader_impl,
    DDS::DataReader_ptr reader,
    DDS::RequestedDeadlineMissedStatus & status,
    CORBA::Long & last_total_count);

  virtual ~RequestedDeadlineWatchdog();

  // Schedule timer for the supplied instance.
  void schedule_timer(OpenDDS::DCPS::SubscriptionInstance_rch instance);

  // Cancel timer for the supplied instance.
  void cancel_timer(OpenDDS::DCPS::SubscriptionInstance_rch instance);

  virtual int handle_timeout(const ACE_Time_Value&, const void* act);

  /// Operation to be executed when the associated timer expires.
  /**
   * This @c Watchdog object updates the
   * @c DDS::RequestedDeadlineMissed structure, and calls
   * @c DataReaderListener::on_requested_deadline_missed().
   */
  void execute(OpenDDS::DCPS::SubscriptionInstance_rch, bool timer_called);

  /// Re-schedule timer for all instances of the DataReader.
  virtual void reschedule_deadline();

private:

  /// Lock for synchronization of @c status_ member.
  lock_type & status_lock_;
  /// Reverse lock used for releasing the @c status_lock_ listener upcall.
  reverse_lock_type reverse_status_lock_;

  /// Pointer to the @c DataReaderImpl object from which the
  /// @c DataReaderListener is obtained.
  OpenDDS::DCPS::DataReaderImpl* const reader_impl_;

  /// Reference to DataReader passed to listener when the deadline
  /// expires.
  DDS::DataReader_var reader_;

  /// Reference to the missed requested deadline status
  /// structure.
  DDS::RequestedDeadlineMissedStatus& status_;

  /// Last total_count when status was last checked.
  CORBA::Long & last_total_count_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_REQUESTED_DEADLINE_WATCHDOG_H */
