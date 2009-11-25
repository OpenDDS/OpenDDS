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
                    MulticastPeer remote_peer);

  virtual bool header_received(const TransportHeader& header);
  virtual void sample_received(ReceivedDataSample& sample);

  virtual bool acked();
  
  void expire_naks(); 
  void send_naks();
 
  void syn_received(ACE_Message_Block* control);
  void send_syn();

  void synack_received(ACE_Message_Block* control);
  void send_synack(MulticastPeer remote_peer);
  
  void nak_received(ACE_Message_Block* control);
  void send_nak(MulticastPeer remote_peer,
                MulticastSequence low,
                MulticastSequence high);

  void nakack_received(ACE_Message_Block* control);
  void send_nakack(MulticastSequence low,
                   MulticastSequence high);

  void send_control(SubMessageId submessage_id,
                    ACE_Message_Block* data);

protected:
  virtual bool join_i(const ACE_INET_Addr& group_address, bool active);
  virtual void leave_i();

private:
  bool acked_;

  SynWatchdog syn_watchdog_;
  NakWatchdog nak_watchdog_;

  TransportHeader received_header_;

  typedef std::map<MulticastPeer, DisjointSequence> SequenceMap;
  SequenceMap sequences_;

  typedef std::pair<MulticastPeer, SequenceNumber> NakRequest;
  typedef std::multimap<ACE_Time_Value, NakRequest> NakHistory;
  NakHistory nak_history_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLEMULTICAST_H */
