/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADSYNCH_H
#define OPENDDS_DCPS_THREADSYNCH_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynchWorker.h"
#include "dds/DCPS/PoolAllocationBase.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ThreadSynchResource;

/**
 * This class is the base class for different ThreadSynch stratege, currently
 * only the thread per connection strategy is implemented and used.
 *
 * Notes for object ownership:
 * 1) Pointer to TransportSendStrategy object (as ThreadSynchWorker) directly but not
 *    reference counted. It won't have access problem during it lifetime because the
 *    TransportSendStrategy object owns this ThreadSynch object.
 * 2) The ThreadSynch object is created by the ThreadSynchResource object and it owns
 *    the ThreadSynchResource object.
 */
class OpenDDS_Dcps_Export ThreadSynch : public PoolAllocationBase {
public:

  virtual ~ThreadSynch();

  /// The worker must introduce himself to this ThreadSynch object.
  /// It is the worker object that "owns" this ThreadSynch object.
  /// Returns 0 for success, -1 for failure.
  int register_worker(const ThreadSynchWorker_rch& worker);

  /// Our owner, the worker_, is breaking our relationship.
  void unregister_worker();

  /// The ThreadSynchWorker would like to have its perform_work() called
  /// from the appropriate thread once the ThreadSynchResource claims
  /// that it is_ready_for_work().
  virtual void work_available() = 0;

protected:

  // This ThreadSynch object takes ownership of the resource.
  ThreadSynch(ThreadSynchResource* resource);

  /// This will tell the worker_ to perform_work().
  /// A return value of 0 means that the perform_work() did all the
  /// work necessary, and we shouldn't ask it to perform_work() again,
  /// unless we receive another work_available() call.  It won't hurt
  /// if we call perform_work() and it has nothing to do - it will
  /// immediately return 0 to indicate it has nothing more to do (at
  /// the moment).
  /// A return value of 1 means that the perform_work() completed, and
  /// there is still more work it could do.  The perform_work() should
  /// be called again in this case.
  ThreadSynchWorker::WorkOutcome perform_work();

  int wait_on_clogged_resource();

  /// The default implementation is to do nothing here.  The
  /// subclass may override the implementation in order to do
  /// something when the worker registers.
  /// Returns 0 for success, -1 for failure.
  virtual int register_worker_i();

  /// The default implementation is to do nothing here.  The
  /// subclass may override the implementation in order to do
  /// something when the worker unregisters.
  virtual void unregister_worker_i();

  /// Access the worker implementation directly.
  ThreadSynchWorker* worker();

private:

  ThreadSynchWorker_rch worker_;
  ThreadSynchResource* resource_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ThreadSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_THREADSYNCH_H */
