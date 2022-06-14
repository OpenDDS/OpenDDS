/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DISPATCHSERVICEEVENTDISPATCHER_H
#define OPENDDS_DCPS_DISPATCHSERVICEEVENTDISPATCHER_H

#include "EventDispatcher.h"
#include "DispatchService.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

class OpenDDS_Dcps_Export ServiceEventDispatcher : public EventDispatcher
{
public:
  ServiceEventDispatcher(size_t count = 1);
  virtual ~ServiceEventDispatcher();

  void shutdown(bool immediate = false);

  bool dispatch(EventBase_rch event);

  long schedule(EventBase_rch event, const MonotonicTimePoint& expiration = MonotonicTimePoint::now());

  size_t cancel(long id);

private:

  mutable ACE_Thread_Mutex mutex_;
  DispatchService_rch dispatcher_;
};
typedef RcHandle<ServiceEventDispatcher> ServiceEventDispatcher_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_DISPATCHSERVICEEVENTDISPATCHER_H
