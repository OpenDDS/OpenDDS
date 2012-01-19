/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportImpl.h"
#include "DataLink.h"
#include "TransportExceptions.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/MonitorFactory.h"
#include "dds/DCPS/Service_Participant.h"
#include "tao/debug.h"
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportImpl::TransportImpl()
  : monitor_(0)
{
  DBG_ENTRY_LVL("TransportImpl", "TransportImpl", 6);
  if (TheServiceParticipant->monitor_factory_) {
    monitor_ = TheServiceParticipant->monitor_factory_->create_transport_monitor(this);
  }
}

TransportImpl::~TransportImpl()
{
  DBG_ENTRY_LVL("TransportImpl", "~TransportImpl", 6);
}

void
TransportImpl::shutdown()
{
  DBG_ENTRY_LVL("TransportImpl", "shutdown", 6);

  // Stop datalink clean task.
  this->dl_clean_task_.close(1);

  if (!this->reactor_task_.is_nil()) {
    this->reactor_task_->stop();
  }

  this->pre_shutdown_i();

  std::set<TransportClient*> local_clients;

  {
    GuardType guard(this->lock_);

    if (this->config_.is_nil()) {
      // This TransportImpl is already shutdown.
//MJM: So, I read here that config_i() actually "starts" us?
      return;
    }

    local_clients.swap(this->clients_);

    // We can release our lock_ now.
  }

  for (std::set<TransportClient*>::iterator it = local_clients.begin();
       it != local_clients.end(); ++it) {
    (*it)->transport_detached(this);
  }

  // Tell our subclass about the "shutdown event".
  this->shutdown_i();

  {
    GuardType guard(this->lock_);
    this->reactor_task_ = 0;
    // The shutdown_i() path may access the configuration so remove configuration
    // reference after shutdown is performed.

    // Drop our references to the config_.
    this->config_ = 0;
  }
}

bool
TransportImpl::configure(TransportInst* config)
{
  DBG_ENTRY_LVL("TransportImpl","configure",6);

  GuardType guard(this->lock_);

  if (config == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: invalid configuration.\n"),
                     false);
  }

  if (!this->config_.is_nil()) {
    // We are rejecting this configuration attempt since this
    // TransportImpl object has already been configured.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TransportImpl already configured.\n"),
                     false);
  }

  config->_add_ref();
  this->config_ = config;

  // Let our subclass take a shot at the configuration object.
  if (this->configure_i(config) == false) {
    if (Transport_debug_level > 0) {
      dump();
    }

    this->config_ = 0;

    // The subclass rejected the configuration attempt.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TransportImpl configuration failed.\n"),
                     false);
  }

  // Open the DL Cleanup task
  // We depend upon the existing config logic to ensure the
  // DL Cleanup task is opened only once
  if (this->dl_clean_task_.open()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: DL Cleanup task failed to open : %p\n",
                      ACE_TEXT("open")), false);
  }

  // Success.
  if (this->monitor_) {
    this->monitor_->report();
  }

  if (Transport_debug_level > 0) {
    std::stringstream os;
    dump(os);

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportImpl::configure()\n%C"),
               os.str().c_str()));
  }

  return true;
}

void
TransportImpl::create_reactor_task()
{
  if (this->reactor_task_.in()) {
    return;
  }

  this->reactor_task_ = new TransportReactorTask;
  if (0 != this->reactor_task_->open(0)) {
    throw Transport::MiscProblem(); // error already logged by TRT::open()
  }
}

void
TransportImpl::attach_client(TransportClient* client)
{
  DBG_ENTRY_LVL("TransportImpl", "attach_client", 6);

  GuardType guard(this->lock_);
  clients_.insert(client);
}

void
TransportImpl::detach_client(TransportClient* client)
{
  DBG_ENTRY_LVL("TransportImpl", "detach_client", 6);

  GuardType guard(this->lock_);
  clients_.erase(client);
}

DataLink*
TransportImpl::find_connect_i(const RepoId& local_id,
                              const AssociationData& remote_association,
                              const ConnectionAttribs& attribs,
                              bool active, bool connect)
{
  const CORBA::ULong num_blobs = remote_association.remote_data_.length();
  const std::string ttype = transport_type();

  for (CORBA::ULong idx = 0; idx < num_blobs; ++idx) {
    if (remote_association.remote_data_[idx].transport_type.in() == ttype) {
      DataLink_rch link;
      if (connect) {
        link = connect_datalink_i(local_id, remote_association.remote_id_,
                                  remote_association.remote_data_[idx].data,
                                  remote_association.remote_reliable_, attribs);
      } else {
        link = find_datalink_i(local_id, remote_association.remote_id_,
                               remote_association.remote_data_[idx].data,
                               remote_association.remote_reliable_,
                               attribs, active);
      }
      if (!link.is_nil()) {
        return link._retn();
      }
    }
  }
  return 0;
}

DataLink*
TransportImpl::find_datalink(const RepoId& local_id,
                             const AssociationData& remote_association,
                             const ConnectionAttribs& attribs,
                             bool active)
{
  return find_connect_i(local_id, remote_association, attribs, active, false);
}

DataLink*
TransportImpl::connect_datalink(const RepoId& local_id,
                                const AssociationData& remote_association,
                                const ConnectionAttribs& attribs)
{
  return find_connect_i(local_id, remote_association, attribs, true, true);
}

TransportImpl::ConnectionEvent::ConnectionEvent(const RepoId& local_id,
                                  const AssociationData& remote_association,
                                  const ConnectionAttribs& attribs)
  : local_id_(local_id)
  , remote_association_(remote_association)
  , attribs_(attribs)
  , cond_(mtx_)
{}

void
TransportImpl::ConnectionEvent::wait(const ACE_Time_Value& timeout)
{
  ACE_Time_Value deadline;
  ACE_Time_Value* p_deadline = 0;
  if (timeout != ACE_Time_Value::zero) {
    deadline = ACE_OS::gettimeofday() + timeout;
    p_deadline = &deadline;
  }

  ACE_GUARD(ACE_Thread_Mutex, g, mtx_);
  while (link_.is_nil()) {
    if (cond_.wait(p_deadline) == -1) {
      return;
    }
  }
}

bool
TransportImpl::ConnectionEvent::complete(const DataLink_rch& link)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mtx_, false);
  if (link_.is_nil()) {
    link_ = link;
    cond_.signal();
    return true;
  }
  return false;
}

bool
TransportImpl::release_link_resources(DataLink* link)
{
  DBG_ENTRY_LVL("TransportImpl", "release_link_resources",6);

  // Create a smart pointer without ownership (bumps up ref count)
  DataLink_rch dl(link, false);

  dl_clean_task_.add(dl);

  return true;
}

void
TransportImpl::report()
{
  if (this->monitor_) {
    this->monitor_->report();
  }
}

void
TransportImpl::dump()
{
  std::stringstream os;
  dump(os);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) TransportImpl::dump() -\n%C"),
             os.str().c_str()));
}

void
TransportImpl::dump(ostream& os)
{
  if (this->config_.is_nil()) {
    os << " (not configured)" << std::endl;
  } else {
    this->config_->dump(os);
  }
}

}
}
