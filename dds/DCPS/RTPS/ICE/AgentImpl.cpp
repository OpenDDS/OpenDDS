/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "AgentImpl.h"

#include "Task.h"
#include "EndpointManager.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/TimeTypes.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

#ifdef OPENDDS_SECURITY

using DCPS::TimeDuration;
using DCPS::MonotonicTimePoint;

struct ScheduleTimerCommand : public DCPS::ReactorInterceptor::Command {
  ACE_Reactor* reactor;
  ACE_Event_Handler* event_handler;
  TimeDuration delay;

  ScheduleTimerCommand(ACE_Reactor* a_reactor, ACE_Event_Handler* a_event_handler, const TimeDuration& a_delay) :
    reactor(a_reactor), event_handler(a_event_handler), delay(a_delay) {}

  void execute()
  {
    reactor->cancel_timer(event_handler, 0);
    reactor->schedule_timer(event_handler, 0, delay.value());
  }
};

void AgentImpl::enqueue(const DCPS::MonotonicTimePoint& a_release_time,
                        WeakTaskPtr wtask)
{
  if (tasks_.empty() || a_release_time < tasks_.top().release_time_) {
    const MonotonicTimePoint release = std::max(last_execute_ + ICE::Configuration::instance()->T_a(), a_release_time);
    execute_or_enqueue(DCPS::make_rch<ScheduleTimerCommand>(reactor(), this, release - MonotonicTimePoint::now()));
  }
  tasks_.push(Item(a_release_time, wtask));
}

bool AgentImpl::reactor_is_shut_down() const
{
  return TheServiceParticipant->is_shut_down();
}

int AgentImpl::handle_timeout(const ACE_Time_Value& a_now, const void* /*act*/)
{
  DCPS::ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  const MonotonicTimePoint now(a_now);
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, mutex, 0);
  check_invariants();

  if (tasks_.empty()) {
    return 0;
  }

  if (tasks_.top().release_time_ <= now) {
    Item item = tasks_.top();
    tasks_.pop();

    TaskPtr task = item.task_.lock();
    if (task) {
      task->execute(now);
      last_execute_ = now;
    }
  }
  process_deferred();
  check_invariants();

  if (!tasks_.empty()) {
    const MonotonicTimePoint release = std::max(last_execute_ + ICE::Configuration::instance()->T_a(), tasks_.top().release_time_);
    execute_or_enqueue(DCPS::make_rch<ScheduleTimerCommand>(reactor(), this, release - now));
  }

  return 0;
}

AgentImpl::AgentImpl()
  : ReactorInterceptor(TheServiceParticipant->reactor(), TheServiceParticipant->reactor_owner())
  , DCPS::InternalDataReaderListener<DCPS::NetworkInterfaceAddress>(TheServiceParticipant->job_queue())
  , unfreeze_(false)
  , reader_(DCPS::make_rch<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> >(true, DCPS::rchandle_from(this)))
  , reader_added_(false)
  , remote_peer_reflexive_counter_(0)
{
  // Bind the lifetime of this to the service participant.
  TheServiceParticipant->set_shutdown_listener(DCPS::static_rchandle_cast<ShutdownListener>(rchandle_from(this)));
}

void AgentImpl::add_endpoint(DCPS::WeakRcHandle<Endpoint> a_endpoint)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();

  if (endpoint_managers_.find(a_endpoint) == endpoint_managers_.end()) {
    EndpointManagerPtr em = DCPS::make_rch<EndpointManager>(this, a_endpoint);
    endpoint_managers_[a_endpoint] = em;
  }

  check_invariants();

  if (!endpoint_managers_.empty() && !reader_added_) {
    TheServiceParticipant->network_interface_address_topic()->connect(reader_);
    reader_added_ = true;
  }
}

void AgentImpl::remove_endpoint(DCPS::WeakRcHandle<Endpoint> a_endpoint)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();

  EndpointManagerMapType::iterator pos = endpoint_managers_.find(a_endpoint);

  if (pos != endpoint_managers_.end()) {
    EndpointManagerPtr em = pos->second;
    em->purge();
    endpoint_managers_.erase(pos);
  }

  check_invariants();

  if (endpoint_managers_.empty() && reader_added_) {
    TheServiceParticipant->network_interface_address_topic()->disconnect(reader_);
    reader_added_ = false;
  }
}

