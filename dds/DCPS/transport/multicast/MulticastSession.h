/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastSession
  : public RcObject<ACE_SYNCH_MUTEX> {
public:
  virtual ~MulticastSession();

  MulticastDataLink* link();

  MulticastPeer remote_peer() const;

  virtual bool acked() = 0;

  virtual bool check_header(const TransportHeader& header) = 0;

  virtual void control_received(char submessage_id,
                                ACE_Message_Block* control) = 0;

  virtual bool start(bool active) = 0;
  virtual void stop() = 0;

protected:
  MulticastDataLink* link_;

  MulticastPeer remote_peer_;

  MulticastSession(MulticastDataLink* link,
                   MulticastPeer remote_peer);

  void send_control(char submessage_id,
                    ACE_Message_Block* data);
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastSession.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTSESSION_H */
