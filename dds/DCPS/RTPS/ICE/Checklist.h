/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY
#ifndef OPENDDS_RTPS_ICE_CHECKLIST_H
#define OPENDDS_RTPS_ICE_CHECKLIST_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/TimeTypes.h"

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
  ACE_UINT64 const priority;
  // 1) set the use_candidate when local is controlling and
  // 2) nominate when the response is successful (controlling and controlled)
  bool const use_candidate;

  CandidatePair(const Candidate& a_local,
                const Candidate& a_remote,
                bool a_local_is_controlling,
                bool a_use_candidate = false);

  bool operator==(const CandidatePair& a_other) const;

  static bool priority_sorted(const CandidatePair& x, const CandidatePair& y)
  {
    return x.priority > y.priority;
  }

private:
  ACE_UINT64 compute_priority();
};

struct ConnectivityCheck {
  ConnectivityCheck(const CandidatePair& a_candidate_pair,
                    const AgentInfo& a_local_agent_info, const AgentInfo& a_remote_agent_info,
                    ACE_UINT64 a_ice_tie_breaker, const DCPS::MonotonicTimePoint& a_expiration_date);

  const CandidatePair& candidate_pair() const
  {
    return candiate_pair_;
  }
  const STUN::Message& request() const
  {
    return request_;
  }
  void cancel()
  {
    cancelled_ = true;
  }
  bool cancelled() const
  {
    return cancelled_;
  }
  DCPS::MonotonicTimePoint expiration_date() const
  {
    return expiration_date_;
  }
  void password(const std::string& a_password)
  {
    request_.password = a_password;
  }
private:
  CandidatePair candiate_pair_;
  STUN::Message request_;
  bool cancelled_;
  DCPS::MonotonicTimePoint expiration_date_;
};

inline bool operator==(const ConnectivityCheck& a_cc, const STUN::TransactionId& a_tid)
{
  return a_cc.request().transaction_id == a_tid;
}

inline bool operator==(const ConnectivityCheck& a_cc, const CandidatePair& a_cp)
{
  return a_cc.candidate_pair() == a_cp;
}

#if !OPENDDS_SAFETY_PROFILE
inline std::ostream& operator<<(std::ostream& stream, const GuidPair& guidp)
{
  stream << guidp.local << ':' << guidp.remote;
  return stream;
}
#endif

struct Checklist : public Task {
  Checklist(EndpointManager* a_endpoint,
            const AgentInfo& a_local, const AgentInfo& a_remote,
            ACE_UINT64 a_ice_tie_breaker);

  void compute_active_foundations(ActiveFoundationSet& a_active_foundations) const;

  void check_invariants() const;

  void unfreeze();

  void unfreeze(const FoundationType& a_foundation);

  void generate_triggered_check(const ACE_INET_Addr& a_local_address, const ACE_INET_Addr& a_remote_address,
                                ACE_UINT32 a_priority,
                                bool a_use_candidate);

  void success_response(const ACE_INET_Addr& a_local_address,
                        const ACE_INET_Addr& a_remote_address,
                        const STUN::Message& a_message);

  void error_response(const ACE_INET_Addr& a_local_address,
                      const ACE_INET_Addr& a_remote_address,
                      const STUN::Message& a_message);

  void add_guid(const GuidPair& a_guid_pair);

  void remove_guid(const GuidPair& a_guid_pair);

  void add_guids(const GuidSetType& a_guids);

  void remove_guids();

  GuidSetType guids() const
  {
    return guids_;
  }

  ACE_INET_Addr selected_address() const;

  const AgentInfo& original_remote_agent_info() const
  {
    return original_remote_agent_info_;
  }

  void set_remote_password(const std::string& a_password)
  {
    remote_agent_info_.password = a_password;
    original_remote_agent_info_.password = a_password;
  }

  void indication();

private:
  bool scheduled_for_destruction_;
  EndpointManager* const endpoint_manager_;
  GuidSetType guids_;
  AgentInfo local_agent_info_;
  AgentInfo remote_agent_info_;
  AgentInfo original_remote_agent_info_;
  bool const local_is_controlling_;
  ACE_UINT64 const ice_tie_breaker_;

  typedef std::list<CandidatePair> CandidatePairsType;
  CandidatePairsType frozen_;
  CandidatePairsType waiting_;
  CandidatePairsType in_progress_;
  CandidatePairsType succeeded_;
  CandidatePairsType failed_;
  // The triggered check queue is a subset of waiting.
  CandidatePairsType triggered_check_queue_;
  // The valid list is a subset of succeeded.
  CandidatePairsType valid_list_;
  // These are iterators into valid_list_.
  CandidatePairsType::const_iterator nominating_;
  CandidatePairsType::const_iterator nominated_;
  bool nominated_is_live_;
  DCPS::MonotonicTimePoint last_indication_;
  DCPS::TimeDuration check_interval_;
  DCPS::TimeDuration max_check_interval_;
  typedef std::list<ConnectivityCheck> ConnectivityChecksType;
  ConnectivityChecksType connectivity_checks_;

  ~Checklist() {}

  void reset();

  void generate_candidate_pairs();

  void fix_foundations();

  void add_valid_pair(const CandidatePair& a_valid_pair);

  bool get_local_candidate(const ACE_INET_Addr& a_address, Candidate& a_candidate);

  bool get_remote_candidate(const ACE_INET_Addr& a_address, Candidate& a_candidate);

  bool is_succeeded(const CandidatePair& a_candidate_pair) const
  {
    return std::find(succeeded_.begin(), succeeded_.end(), a_candidate_pair) != succeeded_.end();
  }

  bool is_in_progress(const CandidatePair& a_candidate_pair) const
  {
    return std::find(in_progress_.begin(), in_progress_.end(), a_candidate_pair) != in_progress_.end();
  }

  void add_triggered_check(const CandidatePair& a_candidate_pair);

  void remove_from_in_progress(const CandidatePair& a_candidate_pair);

  void succeeded(const ConnectivityCheck& a_connectivity_check);

  void failed(const ConnectivityCheck& a_connectivity_check);

  // Measure of now many checks are left.
  size_t size() const
  {
    return frozen_.size() + waiting_.size() + in_progress_.size();
  }

  void do_next_check(const DCPS::MonotonicTimePoint& a_now);

  void execute(const DCPS::MonotonicTimePoint& a_now);
};

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_CHECKLIST_H */
#endif /* OPENDDS_SECURITY */
