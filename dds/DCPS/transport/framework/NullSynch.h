/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NULLSYNCH_H
#define OPENDDS_DCPS_NULLSYNCH_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynch.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export NullSynch : public ThreadSynch {
public:

  NullSynch(ThreadSynchResource* resource);
  virtual ~NullSynch();

  virtual void work_available();
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "NullSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_NULLSYNCH_H */
