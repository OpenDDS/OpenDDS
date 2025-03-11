/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemDataLink.h"

#include "ShmemTransport.h"
#include "ShmemInst.h"
#include "ShmemSendStrategy.h"
#include "ShmemReceiveStrategy.h"

#include <dds/DCPS/transport/framework/TransportControlElement.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DdsDcpsGuidTypeSupportImpl.h>

#include <ace/Log_Msg.h>

#include <cstdlib>

#ifndef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace {
  const Encoding encoding_unaligned_native(Encoding::KIND_UNALIGNED_CDR);
}

ShmemDataLink::ShmemDataLink(const RcHandle<ShmemTransport>& transport)
  : DataLink(transport,
             0,     // priority
             false, // is_loopback,
             false) // is_active
  , send_strategy_(make_rch<ShmemSendStrategy>(this))
  , recv_strategy_(make_rch<ShmemReceiveStrategy>(this))
  , peer_alloc_(0)
  , reactor_task_(transport->reactor_task())
{
}

bool
ShmemDataLink::open(const std::string& peer_address)
{
  peer_address_ = peer_address;
  const ACE_TString name = ACE_TEXT_CHAR_TO_TCHAR(peer_address.c_str());

#ifdef OPENDDS_SHMEM_WINDOWS
  ShmemAllocator::MEMORY_POOL_OPTIONS alloc_opts;
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
#endif

  peer_alloc_ = new ShmemAllocator(name.c_str(), 0 /*lock_name*/
#ifdef OPENDDS_SHMEM_WINDOWS
    , &alloc_opts
#endif
    );

  if (-1 == peer_alloc_->find("Semaphore")) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ShmemDataLink::open: ")
                      ACE_TEXT("peer's shared memory area not found (%C)\n"),
                      peer_address.c_str()),
                     false);
  }

  if (start(static_rchandle_cast<TransportSendStrategy>(send_strategy_),
            static_rchandle_cast<TransportStrategy>(recv_strategy_),
            false)
      != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemDataLink::open: start failed!\n")),
                     false);
  }

  VDBG_LVL((LM_DEBUG, "(%P|%t) ShmemDataLink::open: link[%@] open to peer %C\n",
            this, peer_address_.c_str()), 1);

  assoc_resends_task_ = make_rch<SmPeriodicTask>(reactor_task_,
    ref(*this), &ShmemDataLink::resend_association_msgs);
  ShmemInst_rch cfg = config();
  if (!cfg) {
    return false;
  }
  assoc_resends_task_->enable(false, cfg->association_resend_period());

  return true;
}

int ShmemDataLink::make_reservation(const GUID_t& remote_pub, const GUID_t& local_sub,
  const TransportReceiveListener_wrch& receive_listener, bool reliable)
{
  const int result = DataLink::make_reservation(remote_pub, local_sub, receive_listener, reliable);
  if (result != 0) {
    return result;
  }

  // Tell writer we are ready and resend that message until we get a response.
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, assoc_resends_mutex_, -1);
  if (assoc_resends_.insert(GuidPair(local_sub, remote_pub)).second) {
    send_association_msg(local_sub, remote_pub);
  }

  return 0;
}

void
ShmemDataLink::send_association_msg(const GUID_t& local, const GUID_t& remote)
{
  VDBG((LM_DEBUG, "(%P|%t) ShmemDataLink::send_association_msg from %C to %C\n",
    LogGuid(local).c_str(), LogGuid(remote).c_str()));

  DataSampleHeader header_data;
  header_data.message_id_ = REQUEST_ACK;
  header_data.byte_order_ = ACE_CDR_BYTE_ORDER;
  header_data.message_length_ = guid_cdr_size;
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
  Serializer ser(message.get(), encoding_unaligned_native);
  ser << remote;
  send_strategy_->link_released(false);
  TransportControlElement* send_element = new TransportControlElement(OPENDDS_MOVE_NS::move(message));
  this->send_i(send_element, false);
}

void ShmemDataLink::resend_association_msgs(const MonotonicTimePoint&)
{
  VDBG((LM_DEBUG, "(%P|%t) ShmemDataLink::resend_association_msgs\n"));

  ACE_GUARD(ACE_Thread_Mutex, g, assoc_resends_mutex_);
  for (AssocResends::iterator i = assoc_resends_.begin(); i != assoc_resends_.end(); ++i) {
    send_association_msg(i->local, i->remote);
  }
}