AgentInfo AgentImpl::get_local_agent_info(DCPS::WeakRcHandle<Endpoint> a_endpoint) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, mutex, AgentInfo());
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  return pos->second->agent_info();
}

void AgentImpl::add_local_agent_info_listener(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                              const DCPS::RepoId& a_local_guid,
                                              DCPS::WeakRcHandle<AgentInfoListener> a_agent_info_listener)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->add_agent_info_listener(a_local_guid, a_agent_info_listener);
}

void AgentImpl::remove_local_agent_info_listener(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                                 const DCPS::RepoId& a_local_guid)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->remove_agent_info_listener(a_local_guid);
}

void  AgentImpl::start_ice(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                           const DCPS::RepoId& a_local_guid,
                           const DCPS::RepoId& a_remote_guid,
                           const AgentInfo& a_remote_agent_info)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->start_ice(a_local_guid, a_remote_guid, a_remote_agent_info);
  check_invariants();
}

void AgentImpl::stop_ice(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                         const DCPS::RepoId& a_local_guid,
                         const DCPS::RepoId& a_remote_guid)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->stop_ice(a_local_guid, a_remote_guid);
  check_invariants();
}

ACE_INET_Addr  AgentImpl::get_address(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                      const DCPS::RepoId& a_local_guid,
                                      const DCPS::RepoId& a_remote_guid) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, mutex, ACE_INET_Addr());
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  return pos->second->get_address(a_local_guid, a_remote_guid);
}

// Receive a STUN message.
void  AgentImpl::receive(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                         const ACE_INET_Addr& a_local_address,
                         const ACE_INET_Addr& a_remote_address,
                         const STUN::Message& a_message)
{
  if (a_local_address.is_any()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) AgentImpl::receive: ERROR local_address is empty, ICE will not work on this platform\n")));
    return;
  }

  if (a_remote_address.is_any()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) AgentImpl::receive: ERROR remote_address is empty, ICE will not work on this platform\n")));
    return;
  }

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->receive(a_local_address, a_remote_address, a_message);
  process_deferred();
  check_invariants();
}

void AgentImpl::remove(const FoundationType& a_foundation)
{
  // Foundations that are completely removed from the set of active foundations may unfreeze a checklist.
  unfreeze_ = active_foundations_.remove(a_foundation) || unfreeze_;
}

void AgentImpl::unfreeze(const FoundationType& a_foundation)
{
  to_unfreeze_.push_back(a_foundation);
}

void AgentImpl::check_invariants() const
{
  ActiveFoundationSet expected;

  for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
       limit = endpoint_managers_.end(); pos != limit; ++pos) {
    pos->second->compute_active_foundations(expected);
    pos->second->check_invariants();
  }

  OPENDDS_ASSERT(expected == active_foundations_);
}

void AgentImpl::shutdown()
{
  reactor()->cancel_timer(this, 0);
}

void AgentImpl::notify_shutdown()
{
  shutdown();
}

void AgentImpl::on_data_available(DCPS::RcHandle<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> >)
{
  DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  reader_->take(samples, infos);

  // FUTURE: This polls the endpoints.  The endpoints should just publish the change.
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
         limit = endpoint_managers_.end(); pos != limit; ++pos) {
    pos->second->network_change();
  }
}

void AgentImpl::process_deferred()
{
  // A successful connectivity check unfreezed a foundation.
  // Communicate this to all endpoints and checklists.
  for (FoundationList::const_iterator fpos = to_unfreeze_.begin(), flimit = to_unfreeze_.end(); fpos != flimit; ++fpos) {
    for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
           limit = endpoint_managers_.end(); pos != limit; ++pos) {
      pos->second->unfreeze(*fpos);
    }
  }
  to_unfreeze_.clear();

  // A foundation was completely removed.
  // Communicate this to all endpoints and checklists.
  if (unfreeze_) {
    for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
           limit = endpoint_managers_.end(); pos != limit; ++pos) {
      pos->second->unfreeze();
    }
    unfreeze_ = false;
  }
}

#endif /* OPENDDS_SECURITY */

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
