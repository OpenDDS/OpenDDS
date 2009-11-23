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
  virtual bool on_interval(const void* arg);

  virtual ACE_Time_Value next_timeout();
  virtual void on_timeout(const void* arg);
};

//

class OpenDDS_Multicast_Export NakWatchdog
  : public DataLinkWatchdog<ReliableMulticast> {
public:
  explicit NakWatchdog(ReliableMulticast* link);

protected:
  virtual ACE_Time_Value next_interval();
  virtual bool on_interval(const void* arg);
};

//

enum MulticastSubMessageId {
  MULTICAST_SYN,
  MULTICAST_SYNACK,
  MULTICAST_NAK,
  MULTICAST_NAKACK
};

class OpenDDS_Multicast_Export ReliableMulticast
  : public MulticastDataLink {
public:
  ReliableMulticast(MulticastTransport* transport,
                    ACE_INT32 local_peer,
                    ACE_INT32 remote_peer);

  virtual bool header_received(const TransportHeader& header);
  virtual void sample_received(ReceivedDataSample& sample);
  
  virtual bool acked();

  void syn_received(ACE_Message_Block* message);
  void send_syn(ACE_INT32 remote_peer);

  void synack_received(ACE_Message_Block* message);
  void send_synack(ACE_INT32 remote_peer);
  
  void nak_received(ACE_Message_Block* message);
  void send_nak(ACE_INT32 remote_peer,
                ACE_INT16 low,
                ACE_INT16 high);
  void send_naks();
  
  void nakack_received(ACE_Message_Block* message);
  void send_nakack(ACE_INT32 remote_peer,
                   ACE_INT16 low,
                   ACE_INT16 high);

protected:
  virtual bool join_i(const ACE_INET_Addr& group_address, bool active);
  virtual void leave_i();

private:
  bool acked_;

  ACE_INT16 last_sequence_;

  SynWatchdog syn_watchdog_;
  NakWatchdog nak_watchdog_;

  ACE_RW_Thread_Mutex passive_lock_;
  
  typedef std::map<ACE_INT32, DisjointSequence> SequenceMap;
  SequenceMap sequences_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLEMULTICAST_H */
