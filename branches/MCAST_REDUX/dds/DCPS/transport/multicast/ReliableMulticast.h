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

#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/transport/framework/DataLinkWatchdog_T.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class ReliableMulticast;

class OpenDDS_Multicast_Export NakWatchdog
  : public DataLinkWatchdog<ReliableMulticast> {
public:
  explicit NakWatchdog(ReliableMulticast* link);

protected:
  virtual ACE_Time_Value next_interval();
  virtual void on_interval(const void* arg);
};

class OpenDDS_Multicast_Export SynWatchdog
  : public DataLinkWatchdog<ReliableMulticast> {
public:
  explicit SynWatchdog(ReliableMulticast* link);

protected:
  virtual ACE_Time_Value next_interval();
  virtual void on_interval(const void* arg);

  virtual ACE_Time_Value next_timeout();
  virtual void on_timeout(const void* arg);
};

class OpenDDS_Multicast_Export ReliableMulticast
  : public MulticastDataLink {
public:
  enum {
    MULTICAST_SYN,
    MULTICAST_SYNACK,
    MULTICAST_NAK,
    MULTICAST_NAKACK
  };

  ReliableMulticast(MulticastTransport* transport,
                    MulticastPeer local_peer,
                    MulticastPeer remote_peer);

  virtual bool header_received(const TransportHeader& header);
  virtual void sample_received(ReceivedDataSample& sample);

  virtual bool acked();

  void syn_received(ACE_Message_Block* message);
  void send_syn();

  void synack_received(ACE_Message_Block* message);
  void send_synack(MulticastPeer remote_peer);

  void nak_received(ACE_Message_Block* message);
  void send_nak(MulticastPeer remote_peer,
                MulticastSequence low,
                MulticastSequence high);
  void send_naks();

  void nakack_received(ACE_Message_Block* message);
  void send_nakack(MulticastPeer remote_peer,
                   MulticastSequence low,
                   MulticastSequence high);

protected:
  virtual bool join_i(const ACE_INET_Addr& group_address, bool active);
  virtual void leave_i();

private:
  bool acked_;

  NakWatchdog nak_watchdog_;
  SynWatchdog syn_watchdog_;

  TransportHeader recvd_header_;

  typedef std::map<MulticastPeer, DisjointSequence> SequenceMap;
  SequenceMap sequences_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLEMULTICAST_H */
