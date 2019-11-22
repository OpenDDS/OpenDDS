/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemTransport.h"
#include "ShmemInst.h"
#include "ShmemSendStrategy.h"
#include "ShmemReceiveStrategy.h"

#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

#include "ace/Log_Msg.h"

#include <sstream>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ShmemTransport::ShmemTransport(ShmemInst& inst)
  : TransportImpl(inst)
{
  if (! (configure_i(inst) && open()) ) {
    throw Transport::UnableToCreate();
  }
}

ShmemInst&
ShmemTransport::config() const
{
  return static_cast<ShmemInst&>(TransportImpl::config());
}

ShmemDataLink_rch
ShmemTransport::make_datalink(const std::string& remote_address)
{

  ShmemDataLink_rch link = make_rch<ShmemDataLink>(ref(*this));

  // Open logical connection:
  if (link->open(remote_address))
    return link;

  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
             ACE_TEXT("ShmemTransport::make_datalink: ")
             ACE_TEXT("failed to open DataLink!\n")));
  return ShmemDataLink_rch();
}

TransportImpl::AcceptConnectResult
ShmemTransport::connect_datalink(const RemoteTransport& remote,
                                 const ConnectionAttribs&,
                                 const TransportClient_rch&)
{
  const std::pair<std::string, std::string> key = blob_to_key(remote.blob_);
  if (key.first != this->config().hostname()) {
    return AcceptConnectResult();
  }
  GuardType guard(links_lock_);
  ShmemDataLinkMap::iterator iter = links_.find(key.second);
  if (iter != links_.end()) {
    ShmemDataLink_rch link = iter->second;
    VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) ShmemTransport::connect_datalink ")
              ACE_TEXT("link found.\n")), 2);
    return AcceptConnectResult(link);
  }
  VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) ShmemTransport::connect_datalink ")
            ACE_TEXT("new link.\n")), 2);
  return AcceptConnectResult(add_datalink(key.second));
}

DataLink_rch
ShmemTransport::add_datalink(const std::string& remote_address)
{
  ShmemDataLink_rch link = make_datalink(remote_address);
  links_.insert(ShmemDataLinkMap::value_type(remote_address, link));
  return link;
}

TransportImpl::AcceptConnectResult
ShmemTransport::accept_datalink(const RemoteTransport& remote,
                                const ConnectionAttribs& attribs,
                                const TransportClient_rch& client)
{
  return connect_datalink(remote, attribs, client);
}

void
ShmemTransport::stop_accepting_or_connecting(const TransportClient_wrch&, const RepoId&)
{
  // no-op: accept and connect either complete or fail immediately
}

bool
ShmemTransport::configure_i(ShmemInst& config)
{
  create_reactor_task();

#ifdef OPENDDS_SHMEM_UNSUPPORTED
  ACE_UNUSED_ARG(config);
  ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                    ACE_TEXT("ShmemTransport::configure_i: ")
                    ACE_TEXT("no platform support for shared memory!\n")),
                   false);
#else // ifdef OPENDDS_SHMEM_UNSUPPORTED

  ShmemAllocator::MEMORY_POOL_OPTIONS alloc_opts;
#  if defined OPENDDS_SHMEM_WINDOWS
  alloc_opts.max_size_ = config.pool_size_;
#  elif defined OPENDDS_SHMEM_UNIX
  alloc_opts.base_addr_ = 0;
  alloc_opts.segment_size_ = config.pool_size_;
  alloc_opts.minimum_bytes_ = alloc_opts.segment_size_;
  alloc_opts.max_segments_ = 1;
#  endif // if defined OPENDDS_SHMEM_WINDOWS

  alloc_.reset(
    new ShmemAllocator(ACE_TEXT_CHAR_TO_TCHAR(config.poolname().c_str()),
                       0 /*lock_name is optional*/, &alloc_opts));

  void* mem = alloc_->malloc(sizeof(ShmemSharedSemaphore));
  if (mem == 0) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTrasport::configure_i: failed to allocate")
                      ACE_TEXT(" space for semaphore in shared memory!\n")),
                     false);
  }

  ShmemSharedSemaphore* pSem = reinterpret_cast<ShmemSharedSemaphore*>(mem);
  alloc_->bind("Semaphore", pSem);

  bool ok;
#  if defined OPENDDS_SHMEM_WINDOWS
  *pSem = ::CreateSemaphoreW(0 /*default security*/,
                             0 /*initial count*/,
                             0x7fffffff /*max count (ACE's default)*/,
                             0 /*no name*/);
  ACE_sema_t ace_sema = *pSem;
  ok = (*pSem != 0);
