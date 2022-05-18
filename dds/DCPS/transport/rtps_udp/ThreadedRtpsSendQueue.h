/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_THREADEDRTPSSENDQUEUE_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_THREADEDRTPSSENDQUEUE_H

#include "Rtps_Udp_Export.h"
#include "RtpsSendQueue.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
* A Threaded Send Queue Class For MetaSubmessages (c.f. RtpsSendQueue)
*
* This class is primarily designed to:
*  - Place the thread-aware send queue logic in a single class
*  - Allow for relatively effortless "per thread" send queues
*  - Reuse the HB / AN logic of RtpsSendQueue for both the primary and thread
*    queues
*/
class OpenDDS_Rtps_Udp_Export ThreadedRtpsSendQueue {
public:
  ThreadedRtpsSendQueue();

  /// Add a single submessage to the queue
  /// Returns true if the primary queue contains "sendable" submessages
  bool enqueue(const MetaSubmessage& vec);

  /// Add a vector of submessages to the queue
  /// Returns true if the primary queue contains "sendable" submessages
  bool enqueue(const MetaSubmessageVec& vec);

  /// Enable per-thread queueing for the current thread. Any submessages
  /// enqueued by the current thread will not enter the primary queue
  /// until the current thread calls disable_thread_queue()
  void enable_thread_queue();

  /// Merge per-thread send queue into primary queue and mark it as disabled
  /// Returns true if the primary queue contains "sendable" submessages
  bool disable_thread_queue();

  /// Push all primary HB and AN data into the primary queue vector and swap
  /// with the input vector
  /// Note: calls clear() on the input vector before swapping
  void condense_and_swap(MetaSubmessageVec& vec);

  /// Remove all queued submessage with the given destination (dst_guid_)
  void purge_remote(const RepoId& id);

  /// Remove all queued submessage with the given source (src_guid_)
  void purge_local(const RepoId& id);

private:

  mutable ACE_Thread_Mutex mutex_;

  typedef OPENDDS_MAP(ACE_thread_t, RtpsSendQueue) ThreadQueueMap;
  ThreadQueueMap thread_queue_map_;
  RtpsSendQueue primary_queue_;
  bool has_data_to_send_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_THREADEDRTPSSENDQUEUE_H */
