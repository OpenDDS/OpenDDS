/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTDATALINK_H
#define DCPS_MULTICASTDATALINK_H

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

#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Synch_Traits.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportSendBuffer.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class MulticastTransport;

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  MulticastDataLink(MulticastTransport* transport,
                    MulticastSessionFactory* session_factory,
                    MulticastPeer local_peer,
                    bool is_active);
  virtual ~MulticastDataLink();

  MulticastTransport* transport();

  MulticastPeer local_peer() const;

  void configure(MulticastInst* config,
                 TransportReactorTask* reactor_task);

  void send_strategy(MulticastSendStrategy* send_strategy);
  MulticastSendStrategy* send_strategy();

  void receive_strategy(MulticastReceiveStrategy* recv_strategy);
  MulticastReceiveStrategy* receive_strategy();

  SingleSendBuffer* send_buffer();

  MulticastInst* config();

  TransportReactorTask* reactor_task();
  ACE_Reactor* get_reactor();

  ACE_SOCK_Dgram_Mcast& socket();

  bool join(const ACE_INET_Addr& group_address);

  MulticastSession* find_or_create_session(MulticastPeer remote_peer);
  MulticastSession* find_session(MulticastPeer remote_peer);

  bool check_header(const TransportHeader& header);
  bool check_header(const DataSampleHeader& header);
  void sample_received(ReceivedDataSample& sample);

  bool reassemble(ReceivedDataSample& data, const TransportHeader& header);

private:
  MulticastTransport* transport_;

  MulticastSessionFactory_rch session_factory_;

  MulticastPeer local_peer_;

  MulticastInst* config_;

  TransportReactorTask* reactor_task_;

  MulticastSendStrategy_rch send_strategy_;
  MulticastReceiveStrategy_rch recv_strategy_;

  SingleSendBuffer* send_buffer_;

  ACE_SOCK_Dgram_Mcast socket_;

  ACE_SYNCH_RECURSIVE_MUTEX session_lock_;

  typedef std::map<MulticastPeer, MulticastSession_rch> MulticastSessionMap;
  MulticastSessionMap sessions_;

  virtual void stop_i();
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTDATALINK_H */
