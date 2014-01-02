/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADSYNCHWORKER_H
#define OPENDDS_DCPS_THREADSYNCHWORKER_H

#include "dds/DCPS/dcps_export.h"

#include "ace/Event_Handler.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ThreadSynchWorker {
public:

  virtual ~ThreadSynchWorker();

  enum WorkOutcome {
    WORK_OUTCOME_MORE_TO_DO,
    WORK_OUTCOME_NO_MORE_TO_DO,
    WORK_OUTCOME_CLOGGED_RESOURCE,
    WORK_OUTCOME_BROKEN_RESOURCE
  };

  virtual WorkOutcome perform_work() = 0;

  /// @{ @name Reactor controls.
  /// These methods are used to pass through start and end events to a
  /// reactor if one is being used to manage sending activity.  A
  /// subclass would have a reference to the reactor to forward the
  /// events to.

  /// Indicate the start of when there is work to be done.
  virtual int schedule_wakeup( ACE_Reactor_Mask masks_to_be_added);

  /// Indicate that there is no more work to be done.
  virtual int cancel_wakeup( ACE_Reactor_Mask masks_to_be_cleared);

  /// @}

  /// DataLink reference value for diagnostics.
  std::size_t id() const;

protected:

  ThreadSynchWorker( std::size_t id = 0);

private:
  std::size_t id_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "ThreadSynchWorker.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_THREADSYNCHWORKER_H */
