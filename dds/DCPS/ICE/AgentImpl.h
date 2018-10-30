/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_ICE_AGENT_IMPL_H
#define OPENDDS_RTPS_ICE_AGENT_IMPL_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <ace/Time_Value.h>
#include "dds/Versioned_Namespace.h"
#include "dds/DCPS/ReactorInterceptor.h"
#include "Ice.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  struct Task;
  struct EndpointManager;

  typedef std::pair<std::string, std::string> FoundationType;

  class ActiveFoundationSet {
  public:
    void add(FoundationType const & a_foundation) {
      std::pair<FoundationsType::iterator, bool> x = m_foundations.insert(std::make_pair(a_foundation, 0));
      x.first->second += 1;
    }

    void remove(FoundationType const & a_foundation) {
      FoundationsType::iterator pos = m_foundations.find(a_foundation);
      assert(pos != m_foundations.end());
      pos->second -= 1;
      if (pos->second == 0) {
        m_foundations.erase(pos);
      }
    }

    bool contains(FoundationType const & a_foundation) const {
      return m_foundations.find(a_foundation) != m_foundations.end();
    }

    bool operator==(ActiveFoundationSet const & a_other) const {
      return m_foundations == a_other.m_foundations;
    }

  private:
    typedef std::map<FoundationType, size_t> FoundationsType;
    FoundationsType m_foundations;
  };

  class AgentImpl : public Agent, public DCPS::ReactorInterceptor {
  public:
    ActiveFoundationSet active_foundations;

    AgentImpl();

    Configuration & get_configuration() {
      return m_configuration;
    };

    void add_endpoint(Endpoint * a_endpoint);

    void remove_endpoint(Endpoint * a_endpoint);

    AgentInfo get_local_agent_info(Endpoint * a_endpoint) const;

    void add_local_agent_info_listener(Endpoint * a_endpoint,
                                       DCPS::RepoId const & a_local_guid,
                                       AgentInfoListener * a_agent_info_listener);

    void remove_local_agent_info_listener(Endpoint * a_endpoint,
                                          DCPS::RepoId const & a_local_guid);

    void start_ice(Endpoint * a_endpoint,
                   DCPS::RepoId const & a_local_guid,
                   DCPS::RepoId const & a_remote_guid,
                   AgentInfo const & a_remote_agent_info);

    void stop_ice(Endpoint * a_endpoint,
                  DCPS::RepoId const & a_local_guid,
                  DCPS::RepoId const & a_remote_guid);

    ACE_INET_Addr get_address(Endpoint * a_endpoint,
                              DCPS::RepoId const & a_local_guid,
                              DCPS::RepoId const & a_remote_guid) const;

    void receive(Endpoint * a_endpoint,
                 ACE_INET_Addr const & a_local_address,
                 ACE_INET_Addr const & a_remote_address,
                 STUN::Message const & a_message);

    void enqueue(Task * a_task);

    size_t remote_peer_reflexive_counter() { return m_remote_peer_reflexive_counter++; };

    void unfreeze(FoundationType const & a_foundation);

    ACE_Recursive_Thread_Mutex mutex;

  private:
    Configuration m_configuration;
    size_t m_remote_peer_reflexive_counter;
    typedef std::map<Endpoint *, EndpointManager *> EndpointManagerMapType;
    EndpointManagerMapType m_endpoint_managers;
    struct TaskCompare {
      bool operator() (Task const * x, Task const * y) const;
    };
    std::priority_queue<Task *, std::vector<Task*>, TaskCompare> m_tasks;

    bool reactor_is_shut_down() const;
    int handle_timeout(ACE_Time_Value const & a_now, const void* /*act*/);

    void check_invariants() const;
  };

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_AGENT_IMPL_H */
