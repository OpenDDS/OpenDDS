/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADSYNCHWORKER_H
#define OPENDDS_DCPS_THREADSYNCHWORKER_H

#include "dds/DCPS/dcps_export.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ThreadSynchWorker {
public:

  virtual ~ThreadSynchWorker();

  enum WorkOutcome {
    WORK_OUTCOME_MORE_TO_DO,
    WORK_OUTCOME_NO_MORE_TO_DO,
    WORK_OUTCOME_ClOGGED_RESOURCE,
    WORK_OUTCOME_BROKEN_RESOURCE
  };

  virtual WorkOutcome perform_work() = 0;

protected:

  ThreadSynchWorker();
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "ThreadSynchWorker.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_THREADSYNCHWORKER_H */
