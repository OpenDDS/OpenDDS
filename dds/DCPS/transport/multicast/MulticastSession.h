/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTSESSION_H
#define DCPS_MULTICASTSESSION_H

#include "Multicast_Export.h"

#include "MulticastDataLink.h"
#include "MulticastTypes.h"

#include "ace/Message_Block.h"
#include "ace/Synch_Traits.h"

#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/transport/framework/TransportHeader.h"
#include "dds/DCPS/transport/framework/DataLinkWatchdog_T.h"
#include "dds/DCPS/transport/framework/TransportReassembly.h"

class ACE_Reactor;

namespace OpenDDS {
namespace DCPS {

class MulticastSession;

class OpenDDS_Multicast_Export SynWatchdog
  : public DataLinkWatchdog<ACE_SYNCH_MUTEX> {
public:
  explicit SynWatchdog(MulticastSession* session);

protected:
  virtual ACE_Time_Value next_interval();
  virtual void on_interval(const void* arg);

  virtual ACE_Time_Value next_timeout();
  virtual void on_timeout(const void* arg);

private:
  MulticastSession* session_;
  size_t retries_;
};

class OpenDDS_Multicast_Export MulticastSession
  : public RcObject<ACE_SYNCH_MUTEX> {
public:
  virtual ~MulticastSession();

  MulticastDataLink* link();

  MulticastPeer remote_peer() const;

  bool acked();
  bool wait_for_ack();

  void syn_received(ACE_Message_Block* control);
  void send_syn();

  void synack_received(ACE_Message_Block* control);
  void send_synack();
  virtual void send_nakack(SequenceNumber /*low*/) {}

  virtual bool check_header(const TransportHeader& header) = 0;

  virtual bool control_received(char submessage_id,
                                ACE_Message_Block* control);

  virtual bool start(bool active) = 0;
  virtual void stop();

  bool reassemble(ReceivedDataSample& data, const TransportHeader& header);

protected:
  MulticastDataLink* link_;

  MulticastPeer remote_peer_;

  MulticastSession(MulticastDataLink* link,
                   MulticastPeer remote_peer);

  void send_control(char submessage_id,
                    ACE_Message_Block* data);

  bool start_syn(ACE_Reactor* reactor);

  virtual void syn_hook(const SequenceNumber& /*seq*/) {}

  ACE_SYNCH_MUTEX start_lock_;
  bool started_;

  // A session must be for a publisher
  // or subscriber.  Implementation doesn't
  // support being for both.
  // As to control message,
  // only subscribers receive syn, send synack, send naks, receive nakack,
  // and publisher only send syn, receive synack, receive naks, send nakack.
  bool active_;

  TransportReassembly reassembly_;

private:
  ACE_SYNCH_MUTEX ack_lock_;
  ACE_SYNCH_CONDITION ack_cond_;
  bool acked_;

  SynWatchdog syn_watchdog_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastSession.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTSESSION_H */
