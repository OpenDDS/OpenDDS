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

/**
* A Transactional Send Queue Class For MetaSubmessages
*
* This class is designed to collect submessages from various threads
* in a transactional way so they can be more efficiently bundled.
*/
class OpenDDS_Rtps_Udp_Export TransactionalRtpsSendQueue {
public:
  TransactionalRtpsSendQueue();

  /// Add a single submessage to the queue
  /// Returns true if the queue was empty.
  bool enqueue(const MetaSubmessage& ms);

  /// Add a vector of submessages to the queue
  /// Returns true if the queue was empty and now not empty.
  bool enqueue(const MetaSubmessageVec& vec);

  /// Signal that a thread is beginning to send a sequence of submessages.
  void begin_transaction();

  /// Indicate that the queue is ready to send after all pending transactions are complete.
  void ready_to_send();

  /// Signal that a thread is ending a sequence of submessages.
  /// This method will swap the provided vec with the pending queue if the queue is ready to send.
  void end_transaction(MetaSubmessageVec& vec);

  /// Mark all queued submessage with the given source and destination as ignored.
  void ignore(const RepoId& local, const RepoId& remote);

  /// Mark all queued submessage with the given destination (dst_guid_) as ignored.
  void ignore_remote(const RepoId& id);

  /// Mark all queued submessage with the given source (src_guid_) as ignored.
  void ignore_local(const RepoId& id);

private:

  mutable ACE_Thread_Mutex mutex_;

  MetaSubmessageVec queue_;
  bool ready_to_send_;
  size_t active_transaction_count_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_TRANSACTIONALRTPSSENDQUEUE_H */
