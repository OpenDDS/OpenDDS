/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_POOLSYNCH_H
#define OPENDDS_DCPS_POOLSYNCH_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynch.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class PoolSynchStrategy;
class ThreadSynchResource;

class OpenDDS_Dcps_Export PoolSynch : public ThreadSynch {
public:

  PoolSynch(PoolSynchStrategy* strategy,
            ThreadSynchResource* synch_resource);
  virtual ~PoolSynch();

  virtual void work_available();

protected:

  virtual void unregister_worker_i();

private:
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "PoolSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_POOLSYNCH_H */
