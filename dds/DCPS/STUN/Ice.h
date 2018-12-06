/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_ICE_H
#define OPENDDS_RTPS_ICE_H

#include "ace/INET_Addr.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/STUN/Stun.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/ReactorInterceptor.h"

#include <cassert>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  enum AgentType {
    FULL = 0x0,
    LITE = 0x1,
  };

  enum CandidateType {
    HOST = 0x0,
    SERVER_REFLEXIVE = 0x1,
    PEER_REFLEXIVE = 0x2,
    RELAYED = 0x3,
  };

  struct Candidate {
    ACE_INET_Addr address;
    // Transport - UDP or TCP
    std::string foundation;
    // Component ID
    uint32_t priority;
    CandidateType type;
    // Related Address and Port
    // Extensibility Parameters

    ACE_INET_Addr base;  // Not sent.

    bool operator==(const Candidate& other) const;
  };

  struct AgentInfo {
    typedef std::vector<Candidate> CandidatesType;
    typedef CandidatesType::const_iterator const_iterator;

    CandidatesType candidates;
    AgentType type;
    // Connectivity-Check Pacing Value
    std::string username;
    std::string password;
    // Extensions

    const_iterator begin() const { return candidates.begin(); }
    const_iterator end() const { return candidates.end(); }
    bool operator==(const AgentInfo& other) const {
      return this->candidates == other.candidates && this->type == other.type && this->username == other.username && this->password == other.password;
    }
    bool operator!=(const AgentInfo& other) const { return !(*this == other); }
  };

  typedef std::vector<ACE_INET_Addr> AddressListType;

  class StunSender {
  public:
    virtual ~StunSender() {}
    virtual AddressListType host_addresses() const = 0;
    virtual void send(const ACE_INET_Addr& address, const STUN::Message& message) = 0;
    virtual bool reactor_is_shut_down() const = 0;
  };

  typedef std::pair<std::string, std::string> FoundationType;

  struct CandidatePair {
    Candidate const local;
    Candidate const remote;
    FoundationType const foundation;
    bool const local_is_controlling;
    uint64_t const priority;
    bool const nominate; // nominate this pair on success

    CandidatePair(const Candidate& a_local, const Candidate& a_remote, bool a_local_is_controlling, bool a_nominate = false);
    bool operator==(const CandidatePair& other) const;

    static bool priority_sorted(const CandidatePair& x, const CandidatePair& y) { return x.priority > y.priority; }

  private:
    uint64_t compute_priority() {
      uint64_t const g = local_is_controlling ? local.priority : remote.priority;
      uint64_t const d = local_is_controlling ? remote.priority : local.priority;
      return (std::min(g,d) << 32) + 2 * std::max(g,d) + (g > d ? 1 : 0);
    }
  };

  struct Checklist;

  struct ConnectivityCheck {
    ConnectivityCheck(Checklist* checklist, const CandidatePair& candidate_pair,
                      const AgentInfo& local_agent_info, const AgentInfo& remote_agent_info,
                      uint64_t ice_tie_breaker);
    Checklist* checklist() const { return checklist_; }
    const CandidatePair& candidate_pair() const { return candidate_pair_; }
    const STUN::Message& request() const { return request_; }
    // The request that triggered this check.
    const STUN::Message& triggering_request() const { return triggering_request_; }
    void increment_send_count() { ++send_count_; }
    size_t send_count() const { return send_count_; }
    void cancel() { cancelled_ = true; }
    bool cancelled() const { return cancelled_; }

  private:
    Checklist* checklist_;
    CandidatePair candidate_pair_;
    STUN::Message request_;
    STUN::Message triggering_request_;
    size_t send_count_;
    bool cancelled_;
  };

  class ActiveFoundationSet {
  public:
    void add(const FoundationType& foundation) {
      std::pair<FoundationsType::iterator, bool> x = foundations_.insert(std::make_pair(foundation, 0));
      x.first->second += 1;
    }
    void remove(const FoundationType& foundation) {
      FoundationsType::iterator pos = foundations_.find(foundation);
      assert(pos != foundations_.end());
      pos->second -= 1;
      if (pos->second == 0) {
        foundations_.erase(pos);
      }
    }
    bool contains(const FoundationType& foundation) const {
      return foundations_.find(foundation) != foundations_.end();
    }
  private:
    typedef std::map<FoundationType, size_t> FoundationsType;
    FoundationsType foundations_;
  };

  struct DeferredTriggeredCheck {
    ACE_INET_Addr local_address;
    ACE_INET_Addr remote_address;
    uint32_t priority;
    bool use_candidate;

    DeferredTriggeredCheck(const ACE_INET_Addr& a_local_address,
                           const ACE_INET_Addr& a_remote_address,
                           uint32_t a_priority,
                           bool a_use_candidate)
    : local_address(a_local_address)
    , remote_address(a_remote_address)
    , priority(a_priority)
    , use_candidate(a_use_candidate)
    {}
  };

  struct GuidPair {
    DCPS::RepoId local;
    DCPS::RepoId remote;

    GuidPair(const DCPS::RepoId& a_local, const DCPS::RepoId& a_remote) : local(a_local), remote(a_remote) {}
    bool operator< (const GuidPair& other) const;
  };

  class SignalingChannel {
  public:
    virtual ~SignalingChannel() {}
    virtual void update_agent_info(const GuidPair& guidp, const AgentInfo& agent_info) = 0;
  };

  typedef std::map<GuidPair, SignalingChannel*> GuidSetType;

  class AbstractAgent {
  public:
    virtual ~AbstractAgent() {}
    virtual void start_ice(const GuidPair& guidp, SignalingChannel* signaling_channel = 0) = 0;
    virtual bool update_remote_agent_info(const GuidPair& guidp, const AgentInfo& agent_info) = 0;
    virtual void stop_ice(const GuidPair& guidp) = 0;
    virtual AgentInfo get_local_agent_info() const = 0;
    virtual bool is_running() const = 0;
    virtual ACE_INET_Addr get_address(const DCPS::RepoId& guid) const = 0;

    virtual void receive(const ACE_INET_Addr& local_address, const ACE_INET_Addr& remote_address, const STUN::Message& message) = 0;
  };

  class OpenDDS_Stun_Export Agent : public AbstractAgent {
  public:
    Agent(StunSender* stun_sender, const ACE_INET_Addr& stun_server_address, ACE_Reactor* reactor, ACE_thread_t owner);

    void start_ice(const GuidPair& guidp, SignalingChannel* signaling_channel = 0);
    bool update_remote_agent_info(const GuidPair& guidp, const AgentInfo& agent_info);
    void stop_ice(const GuidPair& guidp);

    AgentInfo get_local_agent_info() const;

    bool is_running() const;

    ACE_INET_Addr get_address(const DCPS::RepoId& guid) const;

    // Receive a STUN message.
    void receive(const ACE_INET_Addr& local_address, const ACE_INET_Addr& remote_address, const STUN::Message& message);

  private:
    ACE_Recursive_Thread_Mutex mutex_;
    // The info for this agent.
    AgentInfo local_agent_info_;
    // Data stream capable of multiplexing STUN messages.
    StunSender* const stun_sender_;
    // The address of the STUN server to use for server-reflexive addresses.
    // Supporting multiple servers would require multiple server-reflexive addresses.
    ACE_INET_Addr const stun_server_address_;

    // List of host addresses.
    AddressListType host_addresses_;
    // Server-reflexive address (from STUN server).
    ACE_INET_Addr server_reflexive_address_;

    ACE_UINT64 ice_tie_breaker_;

    GuidSetType unknown_guids_;

    typedef std::list<Checklist*> ChecklistSetType;
    ChecklistSetType checklists_;

    typedef std::map<DCPS::RepoId, ACE_INET_Addr, DCPS::GUID_tKeyLessThan> SelectedAddressesType;
    SelectedAddressesType selected_addresses_;

    ActiveFoundationSet active_foundations_;
    size_t remote_peer_reflexive_counter_;

    typedef std::list<ConnectivityCheck> ConnectivityChecksType;
    ConnectivityChecksType connectivity_checks_;

    typedef std::vector<DeferredTriggeredCheck> DeferredTriggeredCheckListType;
    typedef std::map<std::string, DeferredTriggeredCheckListType> DeferredTriggeredChecksType;
    DeferredTriggeredChecksType deferred_triggered_checks_;

    void regenerate_local_agent_info();
    bool do_next_check();

    // STUN Message processing.
    void request(const ACE_INET_Addr& local_address, const ACE_INET_Addr& remote_address, const STUN::Message& message);
    void success_response(const ACE_INET_Addr& local_address, const ACE_INET_Addr& remote_address, const STUN::Message& message);
    void error_response(const ACE_INET_Addr& remote_address, const STUN::Message& message);

    void succeeded(const ConnectivityCheck& cc);
    void failed(const ConnectivityCheck& cc);

    Checklist* add_checklist(const AgentInfo& remote_agent_info);
    void remove_checklist(Checklist* checklist);
    void generate_triggered_check(Checklist* checklist, const ACE_INET_Addr& local_address, const ACE_INET_Addr& remote_address, uint32_t priority, bool use_candidate);

    ACE_INET_Addr stun_server_address() const;
    ACE_INET_Addr server_reflexive_address() const;
    void server_reflexive_address(const ACE_INET_Addr& address);
    void send(const ACE_INET_Addr& address, const STUN::Message message);
    bool reactor_is_shut_down() const;

    // Hide the agent from its works to force all interaction through methods.
    // All of the invoked methods require locks.
    struct AgentWorkerView {
      AgentWorkerView(Agent& agent) : agent_(agent) {}
      bool is_running() const { return agent_.is_running(); }
      ACE_INET_Addr stun_server_address() const { return agent_.stun_server_address(); }
      ACE_INET_Addr server_reflexive_address() const { return agent_.server_reflexive_address(); }
      void server_reflexive_address(const ACE_INET_Addr& address) { agent_.server_reflexive_address(address); }
      void regenerate_local_agent_info() { agent_.regenerate_local_agent_info(); }
      void send(const ACE_INET_Addr& address, const STUN::Message message) { agent_.send(address, message); }
      bool do_next_check() { return agent_.do_next_check(); }
      bool reactor_is_shut_down() const { return agent_.reactor_is_shut_down(); }

    private:
      Agent& agent_;
    };

    struct CandidateGatherer : public DCPS::ReactorInterceptor {
      enum State {
        STOPPED,
        RETRANSMITTING,
        MAINTAINING
      };

      CandidateGatherer(Agent& a_agent, ACE_Reactor* reactor, ACE_thread_t owner)
        : DCPS::ReactorInterceptor(reactor, owner), agent_(a_agent), state_(STOPPED) {}

      void start();
      void stop();
      bool success_response(const STUN::Message& message);
    private:
      AgentWorkerView agent_;
      State state_;
      // Binding request sent to STUN server.
      STUN::Message binding_request_;

      static ACE_Time_Value gather_RTO();
      int handle_timeout(const ACE_Time_Value& /*now*/, const void* /*act*/);
      void form_request();
      void send_request();
      void schedule(State next_state, const ACE_Time_Value& delay);
      virtual bool reactor_is_shut_down() const;
    } candidate_gatherer_;

    struct ConnectivityChecker : public DCPS::ReactorInterceptor {
      enum State {
        STOPPED,
        CHECKING,
      };

      ConnectivityChecker(Agent& a_agent, ACE_Reactor* reactor, ACE_thread_t owner)
        : DCPS::ReactorInterceptor(reactor, owner), agent_(a_agent), state_(STOPPED) {}

      void start();
      void stop();

    private:
      AgentWorkerView agent_;
      State state_;

      int handle_timeout(const ACE_Time_Value& /*now*/, const void* /*act*/);
      void schedule(State next_state, const ACE_Time_Value& delay);
      virtual bool reactor_is_shut_down() const;
    } connectivity_checker_;

  };

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_H */
