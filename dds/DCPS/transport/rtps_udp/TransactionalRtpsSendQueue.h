/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_TRANSACTIONALRTPSSENDQUEUE_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_TRANSACTIONALRTPSSENDQUEUE_H

#include "Rtps_Udp_Export.h"
#include "dds/Versioned_Namespace.h"

#include "MetaSubmessage.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ThreadStatusManager;

/**
* A Transactional Send Queue Class For MetaSubmessages
*
* This class is designed to collect submessages from various threads
* in a transactional way so they can be more efficiently bundled.
*/
class OpenDDS_Rtps_Udp_Export TransactionalRtpsSendQueue {
public:
  TransactionalRtpsSendQueue(ThreadStatusManager& thread_status_manager);

  /// Add a single submessage to the queue
  /// Returns true if the queue was empty.
  bool enqueue(const MetaSubmessage& vec);

  /// Add a vector of submessages to the queue
  /// Returns true if the queue was empty and now not empty.
  bool enqueue(const MetaSubmessageVec& vec);

  /// Signal that a thread is beginning to send a sequence of submessages.
  void begin_transaction();

  /// Signal that a thread is ending a sequence of submessages.
  /// Returns true if the queue is non-empty.
  bool end_transaction();

  /// Return the number of active transactions.
  size_t active_transaction_count() const;

  /// Swap the queue into the given vector.
  /// Note: calls clear() on the input vector before swapping
  void swap(MetaSubmessageVec& vec);

  /// Remove all queued submessage with the given source and destination
  void purge(const RepoId& local, const RepoId& remote);

  /// Remove all queued submessage with the given destination (dst_guid_)
  void purge_remote(const RepoId& id);

  /// Remove all queued submessage with the given source (src_guid_)
  void purge_local(const RepoId& id);

private:

  mutable ACE_Thread_Mutex mutex_;
  ConditionVariable<ACE_Thread_Mutex> condition_variable_;
  size_t active_transaction_count_;

  MetaSubmessageVec queue_;
  ThreadStatusManager& thread_status_manager_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_TRANSACTIONALRTPSSENDQUEUE_H */
