/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_ICE_CHECKLIST_H
#define OPENDDS_RTPS_ICE_CHECKLIST_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Task.h"
#include "AgentImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  struct CandidatePair {
    Candidate const local;
    Candidate const remote;
    FoundationType const foundation;
    bool const local_is_controlling;
    uint64_t const priority;
    // 1) set the use_candidate when local is controlling and
    // 2) nominate when the response is successful (controlling and controlled)
    bool const use_candidate;

    CandidatePair(Candidate const & a_local,
                  Candidate const & a_remote,
                  bool a_local_is_controlling,
                  bool a_use_candidate = false);

    bool operator==(CandidatePair const & a_other) const;

    static bool priority_sorted(CandidatePair const & x, CandidatePair const & y) { return x.priority > y.priority; }

  private:
    uint64_t compute_priority();
  };

  struct ConnectivityCheck {
    ConnectivityCheck(CandidatePair const & a_candidate_pair,
                      AgentInfo const & a_local_agent_info, AgentInfo const & a_remote_agent_info,
                      uint64_t a_ice_tie_breaker, ACE_Time_Value const & a_expiration_date);

    CandidatePair const & candidate_pair() const { return m_candidate_pair; }
    STUN::Message const & request() const { return m_request; }
    void cancel() { m_cancelled = true; }
    bool cancelled() const { return m_cancelled; }
    ACE_Time_Value expiration_date() const { return m_expiration_date; }
    void password(std::string const & a_password) { m_request.password = a_password; }
  private:
    CandidatePair m_candidate_pair;
    STUN::Message m_request;
    bool m_cancelled;
    ACE_Time_Value m_expiration_date;
  };

  inline bool operator==(ConnectivityCheck const & a_cc, STUN::TransactionId const & a_tid) {
    return a_cc.request().transaction_id == a_tid;
  }

  inline bool operator==(ConnectivityCheck const & a_cc, CandidatePair const & a_cp) {
    return a_cc.candidate_pair() == a_cp;
  }

  struct GuidPair {
    DCPS::RepoId local;
    DCPS::RepoId remote;

    GuidPair(DCPS::RepoId const & a_local, DCPS::RepoId const & a_remote) : local(a_local), remote(a_remote) {}

    bool operator< (GuidPair const & a_other) const {
      if (DCPS::GUID_tKeyLessThan() (this->local, a_other.local)) return true;
      if (DCPS::GUID_tKeyLessThan() (a_other.local, this->local)) return false;
      if (DCPS::GUID_tKeyLessThan() (this->remote, a_other.remote)) return true;
      if (DCPS::GUID_tKeyLessThan() (a_other.remote, this->remote)) return false;
      return false;
    }
  };

  inline std::ostream& operator<<(std::ostream& stream, GuidPair const & guidp) {
    stream << guidp.local << ':' << guidp.remote;
    return stream;
  }

  typedef std::set<GuidPair> GuidSetType;

  struct Checklist : public Task {
    Checklist(EndpointManager * a_endpoint,
              AgentInfo const & a_local, AgentInfo const & a_remote,
              ACE_UINT64 a_ice_tie_breaker);

    void compute_active_foundations(ActiveFoundationSet & a_active_foundations) const;

    void check_invariants() const;

    void unfreeze();

    void unfreeze(FoundationType const & a_foundation);

    void generate_triggered_check(ACE_INET_Addr const & a_local_address, ACE_INET_Addr const & a_remote_address,
                                  uint32_t a_priority,
                                  bool a_use_candidate);

    void success_response(ACE_INET_Addr const & a_local_address,
                          ACE_INET_Addr const & a_remote_address,
                          STUN::Message const & a_message);

    void error_response(ACE_INET_Addr const & a_local_address,
                        ACE_INET_Addr const & a_remote_address,
                        STUN::Message const & a_message);

    void add_guid(GuidPair const & a_guid_pair);

    void remove_guid(GuidPair const & a_guid_pair);

    void add_guids(GuidSetType const & a_guids);

    void remove_guids();

    GuidSetType guids() const { return m_guids; }

    ACE_INET_Addr selected_address() const;

    AgentInfo const & original_remote_agent_info() const { return m_original_remote_agent_info; }

    void set_remote_password(std::string const & a_password) {
      m_remote_agent_info.password = a_password;
      m_original_remote_agent_info.password = a_password;
    }

    void indication();

  private:
    bool m_scheduled_for_destruction;
    EndpointManager * const m_endpoint_manager;
    GuidSetType m_guids;
    AgentInfo m_local_agent_info;
    AgentInfo m_remote_agent_info;
    AgentInfo m_original_remote_agent_info;
    bool const m_local_is_controlling;
    ACE_UINT64 const m_ice_tie_breaker;

    typedef std::list<CandidatePair> CandidatePairsType;
    CandidatePairsType m_frozen;
    CandidatePairsType m_waiting;
    CandidatePairsType m_in_progress;
    CandidatePairsType m_succeeded;
    CandidatePairsType m_failed;
    // The triggered check queue is a subset of waiting.
    CandidatePairsType m_triggered_check_queue;
    // The valid list is a subset of succeeded.
    CandidatePairsType m_valid_list;
    // These are iterators into m_valid_list.
    CandidatePairsType::const_iterator m_nominating;
    CandidatePairsType::const_iterator m_nominated;
    ACE_Time_Value m_nominated_is_live;
    ACE_Time_Value m_last_indication;
    ACE_Time_Value m_check_interval;
    ACE_Time_Value m_max_check_interval;
    typedef std::list<ConnectivityCheck> ConnectivityChecksType;
    ConnectivityChecksType m_connectivity_checks;

    ~Checklist() {}

    void reset();

    void generate_candidate_pairs();

    void fix_foundations();

    void add_valid_pair(CandidatePair const & a_valid_pair);

    bool get_local_candidate(ACE_INET_Addr const & a_address, Candidate & a_candidate);

    bool get_remote_candidate(ACE_INET_Addr const & a_address, Candidate & a_candidate);

    bool is_succeeded(CandidatePair const & a_candidate_pair) const {
      return std::find(m_succeeded.begin(), m_succeeded.end(), a_candidate_pair) != m_succeeded.end();
    }

    bool is_in_progress(CandidatePair const & a_candidate_pair) const {
      return std::find(m_in_progress.begin(), m_in_progress.end(), a_candidate_pair) != m_in_progress.end();
    }

    void add_triggered_check(CandidatePair const & a_candidate_pair);

    void remove_from_in_progress(CandidatePair const & a_candidate_pair);

    void succeeded(ConnectivityCheck const & a_connectivity_check);

    void failed(ConnectivityCheck const & a_connectivity_check);

    // Measure of now many checks are left.
    size_t size() const {
      return m_frozen.size() + m_waiting.size() + m_in_progress.size();
    }

    void do_next_check(ACE_Time_Value const & a_now);

    void execute(ACE_Time_Value const & a_now);
  };

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_CHECKLIST_H */
