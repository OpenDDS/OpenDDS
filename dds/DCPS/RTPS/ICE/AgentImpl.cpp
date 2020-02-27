/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "AgentImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/TimeTypes.h"
#include "Task.h"
#include "EndpointManager.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

#ifdef OPENDDS_SECURITY

using DCPS::TimeDuration;
using DCPS::MonotonicTimePoint;

bool AgentImpl::TaskCompare::operator()(const Task* x, const Task* y) const
{
  return x->release_time_ > y->release_time_;
}

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

void AgentImpl::enqueue(Task* a_task)
{
  if (!a_task->in_queue_) {
    tasks_.push(a_task);
    a_task->in_queue_ = true;

    if (a_task == tasks_.top()) {
      const TimeDuration delay = std::max(get_configuration().T_a(), a_task->release_time_ - MonotonicTimePoint::now());
      execute_or_enqueue(new ScheduleTimerCommand(reactor(), this, delay));
    }
  }
}

bool AgentImpl::reactor_is_shut_down() const
{
  return TheServiceParticipant->is_shut_down();
}

int AgentImpl::handle_timeout(const ACE_Time_Value& a_now, const void* /*act*/)
{
  const MonotonicTimePoint now(a_now);
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, mutex, 0);
  check_invariants();

  if (tasks_.empty()) {
    return 0;
  }

  Task* task = tasks_.top();
  tasks_.pop();
  task->in_queue_ = false;

  task->execute(now);
  process_deferred();
  check_invariants();

  if (!tasks_.empty()) {
    const TimeDuration delay = std::max(get_configuration().T_a(), tasks_.top()->release_time_ - now);
    execute_or_enqueue(new ScheduleTimerCommand(reactor(), this, delay));
  }

  return 0;
}

AgentImpl::AgentImpl() :
  ReactorInterceptor(TheServiceParticipant->reactor(), TheServiceParticipant->reactor_owner()),
  unfreeze_(false),
  ncm_listener_added_(false),
  remote_peer_reflexive_counter_(0)
  {
    TheServiceParticipant->set_shutdown_listener(this);
  }

void AgentImpl::add_endpoint(Endpoint* a_endpoint)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();

  if (endpoint_managers_.find(a_endpoint) == endpoint_managers_.end()) {
    EndpointManager* em = new EndpointManager(this, a_endpoint);
    endpoint_managers_[a_endpoint] = em;
  }

  check_invariants();

  if (!endpoint_managers_.empty() && !ncm_listener_added_) {
    DCPS::NetworkConfigMonitor_rch ncm = TheServiceParticipant->network_config_monitor();
    if (ncm) {
      ncm->add_listener(*this);
    }
    ncm_listener_added_ = true;
  }
}

void AgentImpl::remove_endpoint(Endpoint* a_endpoint)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();

  EndpointManagerMapType::iterator pos = endpoint_managers_.find(a_endpoint);

  if (pos != endpoint_managers_.end()) {
    EndpointManager* em = pos->second;
    endpoint_managers_.erase(pos);
    em->schedule_for_destruction();
  }

  check_invariants();

  if (endpoint_managers_.empty() && ncm_listener_added_) {
    DCPS::NetworkConfigMonitor_rch ncm = TheServiceParticipant->network_config_monitor();
    if (ncm) {
      ncm->remove_listener(*this);
    }
    ncm_listener_added_ = false;
  }
}

AgentInfo AgentImpl::get_local_agent_info(Endpoint* a_endpoint) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, const_cast<ACE_Recursive_Thread_Mutex&>(mutex), AgentInfo());
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  return pos->second->agent_info();
}

void AgentImpl::add_local_agent_info_listener(Endpoint* a_endpoint,
                                              const DCPS::RepoId& a_local_guid,
                                              AgentInfoListener* a_agent_info_listener)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->add_agent_info_listener(a_local_guid, a_agent_info_listener);
}

void AgentImpl::remove_local_agent_info_listener(Endpoint* a_endpoint,
                                                 const DCPS::RepoId& a_local_guid)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->remove_agent_info_listener(a_local_guid);
}

void  AgentImpl::start_ice(Endpoint* a_endpoint,
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

void AgentImpl::stop_ice(Endpoint* a_endpoint,
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

ACE_INET_Addr  AgentImpl::get_address(Endpoint* a_endpoint,
                                      const DCPS::RepoId& a_local_guid,
                                      const DCPS::RepoId& a_remote_guid) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, const_cast<ACE_Recursive_Thread_Mutex&>(mutex), ACE_INET_Addr());
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  return pos->second->get_address(a_local_guid, a_remote_guid);
}

// Receive a STUN message.
void  AgentImpl::receive(Endpoint* a_endpoint,
                         const ACE_INET_Addr& a_local_address,
                         const ACE_INET_Addr& a_remote_address,
                         const STUN::Message& a_message)
{
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
void AgentImpl::network_change() const
{
  for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
         limit = endpoint_managers_.end(); pos != limit; ++pos) {
    pos->second->network_change();
  }
}

void AgentImpl::add_address(const DCPS::NetworkInterface&,
                            const ACE_INET_Addr&)
{
  network_change();
}

void AgentImpl::remove_address(const DCPS::NetworkInterface&,
                               const ACE_INET_Addr&)
{
  network_change();
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
