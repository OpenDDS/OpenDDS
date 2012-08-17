/*
 * $Id$
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

namespace OpenDDS {
namespace DCPS {

ShmemTransport::ShmemTransport(const TransportInst_rch& inst)
  : alloc_(0)
  , read_task_(0)
  , hostname_(get_fully_qualified_hostname())
{
  if (!inst.is_nil()) {
    if (!configure(inst.in())) {
      throw Transport::UnableToCreate();
    }
  }
}

ShmemDataLink*
ShmemTransport::make_datalink(const std::string& remote_address)
{
  ShmemDataLink_rch link;
  ACE_NEW_RETURN(link, ShmemDataLink(this), 0);

  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::make_datalink: ")
                      ACE_TEXT("failed to create DataLink!\n")),
                     0);
  }

  link->configure(config_i_.in());

  // Assign send strategy:
  ShmemSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, ShmemSendStrategy(link.in()), 0);
  link->send_strategy(send_strategy);

  // Assign receive strategy:
  ShmemReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, ShmemReceiveStrategy(link.in()), 0);
  link->receive_strategy(recv_strategy);

  // Open logical connection:
  if (!link->open(remote_address)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink!\n")),
                     0);
  }

  return link._retn();
}

DataLink*
ShmemTransport::find_datalink_i(const RepoId& /*local_id*/,
                                const RepoId& /*remote_id*/,
                                const TransportBLOB& remote_data,
                                bool /*remote_reliable*/,
                                const ConnectionAttribs& /*attribs*/,
                                bool /*active*/)
{
  const std::pair<std::string, std::string> key = blob_to_key(remote_data);
  if (key.first != hostname_) {
    return 0;
  }

  GuardType guard(links_lock_);
  ShmemDataLinkMap::iterator iter = links_.find(key.second);
  if (iter != links_.end()) {
    ShmemDataLink_rch link = iter->second;
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) ShmemTransport::find_datalink_i ")
              ACE_TEXT("link found, waiting for start.\n")), 2);
    guard.release();
    link->wait_for_start();
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) ShmemTransport::find_datalink_i ")
              ACE_TEXT("done waiting for start.\n")), 2);
    return link._retn();
  }
  return 0;
}

DataLink*
ShmemTransport::connect_datalink_i(const RepoId& /*local_id*/,
                                   const RepoId& /*remote_id*/,
                                   const TransportBLOB& remote_data,
                                   bool /*remote_reliable*/,
                                   const ConnectionAttribs& /*attribs*/)
{
  const std::pair<std::string, std::string> key = blob_to_key(remote_data);
  if (key.first != hostname_) {
    return 0;
  }

  return add_datalink(key.second);
}

DataLink*
ShmemTransport::add_datalink(const std::string& remote_address)
{
  ShmemDataLink_rch link = make_datalink(remote_address);
  {
    GuardType guard(links_lock_);
    std::pair<ShmemDataLinkMap::iterator, bool> result =
      links_.insert(ShmemDataLinkMap::value_type(remote_address, link));
    if (!result.second) { // another thread inserted before us
      link = result.first->second;
      guard.release();
      link->wait_for_start();
    }
  }
  return link._retn();
}

DataLink*
ShmemTransport::accept_datalink(ConnectionEvent& ce)
{
  const std::string ttype = "shmem";
  const CORBA::ULong num_blobs = ce.remote_association_.remote_data_.length();

  for (CORBA::ULong idx = 0; idx < num_blobs; ++idx) {
    if (ce.remote_association_.remote_data_[idx].transport_type.in() == ttype) {
      const std::pair<std::string, std::string> key =
        blob_to_key(ce.remote_association_.remote_data_[idx].data);
      if (key.first == hostname_) {
        return add_datalink(key.second);
      }
    }
  }

  return 0;
}

void
ShmemTransport::stop_accepting(ConnectionEvent& /*ce*/)
{
  // no-op: accept_datalink() completes or fails immediately
}

bool
ShmemTransport::configure_i(TransportInst* config)
{
#if (!defined ACE_WIN32 && defined ACE_LACKS_SYSV_SHMEM) || defined ACE_HAS_WINCE
  ACE_UNUSED_ARG(config);
  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: ")
                    ACE_TEXT("ShmemTransport::configure_i: ")
                    ACE_TEXT("no platform support for shared memory!\n")),
                   false);
