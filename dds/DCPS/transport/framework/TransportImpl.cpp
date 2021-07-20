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

TransportImpl::TransportImpl(TransportInst& config)
  : config_(config)
  , last_link_(0)
  , is_shut_down_(false)
{
  DBG_ENTRY_LVL("TransportImpl", "TransportImpl", 6);
  if (TheServiceParticipant->monitor_factory_) {
    monitor_.reset(TheServiceParticipant->monitor_factory_->create_transport_monitor(this));
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

  // Tell our subclass about the "shutdown event".
  this->shutdown_i();
}


bool
TransportImpl::open()
{
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
               ACE_TEXT("(%P|%t) TransportImpl::open()\n%C"),
               dump_to_str().c_str()));
  }

  return true;
}

void
TransportImpl::add_pending_connection(const TransportClient_rch& client, DataLink_rch link)
{
  GuardType guard(pending_connections_lock_);
  pending_connections_.insert( PendConnMap::value_type(client, link) );
}

void
TransportImpl::create_reactor_task(bool useAsyncSend, const OPENDDS_STRING& name)
{
  if (is_shut_down_ || this->reactor_task_.in()) {
    return;
  }

  this->reactor_task_= make_rch<ReactorTask>(useAsyncSend);

  if (reactor_task_->open_reactor_task(0, TheServiceParticipant->get_thread_status_interval(),
      TheServiceParticipant->get_thread_status_manager(), name.c_str())) {
    throw Transport::MiscProblem(); // error already logged by TRT::open()
  }
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
  return config_.dump_to_str();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
