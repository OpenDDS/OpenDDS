/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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
#include "dds/DCPS/transport/framework/TransportSendBuffer_rch.h"

#include <map>
#include <utility>

namespace OpenDDS {
namespace DCPS {

class ReliableMulticast;

class OpenDDS_Multicast_Export SynWatchdog
  : public DataLinkWatchdog<ReliableMulticast> {
public:
  explicit SynWatchdog(ReliableMulticast* link);

protected:
  virtual ACE_Time_Value next_interval();
  virtual void on_interval(const void* arg);

  virtual ACE_Time_Value next_timeout();
  virtual void on_timeout(const void* arg);

private:
  int retries_;
};

class OpenDDS_Multicast_Export NakWatchdog
  : public DataLinkWatchdog<ReliableMulticast> {
public:
  explicit NakWatchdog(ReliableMulticast* link);

protected:
  virtual ACE_Time_Value next_interval();
  virtual void on_interval(const void* arg);
};

class OpenDDS_Multicast_Export ReliableMulticast
  : public MulticastDataLink {
public:
  enum SubMessageId {
    MULTICAST_SYN,
    MULTICAST_SYNACK,
    MULTICAST_NAK,
    MULTICAST_NAKACK
  };

  ReliableMulticast(MulticastTransport* transport,
                    MulticastPeer local_peer,
                    MulticastPeer remote_peer,
                    bool active);
  ~ReliableMulticast();

  void expire_naks();
  void send_naks();

  virtual bool acked();

  virtual bool header_received(const TransportHeader& header);
  virtual void sample_received(ReceivedDataSample& sample);

  void syn_received(ACE_Message_Block* control);
  void send_syn();

  void synack_received(ACE_Message_Block* control);
  void send_synack(MulticastPeer remote_peer);

  void nak_received(ACE_Message_Block* control);
  void send_nak(MulticastPeer remote_peer,
                MulticastSequence low,
                MulticastSequence high);

  void nakack_received(ACE_Message_Block* control);
  void send_nakack(MulticastSequence low);

  void send_control(SubMessageId submessage_id,
                    ACE_Message_Block* data);

protected:
  virtual void send_strategy(MulticastSendStrategy* send_strategy);

  virtual int start_i();
  virtual void stop_i();

private:
  bool acked_;

  SynWatchdog syn_watchdog_;
  NakWatchdog nak_watchdog_;

  TransportSendBuffer_rch send_buffer_;

  typedef std::map<MulticastPeer, DisjointSequence> NakSequenceMap;
  NakSequenceMap nak_sequences_;

  typedef std::pair<MulticastPeer, SequenceNumber> NakRequest;
  typedef std::multimap<ACE_Time_Value, NakRequest> NakRequestMap;
  NakRequestMap nak_requests_;

  typedef std::multimap<MulticastPeer, SequenceRange> NakPeerMap;
  NakPeerMap nak_peers_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLEMULTICAST_H */
