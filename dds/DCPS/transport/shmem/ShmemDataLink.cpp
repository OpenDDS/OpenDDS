/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemDataLink.h"
#include "ShmemTransport.h"
#include "ShmemInst.h"

#include "ace/Log_Msg.h"

#ifndef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

ShmemDataLink::ShmemDataLink(ShmemTransport* transport)
  : DataLink(transport,
             0, // priority
             false, // is_loopback,
             false) // is_active
  , config_(0)
{
}

bool
ShmemDataLink::open(const ACE_TString& remote_address)
{
  this->remote_address_ = remote_address;
//  this->is_loopback_ = this->remote_address_ == this->config_->local_address_;

  //TODO: initiate handshaking for active side only

  if (start(static_rchandle_cast<TransportSendStrategy>(this->send_strategy_),
            static_rchandle_cast<TransportStrategy>(this->recv_strategy_))
      != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemDataLink::open: start failed!\n")),
                     false);
  }

  return true;
}

void
ShmemDataLink::control_received(ReceivedDataSample& sample)
{
  TransportImpl_rch impl = this->impl();
  RcHandle<ShmemTransport> ut = static_rchandle_cast<ShmemTransport>(impl);
  //TODO: this is wrong:
  // At this time, the TRANSPORT_CONTROL messages in Shmem are only used for
  // the connection handshaking, so receiving one is an indication of the
  // passive_connection event.  In the future the submessage_id_ could be used
  // to allow different types of messages here.
  ut->passive_connection(this->remote_address_, sample.sample_);
}

void
ShmemDataLink::stop_i()
{
  //TODO
}

} // namespace DCPS
} // namespace OpenDDS
