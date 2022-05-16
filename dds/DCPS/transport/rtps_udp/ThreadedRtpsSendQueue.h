/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_THREADEDRTPSSENDQUEUE_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_THREADEDRTPSSENDQUEUE_H

#include "Rtps_Udp_Export.h"

#include "RtpsSendQueue.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

class OpenDDS_Rtps_Udp_Export ThreadedRtpsSendQueue {
public:
  ThreadedRtpsSendQueue();

  bool enqueue(const MetaSubmessageVec& vec);

  void enable_thread_queue();
  bool disable_thread_queue();

  void condense_and_swap(MetaSubmessageVec& vec);

  void purge_remote(const RepoId& id);
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
