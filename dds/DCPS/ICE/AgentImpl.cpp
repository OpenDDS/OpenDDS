/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "AgentImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "Task.h"
#include "EndpointManager.h"

#include <iostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  bool AgentImpl::TaskCompare::operator() (Task const * x, Task const * y) const {
    return x->m_release_time > y->m_release_time;
  }

  struct ScheduleTimerCommand : public DCPS::ReactorInterceptor::Command {
    ACE_Reactor* reactor;
    ACE_Event_Handler* event_handler;
    ACE_Time_Value delay;

    ScheduleTimerCommand(ACE_Reactor * a_reactor, ACE_Event_Handler * a_event_handler, ACE_Time_Value const & a_delay) :
      reactor(a_reactor), event_handler(a_event_handler), delay(a_delay) {}

    void execute() {
      reactor->cancel_timer(event_handler, 0);
      reactor->schedule_timer(event_handler, 0, delay);
    }
  };

  void AgentImpl::enqueue(Task * a_task) {
    if (!a_task->m_in_queue) {
      m_tasks.push(a_task);
      a_task->m_in_queue = true;
      if (a_task == m_tasks.top()) {
        ACE_Time_Value const delay = std::max(get_configuration().T_a(), a_task->m_release_time - ACE_Time_Value().now());
        ScheduleTimerCommand c(reactor(), this, delay);
        execute_or_enqueue(c);
      }
    }
  }

  bool AgentImpl::reactor_is_shut_down() const {
    return TheServiceParticipant->is_shut_down();
  }

  int AgentImpl::handle_timeout(ACE_Time_Value const & a_now, const void* /*act*/) {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, mutex, 0);
    check_invariants();
    if (m_tasks.empty()) {
      return 0;
    }

    Task * task = m_tasks.top();
    m_tasks.pop();
    task->m_in_queue = false;

    task->execute(a_now);

    if (!m_tasks.empty()) {
      ACE_Time_Value const delay = std::max(get_configuration().T_a(), m_tasks.top()->m_release_time - a_now);
      ScheduleTimerCommand c(reactor(), this, delay);
      execute_or_enqueue(c);
    }

    check_invariants();
    return 0;
  }

  AgentImpl::AgentImpl() :
    ReactorInterceptor(TheServiceParticipant->reactor(), TheServiceParticipant->reactor_owner()),
    m_remote_peer_reflexive_counter(0) {}

  void AgentImpl::add_endpoint(Endpoint * a_endpoint) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
    check_invariants();
    if (m_endpoint_managers.find(a_endpoint) == m_endpoint_managers.end()) {
      EndpointManager * em = new EndpointManager(this, a_endpoint);
      m_endpoint_managers[a_endpoint] = em;
    }
    check_invariants();
  }

  void AgentImpl::remove_endpoint(Endpoint * a_endpoint) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
    check_invariants();

    EndpointManagerMapType::const_iterator pos = m_endpoint_managers.find(a_endpoint);
    if (pos != m_endpoint_managers.end()) {
      EndpointManager * em = pos->second;
      m_endpoint_managers.erase(pos);
      em->schedule_for_destruction();
    }
    check_invariants();
  }

  AgentInfo AgentImpl::get_local_agent_info(Endpoint * a_endpoint) const {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, const_cast<ACE_Recursive_Thread_Mutex&>(mutex), AgentInfo());
    EndpointManagerMapType::const_iterator pos = m_endpoint_managers.find(a_endpoint);
    assert(pos != m_endpoint_managers.end());
    return pos->second->agent_info();
  }

  void AgentImpl::add_local_agent_info_listener(Endpoint * a_endpoint,
                                                DCPS::RepoId const & a_local_guid,
                                                AgentInfoListener * a_agent_info_listener) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
    EndpointManagerMapType::const_iterator pos = m_endpoint_managers.find(a_endpoint);
    assert(pos != m_endpoint_managers.end());
    pos->second->add_agent_info_listener(a_local_guid, a_agent_info_listener);
  }

  void AgentImpl::remove_local_agent_info_listener(Endpoint * a_endpoint,
                                                   DCPS::RepoId const & a_local_guid) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
    EndpointManagerMapType::const_iterator pos = m_endpoint_managers.find(a_endpoint);
    assert(pos != m_endpoint_managers.end());
    pos->second->remove_agent_info_listener(a_local_guid);
  }

  void  AgentImpl::start_ice(Endpoint * a_endpoint,
                             DCPS::RepoId const & a_local_guid,
                             DCPS::RepoId const & a_remote_guid,
                             AgentInfo const & a_remote_agent_info) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
    check_invariants();
    EndpointManagerMapType::const_iterator pos = m_endpoint_managers.find(a_endpoint);
    assert(pos != m_endpoint_managers.end());
    pos->second->start_ice(a_local_guid, a_remote_guid, a_remote_agent_info);
    check_invariants();
  }

  void AgentImpl::stop_ice(Endpoint * a_endpoint,
                           DCPS::RepoId const & a_local_guid,
                           DCPS::RepoId const & a_remote_guid) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
    check_invariants();
    EndpointManagerMapType::const_iterator pos = m_endpoint_managers.find(a_endpoint);
    assert(pos != m_endpoint_managers.end());
    pos->second->stop_ice(a_local_guid, a_remote_guid);
    check_invariants();
  }

  ACE_INET_Addr  AgentImpl::get_address(Endpoint * a_endpoint,
                                        DCPS::RepoId const & a_local_guid,
                                        DCPS::RepoId const & a_remote_guid) const {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, const_cast<ACE_Recursive_Thread_Mutex&>(mutex), ACE_INET_Addr());
    EndpointManagerMapType::const_iterator pos = m_endpoint_managers.find(a_endpoint);
    assert(pos != m_endpoint_managers.end());
    return pos->second->get_address(a_local_guid, a_remote_guid);
  }

  // Receive a STUN message.
  void  AgentImpl::receive(Endpoint * a_endpoint,
                           ACE_INET_Addr const & a_local_address,
                           ACE_INET_Addr const & a_remote_address,
                           const STUN::Message& a_message) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
    check_invariants();
    EndpointManagerMapType::const_iterator pos = m_endpoint_managers.find(a_endpoint);
    assert(pos != m_endpoint_managers.end());
    pos->second->receive(a_local_address, a_remote_address, a_message);
    check_invariants();
  }

  void AgentImpl::unfreeze(FoundationType const & a_foundation) {
    for (EndpointManagerMapType::const_iterator pos = m_endpoint_managers.begin(),
           limit = m_endpoint_managers.end(); pos != limit; ++pos) {
      pos->second->unfreeze(a_foundation);
    }
  }

  void AgentImpl::check_invariants() const {
    ActiveFoundationSet expected;

    for (EndpointManagerMapType::const_iterator pos = m_endpoint_managers.begin(),
           limit = m_endpoint_managers.end(); pos != limit; ++pos) {
      pos->second->compute_active_foundations(expected);
      pos->second->check_invariants();
    }

    assert(expected == active_foundations);
  }

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
