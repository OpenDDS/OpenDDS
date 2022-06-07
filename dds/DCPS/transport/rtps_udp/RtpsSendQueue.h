/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSSENDQUEUE_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSSENDQUEUE_H

#include "Rtps_Udp_Export.h"
#include "MetaSubmessage.h"
#include "RoutedGuidPair.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
* A Send Queue Class For MetaSubmessages
*
* This class is primarily designed to:
*  - Place the send queue logic in a single class
*  - Avoid unnecessary STL allocations and deallocations by allowing the queue
*    (vector) capacities to grow and be swapped / cleared without the need for
*    an actual destructor / deallocation call
*  - Avoid sending "old" heartbeats and acknacks which may have been queued up
*    a "long" time ago due to congestion / multiple threads etc
*/
class OpenDDS_Rtps_Udp_Export RtpsSendQueue {
public:
  RtpsSendQueue();

  /// Add a single submessage to the queue
  bool push_back(const MetaSubmessage& ms);

  /// Merge another send queue into this one. Note: empties the 'merged' queue
  bool merge(RtpsSendQueue& from);

  /// Push all HB and AN data into the queue vector and swap with the input vector
  /// Note: calls clear() on the input vector before swapping
  void condense_and_swap(MetaSubmessageVec& vec);

  /// Remove all queued submessage with the given destination (dst_guid_)
  void purge_remote(const RepoId& id);

  /// Remove all queued submessage with the given source (src_guid_)
  void purge_local(const RepoId& id);

  /// Marks this queue as enabled (for external use, no internal effect)
  void enabled(bool enabled);

  /// Check if this queue is marked as enabled. Default is true
  bool enabled() const;

private:

  typedef RoutedGuidPair KeyType;
#if defined ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP(KeyType, MetaSubmessage) MapType;
#else
  typedef OPENDDS_MAP(KeyType, MetaSubmessage) MapType;
#endif
  MapType heartbeat_map_;
  MapType acknack_map_;
  MetaSubmessageVec queue_;
  bool enabled_;
  bool heartbeats_need_merge_;
  bool acknacks_need_merge_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSSENDQUEUE_H */
