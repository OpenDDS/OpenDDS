/*
 * $Id$
 *
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
#include "dds/DCPS/PoolAllocator.h"

namespace OpenDDS {
namespace DCPS {

class ReliableSession;

class OpenDDS_Multicast_Export NakWatchdog
  : public DataLinkWatchdog {
public:
  explicit NakWatchdog(ACE_Reactor* reactor,
                       ACE_thread_t owner,
                       ReliableSession* session);

  virtual bool reactor_is_shut_down() const;

protected:
  virtual ACE_Time_Value next_interval();
  virtual void on_interval(const void* arg);

private:
  ~NakWatchdog() { }
  ReliableSession* session_;
};

class OpenDDS_Multicast_Export ReliableSession
  : public MulticastSession {
public:
  ReliableSession(ACE_Reactor* reactor,
                  ACE_thread_t owner,
                  MulticastDataLink* link,
                  MulticastPeer remote_peer);

  ~ReliableSession();

  virtual bool check_header(const TransportHeader& header);

  virtual bool control_received(char submessage_id,
                                ACE_Message_Block* control);

  void expire_naks();
  void send_naks();

  void nak_received(ACE_Message_Block* control);
  void send_naks(DisjointSequence& found);

  void nakack_received(ACE_Message_Block* control);
  virtual void send_nakack(SequenceNumber low);

  virtual bool start(bool active, bool acked);
  virtual void stop();

  virtual void syn_hook(const SequenceNumber& seq);

private:
  NakWatchdog* nak_watchdog_;

  DisjointSequence nak_sequence_;

  typedef OPENDDS_MAP(ACE_Time_Value, SequenceNumber) NakRequestMap;
  NakRequestMap nak_requests_;

  typedef OPENDDS_SET(SequenceRange) NakPeerSet;
  NakPeerSet nak_peers_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLESESSION_H */
