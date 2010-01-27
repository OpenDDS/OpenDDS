/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSession.h"

#include "ace/Log_Msg.h"

#ifndef __ACE_INLINE__
# include "MulticastSession.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

MulticastSession::MulticastSession(MulticastDataLink* link,
                                   MulticastPeer remote_peer)
  : link_(link),
    remote_peer_(remote_peer),
    defunct_(false)
{
}

MulticastSession::~MulticastSession()
{
}

void
MulticastSession::reliability_lost()
{
  // Stop session to avoid sending/receiving unreliable data:
  stop();

  this->defunct_ = true;

  // Notify transport DataLink has become unreliable; this
  // will ultimately cause the session to be destroyed:
  MulticastTransport* transport = this->link_->transport();
  transport->reliability_lost(this->link_);
}

void
MulticastSession::send_control(char submessage_id, ACE_Message_Block* data)
{
  ACE_Message_Block* control = this->link_->create_control(submessage_id, data);
  if (control == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("MulticastSession::send_control: ")
               ACE_TEXT("create_control failed!\n")));
    return;
  }

  int error;
  if ((error = this->link_->send_control(control)) != SEND_CONTROL_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("MulticastSession::send_control: ")
               ACE_TEXT("send_control failed: %d!\n"),
               error));
    return;
  }
}

} // namespace DCPS
} // namespace OpenDDS
