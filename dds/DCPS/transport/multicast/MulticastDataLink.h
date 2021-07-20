/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTDATALINK_H
#define OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTDATALINK_H

#include "Multicast_Export.h"

#include "MulticastInst.h"
#include "MulticastSendStrategy.h"
#include "MulticastSendStrategy_rch.h"
#include "MulticastReceiveStrategy.h"
#include "MulticastReceiveStrategy_rch.h"
#include "MulticastSession_rch.h"
#include "MulticastSessionFactory.h"
#include "MulticastSessionFactory_rch.h"
#include "MulticastTransport.h"
#include "MulticastTypes.h"

#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/PoolAllocator.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/ReactorTask.h"
#include "dds/DCPS/transport/framework/TransportSendBuffer.h"

#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MulticastTransport;
typedef RcHandle<MulticastTransport> MulticastTransport_rch;

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  MulticastDataLink(MulticastTransport& transport,
                    const MulticastSessionFactory_rch& session_factory,
                    MulticastPeer local_peer,
                    MulticastInst& config,
                    const ReactorTask_rch& reactor_task,
                    bool is_active);
  virtual ~MulticastDataLink();

  MulticastTransport& transport();

  MulticastPeer local_peer() const;

  MulticastSendStrategy* send_strategy();

  MulticastReceiveStrategy* receive_strategy();

  SingleSendBuffer* send_buffer();

  MulticastInst& config();

  ReactorTask_rch reactor_task();
  ACE_Reactor* get_reactor();
  ACE_Proactor* get_proactor();

  ACE_SOCK_Dgram_Mcast& socket();

  bool join(const ACE_INET_Addr& group_address);

  MulticastSession_rch find_or_create_session(MulticastPeer remote_peer);
  MulticastSession_rch find_session(MulticastPeer remote_peer);

  bool check_header(const TransportHeader& header);
  bool check_header(const DataSampleHeader& header);
  void sample_received(ReceivedDataSample& sample);

  bool reassemble(ReceivedDataSample& data, const TransportHeader& header);

  int make_reservation(const RepoId& remote_publication_id,
                       const RepoId& local_subscription_id,
                       const TransportReceiveListener_wrch& receive_listener,
                       bool reliable);
  void release_reservations_i(const RepoId& remote_id,
                              const RepoId& local_id);

  void client_stop(const RepoId& localId);

private:

  MulticastSessionFactory_rch session_factory_;

  MulticastPeer local_peer_;

  ReactorTask_rch reactor_task_;

  MulticastSendStrategy_rch send_strategy_;
  MulticastReceiveStrategy_rch recv_strategy_;

  unique_ptr<SingleSendBuffer> send_buffer_;

  ACE_SOCK_Dgram_Mcast socket_;

  ACE_SYNCH_RECURSIVE_MUTEX session_lock_;

  typedef OPENDDS_MAP(MulticastPeer, MulticastSession_rch) MulticastSessionMap;
  MulticastSessionMap sessions_;

  virtual void stop_i();

  void syn_received_no_session(MulticastPeer source, const Message_Block_Ptr& data,
                               bool swap_bytes);

  void release_remote_i(const RepoId& remote);
  RepoIdSet readers_selected_, readers_withheld_;
  bool ready_to_deliver(const ReceivedDataSample& data);
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTDATALINK_H */
