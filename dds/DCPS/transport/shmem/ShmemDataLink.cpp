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

#include <cstdlib>

#ifndef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

ShmemDataLink::ShmemDataLink(ShmemTransport* transport)
  : DataLink(transport,
             0,     // priority
             false, // is_loopback,
             false) // is_active
  , config_(0)
  , peer_alloc_(0)
{
}

bool
ShmemDataLink::open(const std::string& peer_address)
{
  peer_address_ = peer_address;

  peer_alloc_ =
    new ShmemAllocator(ACE_TEXT_CHAR_TO_TCHAR(peer_address.c_str()));

  if (-1 == peer_alloc_->find("Semaphore")) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ShmemDataLink::open: ")
                      ACE_TEXT("peer's shared memory area not found (%C)\n"),
                      peer_address.c_str()),
                     false);
  }

  if (start(static_rchandle_cast<TransportSendStrategy>(send_strategy_),
            static_rchandle_cast<TransportStrategy>(recv_strategy_))
      != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemDataLink::open: start failed!\n")),
                     false);
  }

  VDBG_LVL((LM_INFO, "(%P|%t) ShmemDataLink link %@ open to peer %C\n",
            this, peer_address_.c_str()), 1);

  return true;
}

void
ShmemDataLink::control_received(ReceivedDataSample& /*sample*/)
{
}

void
ShmemDataLink::stop_i()
{
  if (peer_alloc_) {
    peer_alloc_->release(0 /*don't close*/);
  }
  delete peer_alloc_;
  peer_alloc_ = 0;
}

ShmemAllocator*
ShmemDataLink::local_allocator()
{
  return static_rchandle_cast<ShmemTransport>(impl())->alloc();
}

std::string
ShmemDataLink::local_address()
{
  return static_rchandle_cast<ShmemTransport>(impl())->address();
}

void
ShmemDataLink::signal_semaphore()
{
  return static_rchandle_cast<ShmemTransport>(impl())->signal_semaphore();
}

pid_t
ShmemDataLink::peer_pid()
{
  return std::atoi(peer_address_.c_str() + peer_address_.find('-') + 1);
}

} // namespace DCPS
} // namespace OpenDDS
