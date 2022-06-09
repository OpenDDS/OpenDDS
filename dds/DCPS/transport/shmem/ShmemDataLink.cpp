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

#include "dds/DCPS/transport/framework/TransportControlElement.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"

#include "ace/Log_Msg.h"

#include <cstdlib>

#ifndef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace {
  using OpenDDS::DCPS::Encoding;
  const Encoding encoding_unaligned_native(Encoding::KIND_UNALIGNED_CDR);
}

namespace OpenDDS {
namespace DCPS {

ShmemDataLink::ShmemDataLink(ShmemTransport& transport)
  : DataLink(transport,
             0,     // priority
             false, // is_loopback,
             false) // is_active
  , config_(0)
  , send_strategy_(make_rch<ShmemSendStrategy>(this))
  , recv_strategy_(make_rch<ShmemReceiveStrategy>(this))
  , peer_alloc_(0)
{
}

bool
ShmemDataLink::open(const std::string& peer_address)
{
  peer_address_ = peer_address;
  const ACE_TString name = ACE_TEXT_CHAR_TO_TCHAR(peer_address.c_str());
  ShmemAllocator::MEMORY_POOL_OPTIONS alloc_opts;

#ifdef OPENDDS_SHMEM_WINDOWS
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
OpenDDS::DCPS::ShmemDataLink::send_association_msg(const RepoId& local, const RepoId& remote)
{
  DataSampleHeader header_data;
  header_data.message_id_ = REQUEST_ACK;
  header_data.byte_order_  = ACE_CDR_BYTE_ORDER;
  header_data.message_length_ = sizeof(remote);
  header_data.sequence_ = -1;
  header_data.publication_id_ = local;
  header_data.publisher_id_ = remote;

  Message_Block_Ptr message(
    new ACE_Message_Block(header_data.get_max_serialized_size(),
                          ACE_Message_Block::MB_DATA,
                          0, //cont
                          0, //data
                          0, //allocator_strategy
                          0, //locking_strategy
                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                          ACE_Time_Value::zero,
                          ACE_Time_Value::max_time,
                          0,
                          0));

  *message << header_data;
  DCPS::Serializer ser(message.get(), encoding_unaligned_native);
  ser << remote;
  send_strategy_->link_released(false);
  TransportControlElement* send_element = new TransportControlElement(move(message));
  this->send_i(send_element, false);
  VDBG((LM_INFO, "(%P|%t) ShmemDataLink send_association_msg sent\n"));
}

void
OpenDDS::DCPS::ShmemDataLink::request_ack_received(ReceivedDataSample& sample)
{
  VDBG((LM_INFO, "(%P|%t) ShmemDataLink request_ack_received\n"));
  if (sample.header_.sequence_ == -1 && sample.header_.message_length_ == sizeof(RepoId)) {
    VDBG((LM_INFO, "(%P|%t) ShmemDataLink received association msg\n"));
    RepoId local;
    DCPS::Serializer ser(&(*sample.sample_), encoding_unaligned_native);
    if (ser >> local) {
      invoke_on_start_callbacks(local, sample.header_.publication_id_, true);
    }
    return;
  }
  data_received(sample);
}

void
ShmemDataLink::control_received(ReceivedDataSample& /*sample*/)
{
}

void
ShmemDataLink::stop_i()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (peer_alloc_) {
    peer_alloc_->release(0 /*don't close*/);
  }
  delete peer_alloc_;
  peer_alloc_ = 0;
}

ShmemTransport&
ShmemDataLink::impl() const
{
  return static_cast<ShmemTransport&>(DataLink::impl());
}

ShmemAllocator*
ShmemDataLink::peer_allocator()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
  return peer_alloc_;
}

ShmemAllocator*
ShmemDataLink::local_allocator()
{
  return impl().alloc();
}

std::string
ShmemDataLink::local_address()
{
  return impl().address();
}

void
ShmemDataLink::signal_semaphore()
{
  return impl().signal_semaphore();
}

pid_t
ShmemDataLink::peer_pid()
{
  return std::atoi(peer_address_.c_str() + peer_address_.find('-') + 1);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
