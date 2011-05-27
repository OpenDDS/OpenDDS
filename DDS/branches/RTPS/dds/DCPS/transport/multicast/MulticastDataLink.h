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

#include "MulticastConfiguration.h"
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
#include "dds/DCPS/transport/framework/TransportSendBuffer_rch.h"

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
                    bool is_loopback,
                    bool is_active);
  virtual ~MulticastDataLink();

  MulticastTransport* transport();

  MulticastPeer local_peer() const;

  void configure(MulticastConfiguration* config,
                 TransportReactorTask* reactor_task);

  void send_strategy(MulticastSendStrategy* send_strategy);
  MulticastSendStrategy* send_strategy();

  void receive_strategy(MulticastReceiveStrategy* recv_strategy);
  MulticastReceiveStrategy* receive_strategy();

  TransportSendBuffer* send_buffer();

  MulticastConfiguration* config();

  TransportReactorTask* reactor_task();
  ACE_Reactor* get_reactor();

  ACE_SOCK_Dgram_Mcast& socket();

  bool join(const ACE_INET_Addr& group_address);

  MulticastSession* find_or_create_session(MulticastPeer remote_peer);

  bool acked(MulticastPeer remote_peer);

  bool check_header(const TransportHeader& header);
  bool check_header(const DataSampleHeader& header);
  void sample_received(ReceivedDataSample& sample);

  void set_check_fully_association();

private:
  MulticastTransport* transport_;

  MulticastSessionFactory_rch session_factory_;

  MulticastPeer local_peer_;

  MulticastConfiguration* config_;

  TransportReactorTask* reactor_task_;

  MulticastSendStrategy_rch send_strategy_;
  MulticastReceiveStrategy_rch recv_strategy_;

  TransportSendBuffer_rch send_buffer_;

  ACE_SOCK_Dgram_Mcast socket_;

  ACE_SYNCH_RECURSIVE_MUTEX session_lock_;

  typedef std::map<MulticastPeer, MulticastSession_rch> MulticastSessionMap;
  MulticastSessionMap sessions_;

  virtual void stop_i();

  bool check_fully_association_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTDATALINK_H */