void ShmemDataLink::stop_resend_association_msgs(const GUID_t& local, const GUID_t& remote)
{
  VDBG((LM_DEBUG, "(%P|%t) ShmemDataLink::stop_resend_association_msgs: "
    "local %C remote %C\n", LogGuid(local).c_str(), LogGuid(remote).c_str()));
  ACE_GUARD(ACE_Thread_Mutex, g, assoc_resends_mutex_);
  assoc_resends_.erase(GuidPair(local, remote));
}

void
ShmemDataLink::request_ack_received(ReceivedDataSample& sample)
{
  if (sample.header_.sequence_ == -1 && sample.header_.message_length_ == guid_cdr_size) {
    VDBG((LM_DEBUG, "(%P|%t) ShmemDataLink::request_ack_received: association msg\n"));
    GUID_t local;
    Message_Block_Ptr payload(recv_strategy_->to_msgblock(sample));
    Serializer ser(payload.get(), encoding_unaligned_native);
    if (ser >> local) {
      const GUID_t& remote = sample.header_.publication_id_;
      const GuidConverter local_gc(local);
      const bool local_is_writer = local_gc.isWriter();
      VDBG((LM_DEBUG, "(%P|%t) ShmemDataLink::request_ack_received: "
        "association msg from remote %C %C to local %C %C\n",
        local_is_writer ? "reader" : "writer", LogGuid(remote).c_str(),
        local_is_writer ? "writer" : "reader", std::string(local_gc).c_str()));
      if (local_is_writer) {
        // Reader has signaled it's ready to receive messages.
        if (invoke_on_start_callbacks(local, remote, true)) {
          // In case we're getting duplicates, only acknowledge if we can invoke
          // the on start callback, which should only happen once.
          send_association_msg(local, remote);
        }
      } else {
        // Writer has responded to association ack, stop sending.
        stop_resend_association_msgs(local, remote);
      }
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
  DBG_ENTRY_LVL("ShmemDataLink","stop_i",6);

  {
    ACE_GUARD(ACE_Thread_Mutex, g, assoc_resends_mutex_);
    assoc_resends_.clear();
    assoc_resends_task_->disable();
  }

  {
    ACE_GUARD(ACE_Thread_Mutex, g, peer_alloc_mutex_);
    if (peer_alloc_) {
      // Calling release() has to be done with argument 1 (close),
      // because with 1 ACE_Malloc_T will call release on the underlying
      // shared memory pool
      if (peer_alloc_->release(1 /*close*/) == -1) {
        VDBG_LVL((LM_ERROR,
                  "(%P|%t) ShmemDataLink::stop_i Release shared memory failed\n"), 1);
      }
      delete peer_alloc_;
      peer_alloc_ = 0;
    }
  }
}

RcHandle<ShmemTransport>
ShmemDataLink::transport() const
{
  return dynamic_rchandle_cast<ShmemTransport>(impl());
}

ShmemAllocator*
ShmemDataLink::peer_allocator()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, peer_alloc_mutex_, 0);
  return peer_alloc_;
}

ShmemAllocator*
ShmemDataLink::local_allocator()
{
  ShmemAllocator* result = 0;
  OPENDDS_TEST_AND_CALL_ASSIGN(ShmemTransport_rch, transport(), alloc(), result);
  return result;
}

std::string
ShmemDataLink::local_address()
{
  std::string result;
  OPENDDS_TEST_AND_CALL_ASSIGN(ShmemTransport_rch, transport(), address(), result);
  return result;
}

void
ShmemDataLink::signal_semaphore()
{
  OPENDDS_TEST_AND_CALL(ShmemTransport_rch, transport(), signal_semaphore());
}

pid_t
ShmemDataLink::peer_pid()
{
  return std::atoi(peer_address_.c_str() + peer_address_.find('-') + 1);
}

ShmemInst_rch
ShmemDataLink::config() const
{
  return dynamic_rchandle_cast<ShmemInst>(transport()->config());
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
