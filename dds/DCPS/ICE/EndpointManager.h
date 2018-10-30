/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_ICE_ENDPOINT_MANAGER_H
#define OPENDDS_RTPS_ICE_ENDPOINT_MANAGER_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

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
    uint32_t priority;
    bool use_candidate;
    ACE_Time_Value expiration_date;

    DeferredTriggeredCheck(ACE_INET_Addr const & a_local_address,
                           ACE_INET_Addr const & a_remote_address,
                           uint32_t a_priority,
                           bool a_use_candidate,
                           ACE_Time_Value const & a_expiration_date)
      : local_address(a_local_address)
      , remote_address(a_remote_address)
      , priority(a_priority)
      , use_candidate(a_use_candidate)
      , expiration_date(a_expiration_date)
    {}
  };

  struct EndpointManager {
    AgentImpl * const agent_impl;
    Endpoint * const endpoint;

    EndpointManager(AgentImpl * a_agent_impl, Endpoint * a_endpoint);

    AgentInfo const & agent_info() const { return m_agent_info; }

    void add_agent_info_listener(DCPS::RepoId const & a_local_guid,
                                 AgentInfoListener * a_agent_info_listener) {
      m_agent_info_listeners[a_local_guid] = a_agent_info_listener;
    }

    void remove_agent_info_listener(DCPS::RepoId const & a_local_guid) {
      m_agent_info_listeners.erase(a_local_guid);
    }

    void start_ice(DCPS::RepoId const & a_local_guid,
                   DCPS::RepoId const & a_remote_guid,
                   AgentInfo const & a_remote_agent_info);

    void stop_ice(DCPS::RepoId const & local_guid,
                  DCPS::RepoId const & remote_guid);

    ACE_INET_Addr get_address(DCPS::RepoId const & a_local_guid,
                              DCPS::RepoId const & a_remote_guid) const;

    void receive(ACE_INET_Addr const & a_local_address,
                 ACE_INET_Addr const & a_remote_address,
                 STUN::Message const & a_message);

    void set_responsible_checklist(STUN::TransactionId const & a_transaction_id, Checklist * a_checklist) {
      TransactionIdToChecklistType::const_iterator pos = m_transaction_id_to_checklist.find(a_transaction_id);
      assert(pos == m_transaction_id_to_checklist.end());
      m_transaction_id_to_checklist[a_transaction_id] = a_checklist;
    }

    void unset_responsible_checklist(STUN::TransactionId const & a_transaction_id, Checklist * a_checklist) {
      TransactionIdToChecklistType::const_iterator pos = m_transaction_id_to_checklist.find(a_transaction_id);
      assert(pos != m_transaction_id_to_checklist.end());
      assert(pos->second == a_checklist);
      m_transaction_id_to_checklist.erase(pos);
    }

    void set_responsible_checklist(GuidPair const & a_guid_pair, Checklist * a_checklist) {
      GuidPairToChecklistType::const_iterator pos = m_guid_pair_to_checklist.find(a_guid_pair);
      assert(pos == m_guid_pair_to_checklist.end());
      m_guid_pair_to_checklist[a_guid_pair] = a_checklist;
    }

    void unset_responsible_checklist(GuidPair const & a_guid_pair, Checklist * a_checklist) {
      GuidPairToChecklistType::const_iterator pos = m_guid_pair_to_checklist.find(a_guid_pair);
      assert(pos != m_guid_pair_to_checklist.end());
      assert(pos->second == a_checklist);
      m_guid_pair_to_checklist.erase(pos);
    }

    void set_responsible_checklist(std::string const & a_username, Checklist * a_checklist) {
      UsernameToChecklistType::const_iterator pos = m_username_to_checklist.find(a_username);
      assert(pos == m_username_to_checklist.end());
      m_username_to_checklist[a_username] = a_checklist;
    }

    void unset_responsible_checklist(std::string const & a_username, Checklist * a_checklist) {
      UsernameToChecklistType::const_iterator pos = m_username_to_checklist.find(a_username);
      assert(pos != m_username_to_checklist.end());
      assert(pos->second == a_checklist);
      m_username_to_checklist.erase(pos);
    }

    void unfreeze(FoundationType const & a_foundation);

    void compute_active_foundations(ActiveFoundationSet & a_active_foundations) const;

    void check_invariants() const;

    void schedule_for_destruction();

  private:
    bool m_scheduled_for_destruction;
    AddressListType m_host_addresses;          // Cached list of host addresses.
    ACE_INET_Addr m_server_reflexive_address;  // Server-reflexive address (from STUN server).
    ACE_INET_Addr m_stun_server_address;       // Address of the STUN server.
    ACE_UINT64 m_ice_tie_breaker;
    AgentInfo m_agent_info;

    // State variables for getting and maintaining the server reflexive address.
    bool m_requesting;
    size_t m_send_count;
    ACE_INET_Addr m_next_stun_server_address;
    STUN::Message m_binding_request;       // Binding request sent to STUN server.

    typedef std::deque<DeferredTriggeredCheck> DeferredTriggeredCheckListType;
    typedef std::map<std::string, DeferredTriggeredCheckListType> DeferredTriggeredChecksType;
    DeferredTriggeredChecksType m_deferred_triggered_checks;

    // Managed by checklists.
    typedef std::map<std::string, Checklist*> UsernameToChecklistType;
    UsernameToChecklistType m_username_to_checklist;

    // Managed by checklists.
    typedef std::map<STUN::TransactionId, Checklist*> TransactionIdToChecklistType;
    TransactionIdToChecklistType m_transaction_id_to_checklist;

    // Managed by checklists.
    typedef std::map<GuidPair, Checklist*> GuidPairToChecklistType;
    GuidPairToChecklistType m_guid_pair_to_checklist;

    typedef std::map<DCPS::RepoId, AgentInfoListener *, DCPS::GUID_tKeyLessThan> AgentInfoListenersType;
    AgentInfoListenersType m_agent_info_listeners;

    void change_username();

    void change_password(bool password_only);

    void set_host_addresses(AddressListType const & a_host_addresses);

    void set_server_reflexive_address(ACE_INET_Addr const & a_server_reflexive_address,
                                      ACE_INET_Addr const & a_stun_server_address);

    void regenerate_agent_info(bool password_only);

    void server_reflexive_task(ACE_Time_Value const & a_now);

    bool success_response(STUN::Message const & a_message);

    bool error_response(STUN::Message const & a_message);

    Checklist* create_checklist(AgentInfo const & a_remote_agent_info);

    // STUN Message processing.
    STUN::Message make_unknown_attributes_error_response(STUN::Message const & a_message,
                                                         std::vector<STUN::AttributeType> const & a_unknown_attributes);

    STUN::Message make_bad_request_error_response(STUN::Message const & a_message,
                                                  std::string const & a_reason);

    STUN::Message make_unauthorized_error_response(STUN::Message const & a_message);

    void request(ACE_INET_Addr const & a_local_address,
                 ACE_INET_Addr const & a_remote_address,
                 STUN::Message const & a_message);

    void indication(STUN::Message const & a_message);

    void success_response(ACE_INET_Addr const & a_local_address,
                          ACE_INET_Addr const & a_remote_address,
                          STUN::Message const & a_message);

    void error_response(ACE_INET_Addr const & a_local_address,
                        ACE_INET_Addr const & a_remote_address,
                        STUN::Message const & a_message);

    struct ServerReflexiveTask : public Task {
      EndpointManager * endpoint_manager;
      ServerReflexiveTask(EndpointManager * a_endpoint_manager);
      void execute(ACE_Time_Value const & a_now);
    } m_server_reflexive_task;

    struct ChangePasswordTask : public Task {
      EndpointManager * endpoint_manager;
      ChangePasswordTask(EndpointManager * a_endpoint_manager);
      void execute(ACE_Time_Value const & a_now);
    } m_change_password_task;
  };

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_ENDPOINT_MANAGER_H */
