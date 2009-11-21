/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RELIABLEMULTICAST_H
#define DCPS_RELIABLEMULTICAST_H

#include "Multicast_Export.h"

#include "MulticastDataLink.h"

#include "dds/DCPS/transport/framework/TransportWatchdog_T.h"

namespace OpenDDS {
namespace DCPS {

enum MulticastSubMessageId {
  MULTICAST_SYN,
  MULTICAST_SYNACK
};

class ReliableMulticast;

class OpenDDS_Multicast_Export SynWatchdog
  : public TransportWatchdog<ReliableMulticast> {
public:
  SynWatchDog(ReliableMulticast* link);

protected:
  virtual ACE_Time_Value get_interval();
  virtual bool handle_interval(const void* arg);

  virtual ACE_Time_Value get_timeout();
  virtual void handle_timeout(const void* arg);
};

class OpenDDS_Multicast_Export ReliableMulticast
  : public MulticastDataLink {
public:
  ReliableMulticast(MulticastTransport* transport,
                    ACE_INT32 local_peer,
                    ACE_INT32 remote_peer);

  virtual void sample_received(ReceivedDataSample& sample);
  virtual bool acked();

  void syn_received(ACE_Message_Block* message);
  void send_syn(ACE_INT32 remote_peer);

  void synack_received(ACE_Message_Block* message);
  void send_synack(ACE_INT32 remote_peer);

protected:
  virtual bool join_i(const ACE_INET_Addr& group_address, bool active);
  virtual void leave_i();

private:
  SynWatchdog syn_watchdog_;
  bool acked_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLEMULTICAST_H */
