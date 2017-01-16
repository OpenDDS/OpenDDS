/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADSYNCHWORKER_H
#define OPENDDS_DCPS_THREADSYNCHWORKER_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/RcHandle_T.h"
#include "ace/Synch_Traits.h"
#include <cstddef>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ThreadSynchWorker
  : public RcObject<ACE_SYNCH_MUTEX> {
public:

  virtual ~ThreadSynchWorker();

  enum WorkOutcome {
    WORK_OUTCOME_MORE_TO_DO,
    WORK_OUTCOME_NO_MORE_TO_DO,
    WORK_OUTCOME_CLOGGED_RESOURCE,
    WORK_OUTCOME_BROKEN_RESOURCE
  };

  virtual WorkOutcome perform_work() = 0;

  /// Indicate that queued data is available to be sent.
  virtual void schedule_output();

  /// DataLink reference value for diagnostics.
  std::size_t id() const;

protected:

  ThreadSynchWorker( std::size_t id = 0);

private:
  std::size_t id_;
};

typedef RcHandle<ThreadSynchWorker> ThreadSynchWorker_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ThreadSynchWorker.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_THREADSYNCHWORKER_H */
