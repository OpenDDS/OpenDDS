/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY
#ifndef OPENDDS_RTPS_ICE_ENDPOINT_MANAGER_H
#define OPENDDS_RTPS_ICE_ENDPOINT_MANAGER_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/Definitions.h"
#include "dds/Versioned_Namespace.h"
#include <ace/INET_Addr.h>

#include "Ice.h"
#include "Task.h"
#include "Checklist.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

class AgentImpl;
struct Checklist;

struct DeferredTriggeredCheck {
  ACE_INET_Addr local_address;
  ACE_INET_Addr remote_address;
  ACE_UINT32 priority;
  bool use_candidate;
  DCPS::MonotonicTimePoint expiration_date;

  DeferredTriggeredCheck(const ACE_INET_Addr& a_local_address,
                         const ACE_INET_Addr& a_remote_address,
                         ACE_UINT32 a_priority,
                         bool a_use_candidate,
                         const DCPS::MonotonicTimePoint& a_expiration_date)
  : local_address(a_local_address)
  , remote_address(a_remote_address)
  , priority(a_priority)
  , use_candidate(a_use_candidate)
  , expiration_date(a_expiration_date)
  {}
};

struct EndpointManager {
  AgentImpl* const agent_impl;
  Endpoint* const endpoint;

  EndpointManager(AgentImpl* a_agent_impl, Endpoint* a_endpoint);

  const AgentInfo& agent_info() const
  {
    return agent_info_;
  }

  void add_agent_info_listener(const DCPS::RepoId& a_local_guid,
                               AgentInfoListener* a_agent_info_listener)
  {
    agent_info_listeners_[a_local_guid] = a_agent_info_listener;
  }

  void remove_agent_info_listener(const DCPS::RepoId& a_local_guid)
  {
    agent_info_listeners_.erase(a_local_guid);
  }

  void start_ice(const DCPS::RepoId& a_local_guid,
                 const DCPS::RepoId& a_remote_guid,
                 const AgentInfo& a_remote_agent_info);

  void stop_ice(const DCPS::RepoId& local_guid,
                const DCPS::RepoId& remote_guid);

  ACE_INET_Addr get_address(const DCPS::RepoId& a_local_guid,
                            const DCPS::RepoId& a_remote_guid) const;

  void receive(const ACE_INET_Addr& a_local_address,
               const ACE_INET_Addr& a_remote_address,
               const STUN::Message& a_message);

  void set_responsible_checklist(const STUN::TransactionId& a_transaction_id, Checklist* a_checklist)
  {
    OPENDDS_ASSERT(!transaction_id_to_checklist_.count(a_transaction_id));
    transaction_id_to_checklist_[a_transaction_id] = a_checklist;
  }

  void unset_responsible_checklist(const STUN::TransactionId& a_transaction_id, Checklist* a_checklist)
  {
    TransactionIdToChecklistType::iterator pos = transaction_id_to_checklist_.find(a_transaction_id);
    OPENDDS_ASSERT(pos != transaction_id_to_checklist_.end());
    OPENDDS_ASSERT(pos->second == a_checklist);
    ACE_UNUSED_ARG(a_checklist);
    transaction_id_to_checklist_.erase(pos);
  }

  void set_responsible_checklist(const GuidPair& a_guid_pair, Checklist* a_checklist)
  {
    OPENDDS_ASSERT(!guid_pair_to_checklist_.count(a_guid_pair));
    guid_pair_to_checklist_[a_guid_pair] = a_checklist;
  }

  void unset_responsible_checklist(const GuidPair& a_guid_pair, Checklist* a_checklist)
  {
    GuidPairToChecklistType::iterator pos = guid_pair_to_checklist_.find(a_guid_pair);
    OPENDDS_ASSERT(pos != guid_pair_to_checklist_.end());
    OPENDDS_ASSERT(pos->second == a_checklist);
    ACE_UNUSED_ARG(a_checklist);
    guid_pair_to_checklist_.erase(pos);
  }

  void set_responsible_checklist(const std::string& a_username, Checklist* a_checklist)
  {
    OPENDDS_ASSERT(!username_to_checklist_.count(a_username));
    username_to_checklist_[a_username] = a_checklist;
  }

  void unset_responsible_checklist(const std::string& a_username, Checklist* a_checklist)
  {
    UsernameToChecklistType::iterator pos = username_to_checklist_.find(a_username);
    OPENDDS_ASSERT(pos != username_to_checklist_.end());
    OPENDDS_ASSERT(pos->second == a_checklist);
    ACE_UNUSED_ARG(a_checklist);
    username_to_checklist_.erase(pos);
  }

