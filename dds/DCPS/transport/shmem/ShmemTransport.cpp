/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemTransport.h"

#include "ShmemInst.h"
#include "ShmemSendStrategy.h"
#include "ShmemReceiveStrategy.h"

#include <dds/DCPS/debug.h>
#include <dds/DCPS/AssociationData.h>
#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include <dds/DCPS/transport/framework/TransportClient.h>

#include <ace/Log_Msg.h>

#include <sstream>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ShmemTransport::ShmemTransport(const ShmemInst_rch& inst,
                                 DDS::DomainId_t domain)
  : TransportImpl(inst, domain)
{
  if (!(configure_i(inst) && open())) {
    throw Transport::UnableToCreate();
  }
}

ShmemInst_rch
ShmemTransport::config() const
{
  return dynamic_rchandle_cast<ShmemInst>(TransportImpl::config());
}

ShmemDataLink_rch
ShmemTransport::make_datalink(const std::string& remote_address)
{
  ShmemDataLink_rch link = make_rch<ShmemDataLink>(rchandle_from(this));

  if (!link->open(remote_address)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ShmemTransport::make_datalink: "
        "failed to open DataLink!\n"));
    }
    return ShmemDataLink_rch();
  }

  if (!links_.insert(ShmemDataLinkMap::value_type(remote_address, link)).second) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ShmemTransport::make_datalink: "
        "there is an existing link for %C!\n", remote_address.c_str()));
    }
    return ShmemDataLink_rch();
  }

  return link;
}

ShmemDataLink_rch ShmemTransport::get_or_make_datalink(
  const char* caller, const RemoteTransport& remote)
{
  const std::pair<std::string, std::string> key = blob_to_key(remote.blob_);
  ShmemInst_rch cfg = config();
  if (!cfg || key.first != cfg->hostname()) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ShmemTransport::get_or_make_datalink: "
                 "%C link %C:%C not found, hostname %C.\n",
                 caller, key.first.c_str(), key.second.c_str(),
                 cfg ? cfg->hostname().c_str() : "(no config)"));
    }
    return ShmemDataLink_rch();
  }

  GuardType guard(links_lock_);
  ShmemDataLinkMap::iterator iter = links_.find(key.second);
  const bool make = iter == links_.end();
  VDBG_LVL((LM_DEBUG, "(%P|%t) ShmemTransport::get_or_make_datalink: %C using %C link %C:%C\n",
            caller, make ? "new" : "existing", key.first.c_str(), key.second.c_str()), 2);
  return make ? make_datalink(key.second) : iter->second;
}

TransportImpl::AcceptConnectResult
ShmemTransport::connect_datalink(const RemoteTransport& remote,
                                 const ConnectionAttribs&,
                                 const TransportClient_rch& client)
{
  ShmemDataLink_rch link = get_or_make_datalink("connect_datalink", remote);
  if (!link) {
    return AcceptConnectResult();
  }
  // Wait for invoke_on_start_callbacks to actually start a writer. This is done
  // when the writer gets an association message from the reader in the writer
  // case of ShmemDataLink::request_ack_received.
  link->add_on_start_callback(client, remote.repo_id_);
  add_pending_connection(client, link);
  return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
}

TransportImpl::AcceptConnectResult
ShmemTransport::accept_datalink(const RemoteTransport& remote,
                                const ConnectionAttribs& /*attribs*/,
                                const TransportClient_rch& /*client*/)
{
  return AcceptConnectResult(get_or_make_datalink("accept_datalink", remote));
}

void ShmemTransport::stop_accepting_or_connecting(
  const TransportClient_wrch& client, const GUID_t& remote_id,
  bool /*disassociate*/, bool /*association_failed*/)
{
  ACE_GUARD(ACE_Thread_Mutex, links_guard, links_lock_);
  ACE_GUARD(ACE_Thread_Mutex, pending_guard, pending_connections_lock_);
  typedef PendConnMap::iterator Iter;
  const std::pair<Iter, Iter> range = pending_connections_.equal_range(client);
  for (Iter iter = range.first; iter != range.second; ++iter) {
    ShmemDataLink_rch sdl = dynamic_rchandle_cast<ShmemDataLink>(iter->second);
    TransportClient_rch tc = client.lock();
    if (tc) {
      sdl->stop_resend_association_msgs(tc->get_guid(), remote_id);
    }
  }
  pending_connections_.erase(range.first, range.second);
}

