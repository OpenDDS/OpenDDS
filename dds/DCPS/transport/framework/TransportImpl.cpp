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
#include "dds/DCPS/ServiceEventDispatcher.h"
#include "tao/debug.h"
#include "dds/DCPS/SafetyProfileStreams.h"

#if !defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransportImpl::TransportImpl(TransportInst_rch config,
                             DDS::DomainId_t domain)
  : config_(config)
  , event_dispatcher_(make_rch<ServiceEventDispatcher>(1))
  , is_shut_down_(false)
  , domain_(domain)
{
  DBG_ENTRY_LVL("TransportImpl", "TransportImpl", 6);
  if (TheServiceParticipant->monitor_factory_) {
    monitor_.reset(TheServiceParticipant->monitor_factory_->create_transport_monitor(this));
  }
}

TransportImpl::~TransportImpl()
{
  DBG_ENTRY_LVL("TransportImpl", "~TransportImpl", 6);
  event_dispatcher_->shutdown(true);
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

  if (!this->reactor_task_.is_nil()) {
    this->reactor_task_->stop();
  }

  event_dispatcher_->shutdown(true);

  // Tell our subclass about the "shutdown event".
  this->shutdown_i();
}


bool
TransportImpl::open()
{
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

  if (reactor_task_->open_reactor_task(&TheServiceParticipant->get_thread_status_manager(), name)) {
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

  DataLink_rch link_rch = rchandle_from(link);
  EventBase_rch do_clear = make_rch<DoClear>(link_rch);
  event_dispatcher_->dispatch(do_clear);
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
  TransportInst_rch cfg = config_.lock();
  return cfg ? cfg->dump_to_str(domain_) : OPENDDS_STRING();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