  void unfreeze();

  void unfreeze(const FoundationType& a_foundation);

  void compute_active_foundations(ActiveFoundationSet& a_active_foundations) const;

  void check_invariants() const;

  void schedule_for_destruction();

  void ice_connect(const GuidSetType& guids, const ACE_INET_Addr& addr)
  {
    endpoint->ice_connect(guids, addr);
  }

  void ice_disconnect(const GuidSetType& guids)
  {
    endpoint->ice_disconnect(guids);
  }

  void network_change();

  void send(const ACE_INET_Addr& address, const STUN::Message& message);

private:
  bool scheduled_for_destruction_;
  AddressListType host_addresses_;          // Cached list of host addresses.
  ACE_INET_Addr server_reflexive_address_;  // Server-reflexive address (from STUN server).
  ACE_INET_Addr stun_server_address_;       // Address of the STUN server.
  ACE_UINT64 ice_tie_breaker_;
  AgentInfo agent_info_;

  // State variables for getting and maintaining the server reflexive address.
  bool requesting_;
  size_t send_count_;
  ACE_INET_Addr next_stun_server_address_;
  STUN::Message binding_request_;       // Binding request sent to STUN server.

  typedef std::deque<DeferredTriggeredCheck> DeferredTriggeredCheckListType;
  typedef std::map<std::string, DeferredTriggeredCheckListType> DeferredTriggeredChecksType;
  DeferredTriggeredChecksType deferred_triggered_checks_;

  // Managed by checklists.
  typedef std::map<std::string, Checklist*> UsernameToChecklistType;
  UsernameToChecklistType username_to_checklist_;

  // Managed by checklists.
  typedef std::map<STUN::TransactionId, Checklist*> TransactionIdToChecklistType;
  TransactionIdToChecklistType transaction_id_to_checklist_;

  // Managed by checklists.
  typedef std::map<GuidPair, Checklist*> GuidPairToChecklistType;
  GuidPairToChecklistType guid_pair_to_checklist_;

  typedef std::map<DCPS::RepoId, AgentInfoListener*, DCPS::GUID_tKeyLessThan> AgentInfoListenersType;
  AgentInfoListenersType agent_info_listeners_;

  void change_username();

  void change_password(bool password_only);

  void set_host_addresses(const AddressListType& a_host_addresses);

  void set_server_reflexive_address(const ACE_INET_Addr& a_server_reflexive_address,
                                    const ACE_INET_Addr& a_stun_server_address);

  void regenerate_agent_info(bool password_only);

  void server_reflexive_task(const DCPS::MonotonicTimePoint & a_now);

  bool success_response(const STUN::Message& a_message);

  bool error_response(const STUN::Message& a_message);

  Checklist* create_checklist(const AgentInfo& a_remote_agent_info);

  // STUN Message processing.
  STUN::Message make_unknown_attributes_error_response(const STUN::Message& a_message,
                                                       const std::vector<STUN::AttributeType>& a_unknown_attributes);

  STUN::Message make_bad_request_error_response(const STUN::Message& a_message,
                                                const std::string& a_reason);

  STUN::Message make_unauthorized_error_response(const STUN::Message& a_message);

  void request(const ACE_INET_Addr& a_local_address,
               const ACE_INET_Addr& a_remote_address,
               const STUN::Message& a_message);

  void indication(const ACE_INET_Addr& a_local_address,
                  const ACE_INET_Addr& a_remote_address,
                  const STUN::Message& a_message);

  void success_response(const ACE_INET_Addr& a_local_address,
                        const ACE_INET_Addr& a_remote_address,
                        const STUN::Message& a_message);

  void error_response(const ACE_INET_Addr& a_local_address,
                      const ACE_INET_Addr& a_remote_address,
                      const STUN::Message& a_message);

  struct ServerReflexiveTask : public Task {
    EndpointManager* endpoint_manager;
    ServerReflexiveTask(EndpointManager* a_endpoint_manager);
    void execute(const DCPS::MonotonicTimePoint& a_now);
  } server_reflexive_task_;

  struct ChangePasswordTask : public Task {
    EndpointManager* endpoint_manager;
    ChangePasswordTask(EndpointManager* a_endpoint_manager);
    void execute(const DCPS::MonotonicTimePoint& a_now);
  } change_password_task_;
};

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_ENDPOINT_MANAGER_H */
#endif /* OPENDDS_SECURITY */
