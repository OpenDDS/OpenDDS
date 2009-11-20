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

#include "ace/Event_Handler.h"

namespace OpenDDS {
namespace DCPS {

enum MulticastSubMessageId {
  MULTICAST_SYN,
  MULTICAST_SYNACK
};

class OpenDDS_Multicast_Export SynHandler
  : public ACE_Event_Handler {
public:
  SynHandler(const ACE_Time_Value& deadline);
  
  virtual int handle_timeout(const ACE_Time_Value& now,
                             const void* arg);
private:
  ACE_Time_Value deadline_;
};

class OpenDDS_Multicast_Export ReliableMulticast
  : public MulticastDataLink {
public:
  ReliableMulticast(MulticastTransport* transport,
                    ACE_INT32 local_peer,
                    ACE_INT32 remote_peer);

  virtual void sample_received(ReceivedDataSample& sample);
  virtual bool acked();

  void syn_received(ACE_Message_Block* message);
  void send_syn(ACE_INT32 remote_peer);
 
  void synack_received(ACE_Message_Block* message);
  void send_synack(ACE_INT32 remote_peer); 
  
protected:
  virtual bool join_i(const ACE_INET_Addr& group_address, bool active);
  virtual void leave_i();

private:
  bool acked_;
  long syn_timer_id_;
  
  void cancel_syn_timer();
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLEMULTICAST_H */
