/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemDataLink.h"
#include "ShmemTransport.h"
#include "ShmemInst.h"
#include "ShmemSendStrategy.h"
#include "ShmemReceiveStrategy.h"

#include "ace/Log_Msg.h"

#include <cstdlib>

#ifndef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ShmemDataLink::ShmemDataLink(const ShmemTransport_rch& transport)
  : DataLink(transport,
             0,     // priority
             false, // is_loopback,
             false) // is_active
  , config_(0)
  , send_strategy_( make_rch<ShmemSendStrategy>(this, transport->config()))
  , recv_strategy_( make_rch<ShmemReceiveStrategy>(this))
  , peer_alloc_(0)
{
}

bool
ShmemDataLink::open(const std::string& peer_address)
{
  peer_address_ = peer_address;
  const ACE_TString name = ACE_TEXT_CHAR_TO_TCHAR(peer_address.c_str());
  ShmemAllocator::MEMORY_POOL_OPTIONS alloc_opts;

#ifdef ACE_WIN32
  const bool use_opts = true;
  const ACE_TString name_under = name + ACE_TEXT('_');
  // Find max size of peer's pool so enough local address space is reserved.
  HANDLE fm = ACE_TEXT_CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READONLY,
    0, ACE_DEFAULT_PAGEFILE_POOL_CHUNK, name_under.c_str());
  void* view;
  if (fm == 0 || (view = MapViewOfFile(fm, FILE_MAP_READ, 0, 0, 0)) == 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ShmemDataLink::open: ")
                      ACE_TEXT("peer's shared memory area not found (%C)\n"),
                      peer_address.c_str()),
                     false);
  }
  // location of max_size_ in ctrl block: a size_t after two void*s
  const size_t* pmax = (const size_t*)(((void**)view) + 2);
  alloc_opts.max_size_ = *pmax;
  UnmapViewOfFile(view);
  CloseHandle(fm);
#else
  const bool use_opts = false;
#endif

  peer_alloc_ = new ShmemAllocator(name.c_str(), 0 /*lock_name*/,
                                   use_opts ? &alloc_opts : 0);

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

OPENDDS_END_VERSIONED_NAMESPACE_DECL