bool
ShmemTransport::configure_i(const ShmemInst_rch& config)
{
  if (!config) {
    return false;
  }

  create_reactor_task(false, "ShmemTransport" + config->name());

#ifdef OPENDDS_SHMEM_UNSUPPORTED
  ACE_UNUSED_ARG(config);
  if (log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ShmemTransport::configure_i: "
               "no platform support for shared memory!\n"));
  }
  return false;
#else /* OPENDDS_SHMEM_UNSUPPORTED */

  ShmemAllocator::MEMORY_POOL_OPTIONS alloc_opts;
#  if defined OPENDDS_SHMEM_WINDOWS
  alloc_opts.max_size_ = config->pool_size();
#  elif defined OPENDDS_SHMEM_UNIX
  alloc_opts.base_addr_ = 0;
  alloc_opts.segment_size_ = config->pool_size();
  alloc_opts.minimum_bytes_ = alloc_opts.segment_size_;
  alloc_opts.max_segments_ = 1;
#  endif /* OPENDDS_SHMEM_WINDOWS */

  alloc_.reset(
    new ShmemAllocator(ACE_TEXT_CHAR_TO_TCHAR(config->poolname().c_str()),
                       0 /*lock_name is optional*/, &alloc_opts));

  void* mem = alloc_->malloc(sizeof(ShmemSharedSemaphore));
  if (mem == 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ShmemTransport::configure_i: failed to allocate"
                 " space for semaphore in shared memory!\n"));
    }
    return false;
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
#  endif /* OPENDDS_SHMEM_WINDOWS */
  if (!ok) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::configure_i: ")
                      ACE_TEXT("could not create semaphore\n")),
                     false);
  }

  read_task_.reset(new ReadTask(this, ace_sema));

  VDBG_LVL((LM_DEBUG, "(%P|%t) ShmemTransport %@ configured with address %C\n",
            this, config->poolname().c_str()), 1);

  return true;
#endif /* OPENDDS_SHMEM_UNSUPPORTED */
}

void
ShmemTransport::shutdown_i()
{
  DBG_ENTRY_LVL("ShmemTransport","shutdown_i",6);

  if (read_task_) {
    read_task_->stop();
    ThreadStatusManager::Sleeper s(TheServiceParticipant->get_thread_status_manager());
    read_task_->wait();
  }

  // Shutdown reserved datalinks and release configuration:
  GuardType guard(links_lock_);
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
#  endif /* OPENDDS_SHMEM_WINDOWS */
#endif /* OPENDDS_SHMEM_UNSUPPORTED */

    if (alloc_->release(1 /*close*/) == -1) {
      VDBG_LVL((LM_ERROR,
                "(%P|%t) ShmemTransport::shutdown_i Release shared memory failed\n"), 1);
    }

    alloc_.reset();
  }
}

bool
ShmemTransport::connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const
{
  ShmemInst_rch cfg = config();
  if (cfg) {
    cfg->populate_locator(info, flags, domain_);
    return true;
  }
  return false;
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
  DBG_ENTRY_LVL("ShmemTransport", "release_datalink", 6);

  GuardType guard(links_lock_);
  for (ShmemDataLinkMap::iterator it(links_.begin());
       it != links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (link == static_cast<DataLink*>(it->second.in())) {
      VDBG_LVL((LM_DEBUG,
                "(%P|%t) ShmemTransport::release_datalink link[%@]\n",
                link), 2);

      link->stop();
      links_.erase(it);
      return;
    }
  }

  VDBG_LVL((LM_ERROR,
            "(%P|%t) ShmemTransport::release_datalink link[%@] not found in ShmemDataLinkMap\n",
            link), 1);
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
  ThreadStatusManager::Start s(TheServiceParticipant->get_thread_status_manager(), "ShmemTransport");

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
  ThreadStatusManager::Sleeper s(TheServiceParticipant->get_thread_status_manager());
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
  ShmemInst_rch cfg = config();
  return cfg ? cfg->poolname() : std::string();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
