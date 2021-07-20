/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTSESSION_H
#define OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTSESSION_H

#include "Multicast_Export.h"

#include "MulticastDataLink.h"
#include "MulticastTypes.h"

#include "ace/Message_Block.h"
#include "ace/Synch_Traits.h"

#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/transport/framework/TransportHeader.h"
#include "dds/DCPS/transport/framework/DataLinkWatchdog_T.h"
#include "dds/DCPS/transport/framework/TransportReassembly.h"
#include "dds/DCPS/RcEventHandler.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Reactor;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MulticastSession;

class OpenDDS_Multicast_Export SynWatchdog
  : public DataLinkWatchdog {
public:
  explicit SynWatchdog(ACE_Reactor* reactor,
                       ACE_thread_t owner,
                       MulticastSession* session);

  virtual bool reactor_is_shut_down() const;

protected:
  virtual TimeDuration next_interval();
  virtual void on_interval(const void* arg);

  virtual TimeDuration next_timeout();
  virtual void on_timeout(const void* arg);

private:
  ~SynWatchdog() { }
  MulticastSession* session_;
  size_t retries_;
};

class OpenDDS_Multicast_Export MulticastSession
  : public RcObject {
public:
  virtual ~MulticastSession();

  MulticastDataLink* link();

  MulticastPeer remote_peer() const;

  bool acked();
  void set_acked();
  virtual bool is_reliable() { return false;}

  void syn_received(const Message_Block_Ptr& control);
  void send_all_syn();
  void send_syn(const RepoId& local_writer,
                const RepoId& remote_reader);

  void synack_received(const Message_Block_Ptr& control);
  void send_synack(const RepoId& local_reader, const RepoId& remote_writer);
  virtual void send_naks() {}

  virtual bool check_header(const TransportHeader& header) = 0;
  virtual void record_header_received(const TransportHeader& header) = 0;
  virtual bool ready_to_deliver(const TransportHeader& header,
                                const ReceivedDataSample& data) = 0;
  virtual void release_remote(const RepoId& /*remote*/) {};

  virtual bool control_received(char submessage_id,
                                const Message_Block_Ptr& control);

  virtual bool start(bool active, bool acked) = 0;
  virtual void stop();

  bool reassemble(ReceivedDataSample& data, const TransportHeader& header);

  // Reliability.
  void add_remote(const RepoId& local,
                  const RepoId& remote);

  void remove_remote(const RepoId& local,
                     const RepoId& remote);

protected:
  MulticastDataLink* link_;

  MulticastPeer remote_peer_;

  MulticastSession(ACE_Reactor* reactor,
                   ACE_thread_t owner,
                   MulticastDataLink* link,
                   MulticastPeer remote_peer);

  void send_control(char submessage_id,
                    Message_Block_Ptr data);

  bool start_syn();

  virtual void syn_hook(const SequenceNumber& /*seq*/) {}

  typedef ACE_Reverse_Lock<ACE_Thread_Mutex> Reverse_Lock_t;
  Reverse_Lock_t reverse_start_lock_;

  ACE_Thread_Mutex start_lock_;
  bool started_;

  // A session must be for a publisher
  // or subscriber.  Implementation doesn't
  // support being for both.
  // As to control message,
  // only subscribers receive syn, send synack, send naks, receive nakack,
  // and publisher only send syn, receive synack, receive naks, send nakack.
  bool active_;

  TransportReassembly reassembly_;

  bool acked_;
  typedef OPENDDS_MAP_CMP(RepoId, RepoIdSet, GUID_tKeyLessThan) PendingRemoteMap;
  // For the active side, the pending_remote_map_ is used as a work queue.
  // The active side will send SYNs to all of the readers until it gets a SYNACK.
  // For the passive side, the pending_remote_map_ is used as a filter.
  // The passive side only responds to SYNs that correspond to an association.
  PendingRemoteMap pending_remote_map_;

private:
  void remove_remote_i(const RepoId& local,
                       const RepoId& remote);


  ACE_Thread_Mutex ack_lock_;
  RcHandle<SynWatchdog> syn_watchdog_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "MulticastSession.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTSESSION_H */
