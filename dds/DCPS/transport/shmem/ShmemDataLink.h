/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMDATALINK_H
#define OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMDATALINK_H

#include "Shmem_Export.h"
#include "ShmemAllocator.h"
#include "ShmemReceiveStrategy.h"
#include "ShmemReceiveStrategy_rch.h"
#include "ShmemSendStrategy.h"
#include "ShmemSendStrategy_rch.h"
#include "ShmemTransport_rch.h"

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/PeriodicTask.h>
#include <dds/DCPS/transport/framework/DataLink.h>

#include <string>
#include <set>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;

struct ShmemData {
  enum Status {
    Free = 0,
    InUse = 1,
    RecvDone = 2,
    EndOfAlloc = -1
  };

  /*
   * This is ACE_INT8 instead of Status to try to make the in-memory
   * representation independent of compiler/implementation decisions. We can't
   * guarantee that two processes running code built with different compilers
   * can communicate over shmem, but we'll try to support it when possible.
   */
  ACE_INT8 status_;
  char transport_header_[TRANSPORT_HDR_SERIALIZED_SZ];
  ACE_Based_Pointer_Basic<char> payload_;
};

class OpenDDS_Shmem_Export ShmemDataLink
  : public DataLink {
public:
  ShmemDataLink(const ShmemTransport_rch& transport);

  bool open(const std::string& peer_address);

  int make_reservation(const GUID_t& remote_pub,
                       const GUID_t& local_sub,
                       const TransportReceiveListener_wrch& receive_listener,
                       bool reliable);

  void request_ack_received(ReceivedDataSample& sample);
  void stop_resend_association_msgs(const GUID_t& local, const GUID_t& remote);

  void control_received(ReceivedDataSample& sample);

  std::string local_address();
  std::string peer_address();
  pid_t peer_pid();

  ShmemAllocator* local_allocator();
  ShmemAllocator* peer_allocator();

  void read() { recv_strategy_->read(); }
  void signal_semaphore();
  ShmemTransport_rch transport() const;
  ShmemInst_rch config() const;

protected:
  ShmemSendStrategy_rch send_strategy_;
  ShmemReceiveStrategy_rch recv_strategy_;

  virtual void stop_i();

private:
  void send_association_msg(const GUID_t& local, const GUID_t& remote);
  void resend_association_msgs(const MonotonicTimePoint& now);

  std::string peer_address_;
  ShmemAllocator* peer_alloc_;
  ACE_Thread_Mutex peer_alloc_mutex_;
  ReactorTask_rch reactor_task_;

  ACE_Thread_Mutex assoc_resends_mutex_;
  typedef std::set<GuidPair> AssocResends;
  AssocResends assoc_resends_;

  // This is essentially PmfPeriodicTask<ShmemDataLink>, but using that
  // directly was causing warnings on MSVC x86.  There is only one member
  // function that's used with a PeriodicTask.
  struct PeriodicAssocResend : DCPS::PeriodicTask {
    PeriodicAssocResend(DCPS::ReactorTask_rch reactor_task, const ShmemDataLink& delegate)
      : PeriodicTask(reactor_task)
      , delegate_(delegate)
    {}

    void execute(const MonotonicTimePoint& now)
    {
      const DCPS::RcHandle<ShmemDataLink> handle = delegate_.lock();
      if (handle) {
        handle->resend_association_msgs(now);
      }
    }

    const DCPS::WeakRcHandle<ShmemDataLink> delegate_;
  };

  DCPS::RcHandle<PeriodicAssocResend> assoc_resends_task_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* OPENDDS_SHMEMDATALINK_H */
