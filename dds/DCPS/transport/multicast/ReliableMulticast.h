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
                    peer_type local_peer,
                    peer_type remote_peer);

  virtual bool header_received(const TransportHeader& header);
  virtual void sample_received(ReceivedDataSample& sample);

  virtual bool acked();

  void syn_received(ACE_Message_Block* message);
  void send_syn();

  void synack_received(ACE_Message_Block* message);
  void send_synack(peer_type remote_peer);

  void nak_received(ACE_Message_Block* message);
  void send_nak(peer_type remote_peer,
                ACE_INT16 low,
                ACE_INT16 high);
  void send_naks();

  void nakack_received(ACE_Message_Block* message);
  void send_nakack(peer_type remote_peer,
                   ACE_INT16 low,
                   ACE_INT16 high);

protected:
  virtual bool join_i(const ACE_INET_Addr& group_address, bool active);
  virtual void leave_i();

private:
  bool acked_;

  NakWatchdog nak_watchdog_;
  SynWatchdog syn_watchdog_;

  ACE_RW_Thread_Mutex active_lock_;
  ACE_RW_Thread_Mutex passive_lock_;

  TransportHeader recvd_header_;

  typedef std::map<peer_type, DisjointSequence> SequenceMap;
  SequenceMap sequences_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLEMULTICAST_H */
