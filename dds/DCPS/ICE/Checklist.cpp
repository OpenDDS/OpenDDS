/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Checklist.h"

#include "EndpointManager.h"

#include <iostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  const uint32_t PEER_REFLEXIVE_PRIORITY = (110 << 24) + (65535 << 8) + ((256 - 1) << 0);  // No local preference, component 1.

  CandidatePair::CandidatePair(const Candidate& a_local,
                               const Candidate& a_remote,
                               bool a_local_is_controlling,
                               bool a_use_candidate)
    : local(a_local),
      remote(a_remote),
      foundation(std::make_pair(a_local.foundation, a_remote.foundation)),
      local_is_controlling(a_local_is_controlling),
      priority(compute_priority()),
      use_candidate(a_use_candidate) {
    assert(!a_local.foundation.empty());
    assert(!a_remote.foundation.empty());
  }

  bool CandidatePair::operator==(const CandidatePair& other) const {
    return
      this->local == other.local &&
      this->remote == other.remote &&
      this->use_candidate == other.use_candidate;
  }

  uint64_t CandidatePair::compute_priority() {
    uint64_t const g = local_is_controlling ? local.priority : remote.priority;
    uint64_t const d = local_is_controlling ? remote.priority : local.priority;
    return (std::min(g,d) << 32) + 2 * std::max(g,d) + (g > d ? 1 : 0);
  }

  ConnectivityCheck::ConnectivityCheck(CandidatePair const & a_candidate_pair,
                                       AgentInfo const & a_local_agent_info, AgentInfo const & a_remote_agent_info,
                                       uint64_t a_ice_tie_breaker, ACE_Time_Value const & a_expiration_date)
    : m_candidate_pair(a_candidate_pair), m_cancelled(false), m_expiration_date(a_expiration_date) {
    m_request.class_ = STUN::REQUEST;
    m_request.method = STUN::BINDING;
    m_request.generate_transaction_id();
    m_request.append_attribute(STUN::make_priority(PEER_REFLEXIVE_PRIORITY));
    if (a_candidate_pair.local_is_controlling) {
      m_request.append_attribute(STUN::make_ice_controlling(a_ice_tie_breaker));
    } else {
      m_request.append_attribute(STUN::make_ice_controlled(a_ice_tie_breaker));
    }
    if (a_candidate_pair.local_is_controlling && a_candidate_pair.use_candidate) {
      m_request.append_attribute(STUN::make_use_candidate());
    }
    m_request.append_attribute(STUN::make_username(a_remote_agent_info.username + ":" + a_local_agent_info.username));
    m_request.password = a_remote_agent_info.password;
    m_request.append_attribute(STUN::make_message_integrity());
    m_request.append_attribute(STUN::make_fingerprint());
  }

  Checklist::Checklist(EndpointManager * a_endpoint_manager,
                       AgentInfo const & local, AgentInfo const & remote, ACE_UINT64 a_ice_tie_breaker)
    : Task(a_endpoint_manager->agent_impl)
    , m_scheduled_for_destruction(false)
    , m_endpoint_manager(a_endpoint_manager)
    , m_local_agent_info(local)
    , m_remote_agent_info(remote)
    , m_original_remote_agent_info(remote)
    , m_local_is_controlling(local.username < remote.username)
    , m_ice_tie_breaker(a_ice_tie_breaker)
    , m_nominating(m_valid_list.end())
    , m_nominated(m_valid_list.end())
    , m_nominated_is_live(false)
  {
    m_endpoint_manager->set_responsible_checklist(m_remote_agent_info.username, this);

    generate_candidate_pairs();
  }

  void Checklist::reset() {
    fix_foundations();

    for (ConnectivityChecksType::const_iterator pos = m_connectivity_checks.begin(),
           limit = m_connectivity_checks.end(); pos != limit; ++pos) {
      m_endpoint_manager->unset_responsible_checklist(pos->request().transaction_id, this);
    }

    m_frozen.clear();
    m_waiting.clear();
    m_in_progress.clear();
    m_succeeded.clear();
    m_failed.clear();
    m_triggered_check_queue.clear();
    m_valid_list.clear();
    m_nominating = m_valid_list.end();
    m_nominated = m_valid_list.end();
    m_nominated_is_live = false;
    m_check_interval = ACE_Time_Value();
    m_max_check_interval = ACE_Time_Value();
    m_connectivity_checks.clear();
  }

  void Checklist::generate_candidate_pairs() {
    // Add the candidate pairs.
    for (AgentInfo::CandidatesType::const_iterator local_pos = m_local_agent_info.candidates.begin(), local_limit = m_local_agent_info.candidates.end(); local_pos != local_limit; ++local_pos) {
      for (AgentInfo::CandidatesType::const_iterator remote_pos = m_remote_agent_info.candidates.begin(), remote_limit = m_remote_agent_info.candidates.end(); remote_pos != remote_limit; ++remote_pos) {
        m_frozen.push_back(CandidatePair(*local_pos, *remote_pos, m_local_is_controlling));
      }
    }

    // Sort by priority.
    m_frozen.sort(CandidatePair::priority_sorted);

    // Eliminate duplicates.
    for (CandidatePairsType::iterator pos = m_frozen.begin(), limit = m_frozen.end(); pos != limit; ++pos) {
      CandidatePairsType::iterator test_pos = pos;
      ++test_pos;
      while (test_pos != limit) {
        if (pos->local.base == test_pos->local.base && pos->remote == test_pos->remote) {
          m_frozen.erase(test_pos++);
        } else {
          ++test_pos;
        }
      }
    }

    if (m_frozen.size() != 0) {
      m_check_interval = m_endpoint_manager->agent_impl->get_configuration().T_a();
      double s = m_frozen.size();
      m_max_check_interval = m_endpoint_manager->agent_impl->get_configuration().checklist_period() * (1.0 / s);
      enqueue(ACE_Time_Value().now());
    }
  }

  void Checklist::compute_active_foundations(ActiveFoundationSet& active_foundations) const {
    for (CandidatePairsType::const_iterator pos = m_waiting.begin(), limit = m_waiting.end(); pos != limit; ++pos) {
      active_foundations.add(pos->foundation);
    }
    for (CandidatePairsType::const_iterator pos = m_in_progress.begin(), limit = m_in_progress.end(); pos != limit; ++pos) {
      active_foundations.add(pos->foundation);
    }
  }

  void Checklist::check_invariants() const {
    for (CandidatePairsType::const_iterator pos = m_valid_list.begin(), limit = m_valid_list.end(); pos != limit; ++pos) {
      assert(pos->use_candidate);
    }
  }

  void Checklist::unfreeze() {
    for (CandidatePairsType::const_iterator pos = m_frozen.begin(), limit = m_frozen.end(); pos != limit;) {
      const CandidatePair& cp = *pos;
      if (!m_endpoint_manager->agent_impl->active_foundations.contains(cp.foundation)) {
        m_endpoint_manager->agent_impl->active_foundations.add(cp.foundation);
        m_waiting.push_back(cp);
        m_waiting.sort(CandidatePair::priority_sorted);
        m_frozen.erase(pos++);
      } else {
        ++pos;
      }
    }
  }

  void Checklist::unfreeze(FoundationType const & a_foundation) {
    for (CandidatePairsType::const_iterator pos = m_frozen.begin(), limit = m_frozen.end(); pos != limit;) {
      const CandidatePair& cp = *pos;
      if (cp.foundation == a_foundation) {
        m_endpoint_manager->agent_impl->active_foundations.add(cp.foundation);
        m_waiting.push_back(cp);
        m_waiting.sort(CandidatePair::priority_sorted);
        m_frozen.erase(pos++);
      } else {
        ++pos;
      }
    }
  }

  void Checklist::add_valid_pair(const CandidatePair& valid_pair) {
    assert(valid_pair.use_candidate);
    m_valid_list.push_back(valid_pair);
    m_valid_list.sort(CandidatePair::priority_sorted);
  }

  void Checklist::fix_foundations() {
    for (CandidatePairsType::const_iterator pos = m_waiting.begin(), limit = m_waiting.end(); pos != limit; ++pos) {
      m_endpoint_manager->agent_impl->active_foundations.remove(pos->foundation);
    }
    for (CandidatePairsType::const_iterator pos = m_in_progress.begin(), limit = m_in_progress.end(); pos != limit; ++pos) {
      m_endpoint_manager->agent_impl->active_foundations.remove(pos->foundation);
    }
  }

  bool Checklist::get_local_candidate(const ACE_INET_Addr& address, Candidate& candidate) {
    for (AgentInfo::const_iterator pos = m_local_agent_info.begin(), limit = m_local_agent_info.end(); pos != limit; ++pos) {
      if (pos->address == address) {
        candidate = *pos;
        return true;
      }
    }
    return false;
  }

  bool Checklist::get_remote_candidate(const ACE_INET_Addr& address, Candidate& candidate) {
    for (AgentInfo::const_iterator pos = m_remote_agent_info.begin(), limit = m_remote_agent_info.end(); pos != limit; ++pos) {
      if (pos->address == address) {
        candidate = *pos;
        return true;
      }
    }
    return false;
  }

  void Checklist::add_triggered_check(CandidatePair const & a_candidate_pair) {
    if (m_nominated != m_valid_list.end()) {
      // Don't generate a check when we are done.
      return;
    }

    CandidatePairsType::const_iterator pos;

    pos = std::find(m_frozen.begin(), m_frozen.end(), a_candidate_pair);
    if (pos != m_frozen.end()) {
      m_frozen.erase(pos);
      m_endpoint_manager->agent_impl->active_foundations.add(a_candidate_pair.foundation);
      m_waiting.push_back(a_candidate_pair);
      m_waiting.sort(CandidatePair::priority_sorted);
      m_triggered_check_queue.push_back(a_candidate_pair);
      return;
    }

    pos = std::find(m_waiting.begin(), m_waiting.end(), a_candidate_pair);
    if (pos != m_waiting.end()) {
      // Done.
      return;
    }

    pos = std::find(m_in_progress.begin(), m_in_progress.end(), a_candidate_pair);
    if (pos != m_in_progress.end()) {
      // Duplicating to waiting.
      m_endpoint_manager->agent_impl->active_foundations.add(a_candidate_pair.foundation);
      m_waiting.push_back(a_candidate_pair);
      m_waiting.sort(CandidatePair::priority_sorted);
      m_triggered_check_queue.push_back(a_candidate_pair);
      return;
    }

    pos = std::find(m_succeeded.begin(), m_succeeded.end(), a_candidate_pair);
    if (pos != m_succeeded.end()) {
      // Done.
      return;
    }

    pos = std::find(m_failed.begin(), m_failed.end(), a_candidate_pair);
    if (pos != m_failed.end()) {
      m_failed.erase(pos);
      m_endpoint_manager->agent_impl->active_foundations.add(a_candidate_pair.foundation);
      m_waiting.push_back(a_candidate_pair);
      m_waiting.sort(CandidatePair::priority_sorted);
      m_triggered_check_queue.push_back(a_candidate_pair);
      return;
    }

    // Not in checklist.
    m_endpoint_manager->agent_impl->active_foundations.add(a_candidate_pair.foundation);
    m_waiting.push_back(a_candidate_pair);
    m_waiting.sort(CandidatePair::priority_sorted);
    m_triggered_check_queue.push_back(a_candidate_pair);
  }

  void Checklist::remove_from_in_progress(CandidatePair const & a_candidate_pair) {
    m_endpoint_manager->agent_impl->active_foundations.remove(a_candidate_pair.foundation);
    // Candidates can be in progress multiple times.
    CandidatePairsType::const_iterator pos = std::find(m_in_progress.begin(), m_in_progress.end(), a_candidate_pair);
    m_in_progress.erase(pos);
  }

  void Checklist::generate_triggered_check(const ACE_INET_Addr& local_address, const ACE_INET_Addr& remote_address,
                                           uint32_t priority,
                                           bool use_candidate) {
    Candidate remote;

    if (!get_remote_candidate(remote_address, remote)) {
      // 7.3.1.3
      remote = make_peer_reflexive_candidate(remote_address, priority, m_endpoint_manager->agent_impl->remote_peer_reflexive_counter());
      m_remote_agent_info.candidates.push_back(remote);
      std::sort(m_remote_agent_info.candidates.begin (), m_remote_agent_info.candidates.end (), candidates_sorted);
    }

    // 7.3.1.4
    Candidate local;
    bool flag = get_local_candidate(local_address, local);
    assert(flag);

    CandidatePair cp(local, remote, m_local_is_controlling, use_candidate);

    if (is_succeeded(cp)) {
      return;
    }

    if (is_in_progress(cp)) {
      ConnectivityChecksType::iterator pos = std::find(m_connectivity_checks.begin(), m_connectivity_checks.end(), cp);
      pos->cancel();
    }

    add_triggered_check(cp);
    // This can move something from failed to in progress.
    // In that case, we need to schedule.
    m_check_interval = m_endpoint_manager->agent_impl->get_configuration().T_a();
    enqueue(ACE_Time_Value().now());
  }

  void Checklist::succeeded(const ConnectivityCheck& cc) {
    const CandidatePair& cp = cc.candidate_pair();

    // 7.2.5.3.3
    // 7.2.5.4

    remove_from_in_progress(cp);
    m_succeeded.push_back(cp);
    m_succeeded.sort(CandidatePair::priority_sorted);

    if (cp.use_candidate) {
      if (m_local_is_controlling) {
        m_nominated = m_nominating;
        m_nominating = m_valid_list.end();
        assert(m_frozen.empty());
        assert(m_waiting.empty());
      } else {
        m_nominated = std::find(m_valid_list.begin(), m_valid_list.end(), cp);
        // This is the case where the use_candidate check succeeded before the normal check.
        if (m_nominated == m_valid_list.end()) {
          m_valid_list.push_front(cp);
          m_nominated = m_valid_list.begin();
        }

        while (!m_frozen.empty()) {
          CandidatePair cp = m_frozen.front();
          m_frozen.pop_front();
          m_failed.push_back(cp);
        }

        while (!m_waiting.empty()) {
          CandidatePair cp = m_waiting.front();
          m_waiting.pop_front();
          m_endpoint_manager->agent_impl->active_foundations.remove(cp.foundation);
          m_failed.push_back(cp);
        }
        m_triggered_check_queue.clear();
      }

      m_nominated_is_live = true;
      m_last_indication = ACE_Time_Value().now();

      while (!m_connectivity_checks.empty()) {
        ConnectivityCheck cc = m_connectivity_checks.front();
        m_connectivity_checks.pop_front();

        if (!cc.cancelled()) {
          failed(cc);
        } else {
          remove_from_in_progress(cc.candidate_pair());
        }
      }

      assert(m_frozen.empty());
      assert(m_waiting.empty());
      assert(m_triggered_check_queue.empty());
      assert(m_in_progress.empty());
      assert(m_connectivity_checks.empty());
    }

    m_endpoint_manager->agent_impl->unfreeze(cp.foundation);
  }

  void Checklist::failed(const ConnectivityCheck& cc) {
    const CandidatePair& cp = cc.candidate_pair();
    // 7.2.5.4
    remove_from_in_progress(cp);
    m_failed.push_back(cp);
    m_failed.sort(CandidatePair::priority_sorted);

    if (cp.use_candidate && m_local_is_controlling) {
      m_valid_list.pop_front();
      m_nominating = m_valid_list.end();
    }
  }

  void Checklist::success_response(ACE_INET_Addr const & local_address,
                                   ACE_INET_Addr const & remote_address,
                                   STUN::Message const & a_message) {
    ConnectivityChecksType::const_iterator pos = std::find(m_connectivity_checks.begin(), m_connectivity_checks.end(), a_message.transaction_id);
    assert(pos != m_connectivity_checks.end());

    ConnectivityCheck const cc = *pos;

    std::vector<STUN::AttributeType> unknown_attributes = a_message.unknown_comprehension_required_attributes();
    if (!unknown_attributes.empty()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING Unknown comprehension required attributes\n")));
      failed(cc);
      m_connectivity_checks.erase(pos);
      m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);
      return;
    }

    if (!a_message.has_fingerprint()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING No FINGERPRINT attribute\n")));
      failed(cc);
      m_connectivity_checks.erase(pos);
      m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);
      return;
    }

    ACE_INET_Addr mapped_address;
    if (!a_message.get_mapped_address(mapped_address)) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING No (XOR_)MAPPED_ADDRESS attribute\n")));
      failed(cc);
      m_connectivity_checks.erase(pos);
      m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);
      return;
    }

    if (!a_message.has_message_integrity()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING No MESSAGE_INTEGRITY attribute\n")));
      failed(cc);
      m_connectivity_checks.erase(pos);
      m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);
      return;
    }

    // Require integrity for checks.
    if (!a_message.verify_message_integrity(cc.request().password)) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING MESSAGE_INTEGRITY check failed\n")));
      failed(cc);
      m_connectivity_checks.erase(pos);
      m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);
      return;
    }

    // At this point the check will either succeed or fail so remove from the list.
    m_connectivity_checks.erase(pos);
    m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);

    const CandidatePair& cp = cc.candidate_pair();

    if (remote_address != cp.remote.address || local_address != cp.local.base) {
      // 7.2.5.2.1 Non-Symmetric Transport Addresses
      failed(cc);
      return;
    }

    succeeded(cc);

    if (cp.use_candidate) {
      return;
    }

    // 7.2.5.3.2 Constructing a Valid Pair
    Candidate local;

    if (!get_local_candidate(mapped_address, local)) {
      // 7.2.5.3.1 Discovering Peer-Reflexive Candidates
      uint32_t priority;
      // Our message, no need to check.
      cc.request().get_priority(priority);
      local = make_peer_reflexive_candidate(mapped_address, cp.local.base, cp.remote.address, priority);
      m_local_agent_info.candidates.push_back(local);
      std::sort(m_local_agent_info.candidates.begin (), m_local_agent_info.candidates.end (), candidates_sorted);
    }

    // The valid pair
    CandidatePair vp(local, cp.remote, m_local_is_controlling, true);

    add_valid_pair(vp);
  }

  void Checklist::error_response(ACE_INET_Addr const & /*local_address*/,
                                 ACE_INET_Addr const & /*remote_address*/,
                                 STUN::Message const & a_message) {
    ConnectivityChecksType::const_iterator pos = std::find(m_connectivity_checks.begin(), m_connectivity_checks.end(), a_message.transaction_id);
    assert(pos != m_connectivity_checks.end());

    ConnectivityCheck const cc = *pos;

    if (!a_message.has_message_integrity()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING No MESSAGE_INTEGRITY attribute\n")));
      // Retry.
      return;
    }

    if (!a_message.verify_message_integrity(cc.request().password)) {
      // Retry.
      return;
    }

    // We have a verified error response.
    std::vector<STUN::AttributeType> unknown_attributes = a_message.unknown_comprehension_required_attributes();
    if (!unknown_attributes.empty()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING Unknown comprehension required attributes\n")));
      failed(cc);
      m_connectivity_checks.erase(pos);
      m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);
      return;
    }

    if (!a_message.has_fingerprint()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING No FINGERPRINT attribute\n")));
      failed(cc);
      m_connectivity_checks.erase(pos);
      m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);
      return;
    }

    if (a_message.has_error_code()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING STUN error response code=%d reason=%s\n"), a_message.get_error_code(), a_message.get_error_reason().c_str()));
      if (a_message.get_error_code() == 420 && a_message.has_unknown_attributes()) {
        std::vector<STUN::AttributeType> unknown_attributes = a_message.get_unknown_attributes();
        for (std::vector<STUN::AttributeType>::const_iterator pos = unknown_attributes.begin(),
               limit = unknown_attributes.end(); pos != limit; ++pos) {
          ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING Unknown STUN attribute %d\n"), *pos));
        }
      }
      if (a_message.get_error_code() == 400 && a_message.get_error_code() == 420) {
        // Waiting and/or resending won't fix these errors.
        failed(cc);
        m_connectivity_checks.erase(pos);
        m_endpoint_manager->unset_responsible_checklist(cc.request().transaction_id, this);
      }
    } else {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING STUN error response (no code)\n")));
    }
  }

  void Checklist::do_next_check(ACE_Time_Value const & a_now) {
    // Triggered checks.
    if (!m_triggered_check_queue.empty()) {
      CandidatePair cp = m_triggered_check_queue.front();
      m_triggered_check_queue.pop_front();

      ConnectivityCheck cc(cp, m_local_agent_info, m_remote_agent_info, m_ice_tie_breaker, a_now + m_endpoint_manager->agent_impl->get_configuration().connectivity_check_ttl());

      m_waiting.remove(cp);
      m_in_progress.push_back(cp);
      m_in_progress.sort(CandidatePair::priority_sorted);

      m_endpoint_manager->endpoint->send(cc.candidate_pair().remote.address, cc.request());
      m_connectivity_checks.push_back(cc);
      m_endpoint_manager->set_responsible_checklist(cc.request().transaction_id, this);
      m_check_interval = m_endpoint_manager->agent_impl->get_configuration().T_a();
      return;
    }

    unfreeze();

    // Ordinary check.
    if (!m_waiting.empty()) {
      CandidatePair cp = m_waiting.front();
      m_waiting.pop_front();

      ConnectivityCheck cc(cp, m_local_agent_info, m_remote_agent_info, m_ice_tie_breaker, a_now + m_endpoint_manager->agent_impl->get_configuration().connectivity_check_ttl());

      m_in_progress.push_back(cp);
      m_in_progress.sort(CandidatePair::priority_sorted);

      m_endpoint_manager->endpoint->send(cc.candidate_pair().remote.address, cc.request());
      m_connectivity_checks.push_back(cc);
      m_endpoint_manager->set_responsible_checklist(cc.request().transaction_id, this);
      m_check_interval = m_endpoint_manager->agent_impl->get_configuration().T_a();
      return;
    }

    // Retry.
    while (!m_connectivity_checks.empty()) {
      ConnectivityCheck cc = m_connectivity_checks.front();
      m_connectivity_checks.pop_front();

      if (cc.expiration_date() < a_now) {
        if (!cc.cancelled()) {
          // Failing can allow nomination to proceed.
          failed(cc);
        } else {
          remove_from_in_progress(cc.candidate_pair());
        }
        continue;
      }

      // We leave the cancelled checks in case we get a response.
      if (!cc.cancelled()) {
        // Reset the password in the event that it changed.
        cc.password(m_remote_agent_info.password);
        m_endpoint_manager->endpoint->send(cc.candidate_pair().remote.address, cc.request());
      }
      m_connectivity_checks.push_back(cc);

      // Backoff.
      m_check_interval = std::min(m_check_interval * 2, m_max_check_interval);
      break;
    }

    // Waiting for the remote.
    m_check_interval = m_endpoint_manager->agent_impl->get_configuration().checklist_period();
  }

  void Checklist::execute(ACE_Time_Value const & a_now) {
    if (m_scheduled_for_destruction) {
      delete this;
      return;
    }

    // Nominating check.
    if (m_frozen.empty() &&
        m_waiting.empty() &&
        // m_in_progress.empty() &&
        m_local_is_controlling &&
        !m_valid_list.empty() &&
        m_nominating == m_valid_list.end() &&
        m_nominated == m_valid_list.end()) {
      add_triggered_check(m_valid_list.front());
      m_nominating = m_valid_list.begin();
    }

    bool flag = false;
    ACE_Time_Value interval = std::max(m_check_interval, m_endpoint_manager->agent_impl->get_configuration().indication_period());

    if (!m_triggered_check_queue.empty() ||
        !m_frozen.empty() ||
        !m_waiting.empty() ||
        !m_connectivity_checks.empty()) {
      do_next_check(a_now);
      flag = true;
      interval = std::min(interval, m_check_interval);
    }

    if (m_nominated != m_valid_list.end()) {
      // Send an indication.
      STUN::Message message;
      message.class_ = STUN::INDICATION;
      message.method = STUN::BINDING;
      message.generate_transaction_id();
      message.append_attribute(STUN::make_username(m_remote_agent_info.username + ":" + m_local_agent_info.username));
      message.password = m_remote_agent_info.password;
      message.append_attribute(STUN::make_message_integrity());
      message.append_attribute(STUN::make_fingerprint());
      m_endpoint_manager->endpoint->send(selected_address(), message);
      flag = true;
      interval = std::min(interval, m_endpoint_manager->agent_impl->get_configuration().indication_period());

      // Check that we are receiving indications.
      m_nominated_is_live = (a_now - m_last_indication) < m_endpoint_manager->agent_impl->get_configuration().nominated_ttl();
    }

    if (flag) {
      enqueue(ACE_Time_Value().now() + interval);
    }

    // The checklist has failed.  Don't schedule.
  }

  void Checklist::add_guid(GuidPair const & a_guid_pair) {
    m_guids.insert(a_guid_pair);
    m_endpoint_manager->set_responsible_checklist(a_guid_pair, this);
  }

  void Checklist::remove_guid(GuidPair const & a_guid_pair) {
    m_guids.erase(a_guid_pair);
    m_endpoint_manager->unset_responsible_checklist(a_guid_pair, this);
    if (m_guids.empty()) {
      // Cleanup this checklist.
      m_endpoint_manager->unset_responsible_checklist(m_remote_agent_info.username, this);
      reset();
      m_scheduled_for_destruction = true;

      // Flush ourselves out of the task queue.
      // Schedule for now but it may be later.
      enqueue(ACE_Time_Value().now());
    }
  }

  void Checklist::add_guids(GuidSetType const & a_guids) {
    for (GuidSetType::const_iterator pos = a_guids.begin(), limit = a_guids.end(); pos != limit; ++pos) {
      add_guid(*pos);
    }
  }

  void Checklist::remove_guids() {
    GuidSetType guids = m_guids;
    for (GuidSetType::const_iterator pos = guids.begin(), limit = guids.end(); pos != limit; ++pos) {
      remove_guid(*pos);
    }
  }

  ACE_INET_Addr Checklist::selected_address() const {
    if (m_nominated_is_live && m_nominated != m_valid_list.end()) {
      return m_nominated->remote.address;
    }
    return ACE_INET_Addr();
  }

  void Checklist::indication() {
    m_last_indication = ACE_Time_Value().now();
  }
} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
