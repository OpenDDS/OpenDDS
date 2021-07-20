/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_THREADSYNCHRESOURCE_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_THREADSYNCHRESOURCE_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ThreadSynchResource {
public:

  virtual ~ThreadSynchResource();

  virtual int wait_to_unclog();
  void set_handle(ACE_HANDLE handle);

protected:

  virtual void notify_lost_on_backpressure_timeout() = 0;

  ThreadSynchResource();
  ACE_HANDLE handle_;
  TimeDuration* timeout_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ThreadSynchResource.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_THREADSYNCHRESOURCE_H */