#else
  config_i_ = dynamic_cast<ShmemInst*>(config);
  if (config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }
  config_i_->_add_ref();

  std::ostringstream pool;
  pool << "OpenDDS-" << ACE_OS::getpid() << '-' << config->name();
  poolname_ = pool.str();

  ShmemAllocator::MEMORY_POOL_OPTIONS alloc_opts;
#ifdef ACE_WIN32
  alloc_opts.max_size_ = config_i_->pool_size_;
#elif !defined ACE_LACKS_SYSV_SHMEM
  alloc_opts.segment_size_ = config_i_->pool_size_;
  alloc_opts.minimum_bytes_ = alloc_opts.segment_size_;
  alloc_opts.max_segments_ = 1;
#endif

  alloc_ =
    new ShmemAllocator(ACE_TEXT_CHAR_TO_TCHAR(poolname_.c_str()),
                       0 /*lock_name is optional*/, &alloc_opts);

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
# ifdef ACE_WIN32
  *pSem = ::CreateSemaphoreW(0 /*default security*/,
                             0 /*initial count*/,
                             0x7fffffff /*max count (ACE's default)*/,
                             0 /*no name*/);
  ACE_sema_t ace_sema = *pSem;
  ok = (*pSem != 0);
# else
  ok = (0 == ::sem_init(pSem, 1 /*process shared*/, 0 /*initial count*/));
  ACE_sema_t ace_sema = {pSem, 0 /*no name*/
#  if !defined (ACE_HAS_POSIX_SEM_TIMEOUT) && !defined (ACE_DISABLE_POSIX_SEM_TIMEOUT_EMULATION)
                         , PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER
#  endif
  };
# endif
  if (!ok) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::configure_i: ")
                      ACE_TEXT("could not create semaphore\n")),
                     false);
  }

  read_task_ = new ReadTask(this, ace_sema);

  VDBG_LVL((LM_INFO, "(%P|%t) ShmemTransport %@ configured with address %C\n",
            this, poolname_.c_str()), 1);

  return true;
#endif
}

void
ShmemTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:
  GuardType guard(links_lock_);
  for (ShmemDataLinkMap::iterator it(links_.begin());
       it != links_.end(); ++it) {
    it->second->transport_shutdown();
  }
  links_.clear();

  read_task_->stop();
  delete read_task_;
  read_task_ = 0;

  void* mem = 0;
  alloc_->find("Semaphore", mem);
  ShmemSharedSemaphore* pSem = reinterpret_cast<ShmemSharedSemaphore*>(mem);
#ifdef ACE_WIN32
  ::CloseHandle(*pSem);
#elif defined ACE_HAS_POSIX_SEM
  ::sem_destroy(pSem);
#else
  ACE_UNUSED_ARG(pSem);
#endif

  alloc_->release(1 /*close*/);
  delete alloc_;
  alloc_ = 0;

  config_i_ = 0;
}

bool
ShmemTransport::connection_info_i(TransportLocator& info) const
{
  info.transport_type = "shmem";

  const size_t len = hostname_.size() + 1 /* null */ + poolname_.size();
  info.data.length(static_cast<CORBA::ULong>(len));

  CORBA::Octet* buff = info.data.get_buffer();
  std::memcpy(buff, hostname_.c_str(), hostname_.size());
  buff += hostname_.size();

  *(buff++) = 0;
  std::memcpy(buff, poolname_.c_str(), poolname_.size());
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
  while (true) {
    ACE_OS::sema_wait(&semaphore_);
    if (stopped_) {
      return 0;
    }
    outer_->read_from_links();
  }
  return 1;
}

void
ShmemTransport::ReadTask::stop()
{
  stopped_ = true;
  ACE_OS::sema_post(&semaphore_);
  wait();
}

void
ShmemTransport::read_from_links()
{
  GuardType guard(links_lock_);
  typedef ShmemDataLinkMap::iterator iter_t;
  for (iter_t it = links_.begin(); it != links_.end(); ++it) {
    it->second->read();
  }
}

void
ShmemTransport::signal_semaphore()
{
  ACE_OS::sema_post(&read_task_->semaphore_);
}

} // namespace DCPS
} // namespace OpenDDS
