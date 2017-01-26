/*
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
#include "dds/DCPS/SafetyProfileStreams.h"

#if !defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransportImpl::TransportImpl()
  : monitor_(0),
    last_link_(0),
    is_shut_down_(false)
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

bool
TransportImpl::is_shut_down() const
{
  return is_shut_down_;
}

void
TransportImpl::shutdown()
{
  DBG_ENTRY_LVL("TransportImpl", "shutdown", 6);

  is_shut_down_ = true;

  // Stop datalink clean task.
  this->dl_clean_task_.close(1);

  if (!this->reactor_task_.is_nil()) {
    this->reactor_task_->stop();
  }

  this->pre_shutdown_i();

  OPENDDS_SET(TransportClient_rch) local_clients;

  {
    GuardType guard(this->lock_);

    local_clients.swap(this->clients_);
    // We can release our lock_ now.
  }

  for (OPENDDS_SET(TransportClient_rch)::iterator it = local_clients.begin();
       it != local_clients.end(); ++it) {
    (*it)->transport_detached(this);
  }

  // Tell our subclass about the "shutdown event".
  this->shutdown_i();

  {
    GuardType guard(this->lock_);
    this->reactor_task_.reset();
  }
}

bool
TransportImpl::configure(const TransportInst_rch& config)
{
  DBG_ENTRY_LVL("TransportImpl","configure",6);

  GuardType guard(this->lock_);

  if (!config) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: invalid configuration.\n"),
                     false);
  }

  if (this->config_) {
    // We are rejecting this configuration attempt since this
    // TransportImpl object has already been configured.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TransportImpl already configured.\n"),
                     false);
  }

  this->config_ = config;

  // Let our subclass take a shot at the configuration object.
  if (this->configure_i(config.in()) == false) {
    if (Transport_debug_level > 0) {
      dump();
    }

    guard.release();
    shutdown();

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

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportImpl::configure()\n%C"),
               dump_to_str().c_str()));
  }


  return true;
}

void
TransportImpl::add_pending_connection(const TransportClient_rch& client, DataLink_rch link)
{
  pending_connections_.insert( PendConnMap::value_type(client, link));
}

void
TransportImpl::create_reactor_task(bool useAsyncSend)
{
  if (this->reactor_task_.in()) {
    return;
  }

  this->reactor_task_= make_rch<TransportReactorTask>(useAsyncSend);
  if (0 != this->reactor_task_->open(0)) {
    throw Transport::MiscProblem(); // error already logged by TRT::open()
  }
}

void
TransportImpl::attach_client(const TransportClient_rch& client)
{
  DBG_ENTRY_LVL("TransportImpl", "attach_client", 6);

  GuardType guard(this->lock_);
  clients_.insert(client);
}

void
TransportImpl::detach_client(const TransportClient_rch& client)
{
  DBG_ENTRY_LVL("TransportImpl", "detach_client", 6);

  pre_detach(client);
  GuardType guard(this->lock_);
  clients_.erase(client);
}

void
TransportImpl::unbind_link(DataLink*)
{
  // may be overridden by subclass
  DBG_ENTRY_LVL("TransportImpl", "unbind_link",6);
}

bool
TransportImpl::release_link_resources(DataLink* link)
{
  DBG_ENTRY_LVL("TransportImpl", "release_link_resources",6);

  // Create a smart pointer without ownership (bumps up ref count)
  dl_clean_task_.add(rchandle_from(link));

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
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) TransportImpl::dump() -\n%C"),
             dump_to_str().c_str()));
}

OPENDDS_STRING
TransportImpl::dump_to_str()
{
  if (this->config_.is_nil()) {
    return OPENDDS_STRING(" (not configured)\n");
  } else {
    return this->config_->dump_to_str();
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
