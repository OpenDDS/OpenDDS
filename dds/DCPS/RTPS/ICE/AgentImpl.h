/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY
#ifndef OPENDDS_DCPS_RTPS_ICE_AGENTIMPL_H
#define OPENDDS_DCPS_RTPS_ICE_AGENTIMPL_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "EndpointManager.h"
#include "Ice.h"
#include "Task.h"

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/NetworkConfigMonitor.h"
#include "dds/DCPS/InternalDataReader.h"
#include "dds/DCPS/ReactorInterceptor.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/Versioned_Namespace.h"

#include <ace/Time_Value.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

typedef std::vector<FoundationType> FoundationList;

class AgentImpl
  : public virtual Agent
  , public virtual DCPS::ReactorInterceptor
  , public virtual DCPS::ShutdownListener
  , public virtual DCPS::InternalDataReaderListener<DCPS::NetworkInterfaceAddress>
{
public:
  AgentImpl();

  void shutdown();

  void notify_shutdown();

  void add_endpoint(DCPS::WeakRcHandle<Endpoint> a_endpoint);

  void remove_endpoint(DCPS::WeakRcHandle<Endpoint> a_endpoint);

  AgentInfo get_local_agent_info(DCPS::WeakRcHandle<Endpoint> a_endpoint) const;

  void add_local_agent_info_listener(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                     const DCPS::RepoId& a_local_guid,
                                     DCPS::WeakRcHandle<AgentInfoListener> a_agent_info_listener);

  void remove_local_agent_info_listener(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                        const DCPS::RepoId& a_local_guid);

  void start_ice(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                 const DCPS::RepoId& a_local_guid,
                 const DCPS::RepoId& a_remote_guid,
                 const AgentInfo& a_remote_agent_info);

  void stop_ice(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                const DCPS::RepoId& a_local_guid,
                const DCPS::RepoId& a_remote_guid);

  ACE_INET_Addr get_address(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                            const DCPS::RepoId& a_local_guid,
                            const DCPS::RepoId& a_remote_guid) const;

  void receive(DCPS::WeakRcHandle<Endpoint> a_endpoint,
               const ACE_INET_Addr& a_local_address,
               const ACE_INET_Addr& a_remote_address,
               const STUN::Message& a_message);

  void enqueue(const DCPS::MonotonicTimePoint& a_release_time, WeakTaskPtr a_task);

  size_t remote_peer_reflexive_counter()
  {
    return remote_peer_reflexive_counter_++;
  }

  bool contains(const FoundationType& a_foundation) const
  {
    return active_foundations_.contains(a_foundation);
  }

  void add(const FoundationType& a_foundation)
  {
    active_foundations_.add(a_foundation);
  }

  void remove(const FoundationType& a_foundation);

  void unfreeze(const FoundationType& a_foundation);

  mutable ACE_Recursive_Thread_Mutex mutex;

private:
  void on_data_available(DCPS::RcHandle<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> > reader);
  void process_deferred();

  ActiveFoundationSet active_foundations_;
  FoundationList to_unfreeze_;
  bool unfreeze_;
  DCPS::RcHandle<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> > reader_;
  bool reader_added_;
  size_t remote_peer_reflexive_counter_;
  typedef std::map<DCPS::WeakRcHandle<Endpoint>, EndpointManagerPtr> EndpointManagerMapType;
  EndpointManagerMapType endpoint_managers_;
  struct Item {
    DCPS::MonotonicTimePoint release_time_;
    WeakTaskPtr task_;
    Item(const DCPS::MonotonicTimePoint& release_time,
         WeakTaskPtr task)
      : release_time_(release_time)
      , task_(task)
    {}
    bool operator<(const Item& other) const
    {
      return release_time_ > other.release_time_;
    }
  };
  std::priority_queue<Item> tasks_;
  DCPS::MonotonicTimePoint last_execute_;

  bool reactor_is_shut_down() const;
  int handle_timeout(const ACE_Time_Value& a_now, const void* /*act*/);

  void check_invariants() const;
};

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_AGENT_IMPL_H */
#endif /* OPENDDS_SECURITY */
