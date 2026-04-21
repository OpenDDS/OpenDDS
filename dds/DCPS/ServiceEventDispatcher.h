/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SERVICE_EVENT_DISPATCHER_H
#define OPENDDS_DCPS_SERVICE_EVENT_DISPATCHER_H

#include "EventDispatcher.h"
#include "DispatchService.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ServiceEventDispatcher : public EventDispatcher {
public:
  /**
   * Create a ServiceEventDispatcher
   *
   * ServiceEventDispatcher uses DispatchService as its execution engine.
   * Immediate events are accepted onto a FIFO queue. Scheduled events become
   * eligible at or after their expiration time and are then transferred onto
   * that same queue. With multiple worker threads, callback start and
   * completion order is not otherwise guaranteed.
   *
   * @param count the requested size of the internal thread pool (see
   * DispatchService); values less than 1 are treated as 1
   */
  explicit ServiceEventDispatcher(size_t count = 1);
  virtual ~ServiceEventDispatcher();

  /**
   * Request shutdown of this ServiceEventDispatcher.
   *
   * If immediate is false, already queued immediate events are allowed to
   * drain before shutdown completes. Still-scheduled timer events are canceled.
   *
   * If immediate is true, still-scheduled timer events and queued immediate
   * events that have not started are canceled.
   *
   * Events canceled by ServiceEventDispatcher have handle_cancel() invoked.
   * Callbacks already in progress are not interrupted by shutdown.
   */
  void shutdown(bool immediate = false);

  bool dispatch(EventBase_rch event);

  long schedule(EventBase_rch event, const MonotonicTimePoint& expiration = MonotonicTimePoint::now());

  size_t cancel(long id);

  size_t queue_size() const;

private:

  static void handle_cancel_and_release(EventBase* ptr);

  mutable ACE_Thread_Mutex mutex_;
  DispatchService_rch dispatcher_;
};
typedef RcHandle<ServiceEventDispatcher> ServiceEventDispatcher_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_SERVICE_EVENT_DISPATCHER_H
