/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RELIABLESESSION_H
#define DCPS_RELIABLESESSION_H

#include "Multicast_Export.h"

#include "MulticastSession.h"
#include "MulticastTypes.h"

#include "ace/Synch_Traits.h"

#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/transport/framework/DataLinkWatchdog_T.h"

#include <map>
#include <set>

namespace OpenDDS {
namespace DCPS {

class ReliableSession;

class OpenDDS_Multicast_Export SynWatchdog
  : public DataLinkWatchdog<ACE_SYNCH_MUTEX> {
public:
  explicit SynWatchdog(ReliableSession* session);

protected:
  virtual ACE_Time_Value next_interval();
  virtual void on_interval(const void* arg);

  virtual ACE_Time_Value next_timeout();
  virtual void on_timeout(const void* arg);

private:
  ReliableSession* session_;
  size_t retries_;
};

class OpenDDS_Multicast_Export NakWatchdog
  : public DataLinkWatchdog<ACE_SYNCH_MUTEX> {
public:
  explicit NakWatchdog(ReliableSession* session);

protected:
  virtual ACE_Time_Value next_interval();
  virtual void on_interval(const void* arg);

private:
  ReliableSession* session_;
};

class OpenDDS_Multicast_Export ReliableSession
  : public MulticastSession {
public:
  ReliableSession(MulticastDataLink* link,
                  MulticastPeer remote_peer);

  virtual bool acked();

  virtual bool check_header(const TransportHeader& header);

  virtual void control_received(char submessage_id,
                                ACE_Message_Block* control);

  void syn_received(ACE_Message_Block* control);
  void send_syn();

  void synack_received(ACE_Message_Block* control);
  void send_synack();

  void expire_naks();
  void send_naks();

  void nak_received(ACE_Message_Block* control);
  void send_naks (DisjointSequence& missing);

  void nakack_received(ACE_Message_Block* control);
  void send_nakack(MulticastSequence low);

  virtual bool start(bool active);
  virtual void stop();

private:
  ACE_SYNCH_MUTEX ack_lock_;
  bool acked_;

  ACE_SYNCH_MUTEX start_lock_;
  bool started_;
  
  // A session must be for a publisher
  // or subscriber.  Implementation doesn't
  // support being for both.
  // As to control message, 
  // only subscribers receive syn, send synack, send naks, receive nakack,
  // and publisher only send syn, receive synack,receive naks, send nakack.
  bool active_; 

  SynWatchdog syn_watchdog_;
  NakWatchdog nak_watchdog_;

  DisjointSequence nak_sequence_;

  typedef std::multimap<ACE_Time_Value, SequenceNumber> NakRequestMap;
  NakRequestMap nak_requests_;

  typedef std::set<SequenceRange> NakPeerSet;
  NakPeerSet nak_peers_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLESESSION_H */
