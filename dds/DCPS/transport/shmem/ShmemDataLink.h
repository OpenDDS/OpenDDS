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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;

struct ShmemData {
  int status_;
  char transport_header_[TRANSPORT_HDR_SERIALIZED_SZ];
  ACE_Based_Pointer_Basic<char> payload_;
};

/**
 * values for ShmemData::status_
 */
enum {
  SHMEM_DATA_FREE = 0,
  SHMEM_DATA_IN_USE = 1,
  SHMEM_DATA_RECV_DONE = 2,
  SHMEM_DATA_END_OF_ALLOC = -1
};

class OpenDDS_Shmem_Export ShmemDataLink
  : public DataLink {
public:

  ShmemDataLink(const ShmemTransport_rch& transport);

  bool open(const std::string& peer_address);

  int make_reservation(const GUID_t& remote_sub, const GUID_t& local_pub,
    const TransportSendListener_wrch& send_listener, bool reliable);

  int make_reservation(const GUID_t& remote_pub,
                       const GUID_t& local_sub,
                       const TransportReceiveListener_wrch& receive_listener,
                       bool reliable);

  void request_ack_received(ReceivedDataSample& sample);

  void control_received(ReceivedDataSample& sample);

  std::string local_address();
  std::string peer_address();
  pid_t peer_pid();

  ShmemAllocator* local_allocator();
  ShmemAllocator* peer_allocator();

  void read() { recv_strategy_->read(); }
  void signal_semaphore();
  ShmemTransport_rch transport() const;
  ShmemInst& config() const;

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

  struct GuidPair {
    const GUID_t local;
    const GUID_t remote;

    GuidPair(const GUID_t& local, const GUID_t& remote)
    : local(local)
    , remote(remote)
    {
    }

    bool operator<(const GuidPair& other) const
    {
      return GUID_tKeyLessThan()(local, other.local) && GUID_tKeyLessThan()(remote, other.remote);
    }
  };
  ACE_Thread_Mutex assoc_resends_mutex_;
  typedef std::map<GuidPair, size_t> AssocResends;
  AssocResends assoc_resends_;
  typedef PmfPeriodicTask<ShmemDataLink> SmPeriodicTask;
  DCPS::RcHandle<SmPeriodicTask> assoc_resends_task_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* OPENDDS_SHMEMDATALINK_H */
