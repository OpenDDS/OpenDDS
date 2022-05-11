/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSSENDQUEUE_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSSENDQUEUE_H

#include "Rtps_Udp_Export.h"

#include "MetaSubmessage.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

class OpenDDS_Rtps_Udp_Export RtpsSendQueue {
public:
  RtpsSendQueue();

  bool push_back(const MetaSubmessage& ms);
  bool merge(RtpsSendQueue& from);
  void condense_and_swap(MetaSubmessageVec& vec);

  void purge_remote(const RepoId& id);
  void purge_local(const RepoId& id);

  void enabled(bool enabled);
  bool enabled() const;

private:
  typedef std::pair<RepoId, RepoId> KeyType;
  typedef OPENDDS_MAP(KeyType, MetaSubmessage) MapType;
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
