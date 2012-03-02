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

#include <map>
#include <set>

namespace OpenDDS {
namespace DCPS {

class ReliableSession;

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

  virtual bool check_header(const TransportHeader& header);

  virtual bool control_received(char submessage_id,
                                ACE_Message_Block* control);

  void expire_naks();
  void send_naks();

  void nak_received(ACE_Message_Block* control);
  void send_naks(DisjointSequence& found);

  void nakack_received(ACE_Message_Block* control);
  virtual void send_nakack(SequenceNumber low);

  virtual bool start(bool active);
  virtual void stop();

  virtual void syn_hook(const SequenceNumber& seq);

private:
  NakWatchdog nak_watchdog_;

  DisjointSequence nak_sequence_;

  typedef std::map<ACE_Time_Value, SequenceNumber> NakRequestMap;
  NakRequestMap nak_requests_;

  typedef std::set<SequenceRange> NakPeerSet;
  NakPeerSet nak_peers_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLESESSION_H */
