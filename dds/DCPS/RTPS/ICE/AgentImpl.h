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

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/ReactorInterceptor.h"
#include "dds/DCPS/NetworkConfigMonitor.h"

#include "Ice.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

struct Task;
struct EndpointManager;

typedef std::pair<std::string, std::string> FoundationType;

class ActiveFoundationSet {
public:
  void add(const FoundationType& a_foundation)
  {
    std::pair<FoundationsType::iterator, bool> x = foundations_.insert(std::make_pair(a_foundation, 0));
    x.first->second += 1;
  }

  void remove(const FoundationType& a_foundation)
  {
    FoundationsType::iterator pos = foundations_.find(a_foundation);
    OPENDDS_ASSERT(pos != foundations_.end());
    pos->second -= 1;

    if (pos->second == 0) {
      foundations_.erase(pos);
    }
  }

  bool contains(const FoundationType& a_foundation) const
  {
    return foundations_.find(a_foundation) != foundations_.end();
  }

  bool operator==(const ActiveFoundationSet& a_other) const
  {
    return foundations_ == a_other.foundations_;
  }

private:
  typedef std::map<FoundationType, size_t> FoundationsType;
  FoundationsType foundations_;
};

class AgentImpl : public Agent, public DCPS::ReactorInterceptor, public virtual DCPS::NetworkConfigListener {
public:
  ActiveFoundationSet active_foundations;

  AgentImpl();

  Configuration& get_configuration()
  {
    return configuration_;
  }

  void add_endpoint(Endpoint* a_endpoint);

  void remove_endpoint(Endpoint* a_endpoint);

  AgentInfo get_local_agent_info(Endpoint* a_endpoint) const;

  void add_local_agent_info_listener(Endpoint* a_endpoint,
                                     const DCPS::RepoId& a_local_guid,
                                     AgentInfoListener* a_agent_info_listener);

  void remove_local_agent_info_listener(Endpoint* a_endpoint,
                                        const DCPS::RepoId& a_local_guid);

  void start_ice(Endpoint* a_endpoint,
                 const DCPS::RepoId& a_local_guid,
                 const DCPS::RepoId& a_remote_guid,
                 const AgentInfo& a_remote_agent_info);

  void stop_ice(Endpoint* a_endpoint,
                const DCPS::RepoId& a_local_guid,
                const DCPS::RepoId& a_remote_guid);

  ACE_INET_Addr get_address(Endpoint* a_endpoint,
                            const DCPS::RepoId& a_local_guid,
                            const DCPS::RepoId& a_remote_guid) const;

  void receive(Endpoint* a_endpoint,
               const ACE_INET_Addr& a_local_address,
               const ACE_INET_Addr& a_remote_address,
               const STUN::Message& a_message);

  void enqueue(Task* a_task);

  size_t remote_peer_reflexive_counter()
  {
    return remote_peer_reflexive_counter_++;
  }

  void unfreeze(const FoundationType& a_foundation);

  ACE_Recursive_Thread_Mutex mutex;

private:
  void network_change() const;
  void add_address(const DCPS::NetworkInterface& interface,
                   const ACE_INET_Addr& address);
  void remove_address(const DCPS::NetworkInterface& interface,
                      const ACE_INET_Addr& address);

  bool ncm_listener_added_;
  Configuration configuration_;
  size_t remote_peer_reflexive_counter_;
  typedef std::map<Endpoint*, EndpointManager*> EndpointManagerMapType;
  EndpointManagerMapType endpoint_managers_;
  struct TaskCompare {
    bool operator()(const Task* x, const Task* y) const;
  };
  std::priority_queue<Task*, std::vector<Task*>, TaskCompare> tasks_;

  bool reactor_is_shut_down() const;
  int handle_timeout(const ACE_Time_Value& a_now, const void* /*act*/);

  void check_invariants() const;
};

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_AGENT_IMPL_H */
