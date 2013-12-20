/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PERCONNECTIONSYNCH_H
#define OPENDDS_DCPS_PERCONNECTIONSYNCH_H

#include "ThreadSynch.h"
#include "ace/Task.h"
#include "ace/Synch.h"
#include "ace/Condition_T.h"

namespace OpenDDS {
namespace DCPS {

class PerConnectionSynch : public ThreadSynch {
public:

  PerConnectionSynch(ThreadSynchResource* synch_resource);
  virtual ~PerConnectionSynch();

  virtual void work_available();
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "PerConnectionSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_THREADSYNCH_H */
