/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_MULTICAST_RELIABLESESSION_H
#define OPENDDS_DCPS_TRANSPORT_MULTICAST_RELIABLESESSION_H

#include "Multicast_Export.h"

#include "MulticastSession.h"
#include "MulticastTypes.h"

#include "ace/Synch_Traits.h"

#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/RcEventHandler.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
  virtual TimeDuration next_interval();
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
  virtual void record_header_received(const TransportHeader& header);

  virtual bool ready_to_deliver(const TransportHeader& header,
                                const ReceivedDataSample& data);
  void deliver_held_data();
  virtual void release_remote(const RepoId& remote);

  virtual bool control_received(char submessage_id,
                                const Message_Block_Ptr& control);

  void expire_naks();
  void send_naks();

  void nak_received(const Message_Block_Ptr& control);
  void send_naks(DisjointSequence& found);

  void nakack_received(const Message_Block_Ptr& control);
  virtual void send_nakack(SequenceNumber low);

  virtual bool start(bool active, bool acked);
  virtual void stop();
  virtual bool is_reliable() { return true;}

  virtual void syn_hook(const SequenceNumber& seq);

private:
  RcHandle<NakWatchdog> nak_watchdog_;

  DisjointSequence nak_sequence_;

  typedef OPENDDS_MAP(MonotonicTimePoint, SequenceNumber) NakRequestMap;
  NakRequestMap nak_requests_;

  ACE_Thread_Mutex held_lock_;
  typedef SequenceNumber TransportHeaderSN;
  OPENDDS_MULTIMAP(TransportHeaderSN, ReceivedDataSample) held_;

  typedef OPENDDS_SET(SequenceRange) NakPeerSet;
  NakPeerSet nak_peers_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RELIABLESESSION_H */