#  elif defined OPENDDS_SHMEM_UNIX
  ok = (0 == ::sem_init(pSem, 1 /*process shared*/, 0 /*initial count*/));
  ACE_sema_t ace_sema;
  std::memset(&ace_sema, 0, sizeof ace_sema);
  ace_sema.sema_ = pSem;
#    ifdef OPENDDS_SHMEM_UNIX_EMULATE_SEM_TIMEOUT
  ace_sema.lock_ = PTHREAD_MUTEX_INITIALIZER;
  ace_sema.count_nonzero_ = PTHREAD_COND_INITIALIZER;
#    endif
#  endif // if defined OPENDDS_SHMEM_WINDOWS
  if (!ok) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::configure_i: ")
                      ACE_TEXT("could not create semaphore\n")),
                     false);
  }

  read_task_.reset(new ReadTask(this, ace_sema));

  VDBG_LVL((LM_INFO, "(%P|%t) ShmemTransport %@ configured with address %C\n",
            this, config.poolname().c_str()), 1);

  return true;
#endif // ifdef OPENDDS_SHMEM_UNSUPPORTED
}

void
ShmemTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:
  GuardType guard(links_lock_);
  if (read_task_) {
    read_task_->stop();
    read_task_->wait();
  }

  for (ShmemDataLinkMap::iterator it(links_.begin());
       it != links_.end(); ++it) {
    it->second->transport_shutdown();
  }
  links_.clear();

  read_task_.reset();

  if (alloc_) {
#ifndef OPENDDS_SHMEM_UNSUPPORTED
    void* mem = 0;
    alloc_->find("Semaphore", mem);
    ShmemSharedSemaphore* pSem = reinterpret_cast<ShmemSharedSemaphore*>(mem);
#  if defined OPENDDS_SHMEM_WINDOWS
    ::CloseHandle(*pSem);
#  elif defined OPENDDS_SHMEM_UNIX
    ::sem_destroy(pSem);
#  endif // if defined OPENDDS_SHMEM_WINDOWS
#endif // ifndef OPENDDS_SHMEM_UNSUPPORTED

    alloc_->release(1 /*close*/);
    alloc_.reset();
  }
}

bool
ShmemTransport::connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const
{
  config().populate_locator(info, flags);
  return true;
}

std::pair<std::string, std::string>
ShmemTransport::blob_to_key(const TransportBLOB& blob)
{
  const char* const c_str = reinterpret_cast<const char*>(blob.get_buffer());
  const std::string host(c_str);
  const size_t host_len = host.size();

  const std::string pool(c_str + host_len + 1, blob.length() - host_len - 1);
  return make_pair(host, pool);
}

void
ShmemTransport::release_datalink(DataLink* link)
{
  GuardType guard(links_lock_);
  for (ShmemDataLinkMap::iterator it(links_.begin());
       it != links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (link == static_cast<DataLink*>(it->second.in())) {
      link->stop();
      links_.erase(it);
      return;
    }
  }
}

ShmemTransport::ReadTask::ReadTask(ShmemTransport* outer, ACE_sema_t semaphore)
  : outer_(outer)
  , semaphore_(semaphore)
  , stopped_(false)
{
  activate();
}

int
ShmemTransport::ReadTask::svc()
{
  while (!stopped_) {
    ACE_OS::sema_wait(&semaphore_);
    if (stopped_) {
      return 0;
    }
    outer_->read_from_links();
  }
  return 0;
}

void
ShmemTransport::ReadTask::stop()
{
  if (stopped_) {
    return;
  }
  stopped_ = true;
  ACE_OS::sema_post(&semaphore_);
  wait();
}

void
ShmemTransport::ReadTask::signal_semaphore()
{
  if (stopped_) {
    return;
  }
  ACE_OS::sema_post(&semaphore_);
}

void
ShmemTransport::read_from_links()
{
  std::vector<ShmemDataLink_rch> dl_copies;
  {
    GuardType guard(links_lock_);
    typedef ShmemDataLinkMap::iterator iter_t;
    for (iter_t it = links_.begin(); it != links_.end(); ++it) {
      dl_copies.push_back(it->second);
    }
  }

  typedef std::vector<ShmemDataLink_rch>::iterator dl_iter_t;
  for (dl_iter_t dl_it = dl_copies.begin(); !is_shut_down() && dl_it != dl_copies.end(); ++dl_it) {
    dl_it->in()->read();
  }
}

void
ShmemTransport::signal_semaphore()
{
  if (is_shut_down()) {
    return;
  }
  read_task_->signal_semaphore();
}

std::string
ShmemTransport::address()
{
  return this->config().poolname();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
