/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_OFFERED_DEADLINE_WATCHDOG_H
#define OPENDDS_OFFERED_DEADLINE_WATCHDOG_H

#include "dds/DdsDcpsPublicationC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/Watchdog.h"
#include "dds/DCPS/PublicationInstance.h"

#include "ace/Reverse_Lock_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataWriterImpl;

/**
 * @class OfferedDeadlineWatchdog
 *
 * @brief Watchdog responsible calling the @c DataWriterListener
 *        when the deadline period expires.
 *
 * This watchdog object calls the
 * @c on_offered_deadline_missed() listener callback when the
 * configured finite deadline period expires.
 */
class OfferedDeadlineWatchdog : public Watchdog {
public:

  typedef ACE_Recursive_Thread_Mutex  lock_type;
  typedef ACE_Reverse_Lock<lock_type> reverse_lock_type;

  OfferedDeadlineWatchdog(
    lock_type & lock,
    DDS::DeadlineQosPolicy qos,
    OpenDDS::DCPS::DataWriterImpl * writer_impl,
    DDS::DataWriter_ptr writer,
    DDS::OfferedDeadlineMissedStatus & status,
    CORBA::Long & last_total_count);

  virtual ~OfferedDeadlineWatchdog();


  virtual int handle_timeout(const ACE_Time_Value&, const void* act);


  /// Operation to be executed when the associated timer expires.
  /**
   * This @c Watchdog object updates the
   * @c DDS::OfferedDeadlineMissed structure, and calls
   * @c DataWriterListener::on_requested_deadline_missed().
   */
  void execute(PublicationInstance_rch instance, bool timer_called);

  // Schedule timer for the supplied instance.
  void schedule_timer(PublicationInstance_rch instance);

  // Cancel timer for the supplied instance.
  void cancel_timer(PublicationInstance_rch instance);

  /// Re-schedule timer for all instances of the DataWriter.
  virtual void reschedule_deadline();

private:

  /// Lock for synchronization of @c status_ member.
  lock_type & status_lock_;
  /// Reverse lock used for releasing the @c status_lock_ listener upcall.
  reverse_lock_type reverse_status_lock_;

  /// Pointer to the @c DataWriterImpl object from which the
  /// @c DataWriterListener is obtained.
  OpenDDS::DCPS::DataWriterImpl * const writer_impl_;

  /// Reference to DataWriter passed to listener when the deadline
  /// expires.
  DDS::DataWriter_var writer_;

  /// Reference to the missed requested deadline status
  /// structure.
  DDS::OfferedDeadlineMissedStatus & status_;

  /// Last total_count when status was last checked.
  CORBA::Long & last_total_count_;

};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_OFFERED_DEADLINE_WATCHDOG_H */
