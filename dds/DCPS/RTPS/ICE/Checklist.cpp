/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "Checklist.h"

#include "EndpointManager.h"
#include "Ice.h"

#include "dds/DCPS/Definitions.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

using OpenDDS::DCPS::MonotonicTimePoint;
using OpenDDS::DCPS::TimeDuration;

const ACE_UINT32 PEER_REFLEXIVE_PRIORITY = (110 << 24) + (65535 << 8) + ((256 - 1) << 0);  // No local preference, component 1.

CandidatePair::CandidatePair(const Candidate& a_local,
                             const Candidate& a_remote,
                             bool a_local_is_controlling,
                             bool a_use_candidate)
  : local(a_local),
    remote(a_remote),
    foundation(std::make_pair(a_local.foundation, a_remote.foundation)),
    local_is_controlling(a_local_is_controlling),
    priority(compute_priority()),
    use_candidate(a_use_candidate)
{
  OPENDDS_ASSERT(!a_local.foundation.empty());
  OPENDDS_ASSERT(!a_remote.foundation.empty());
}

bool CandidatePair::operator==(const CandidatePair& other) const
{
  return
    this->local == other.local &&
    this->remote == other.remote &&
    this->use_candidate == other.use_candidate;
}

ACE_UINT64 CandidatePair::compute_priority()
{
  ACE_UINT64 const g = local_is_controlling ? local.priority : remote.priority;
  ACE_UINT64 const d = local_is_controlling ? remote.priority : local.priority;
  return (std::min(g,d) << 32) + 2 * std::max(g,d) + (g > d ? 1 : 0);
}

ConnectivityCheck::ConnectivityCheck(const CandidatePair& a_candidate_pair,
                                     const AgentInfo& a_local_agent_info, const AgentInfo& a_remote_agent_info,
                                     ACE_UINT64 a_ice_tie_breaker, const MonotonicTimePoint& a_expiration_date)
  : candiate_pair_(a_candidate_pair), cancelled_(false), expiration_date_(a_expiration_date)
{
  request_.class_ = STUN::REQUEST;
  request_.method = STUN::BINDING;
  request_.generate_transaction_id();
  request_.append_attribute(STUN::make_priority(PEER_REFLEXIVE_PRIORITY));

  if (a_candidate_pair.local_is_controlling) {
    request_.append_attribute(STUN::make_ice_controlling(a_ice_tie_breaker));
  }

  else {
    request_.append_attribute(STUN::make_ice_controlled(a_ice_tie_breaker));
  }

  if (a_candidate_pair.local_is_controlling && a_candidate_pair.use_candidate) {
    request_.append_attribute(STUN::make_use_candidate());
  }

  request_.append_attribute(STUN::make_username(a_remote_agent_info.username + ":" + a_local_agent_info.username));
  request_.password = a_remote_agent_info.password;
  request_.append_attribute(STUN::make_message_integrity());
  request_.append_attribute(STUN::make_fingerprint());
}

Checklist::Checklist(EndpointManager* a_endpoint_manager,
                     const AgentInfo& local, const AgentInfo& remote, ACE_UINT64 a_ice_tie_breaker)
  : Task(a_endpoint_manager->agent_impl)
  , scheduled_for_destruction_(false)
  , endpoint_manager_(a_endpoint_manager)
  , local_agent_info_(local)
  , remote_agent_info_(remote)
  , original_remote_agent_info_(remote)
  , local_is_controlling_(local.username < remote.username)
  , ice_tie_breaker_(a_ice_tie_breaker)
  , nominating_(valid_list_.end())
  , nominated_(valid_list_.end())
  , nominated_is_live_(false)
{
  endpoint_manager_->set_responsible_checklist(remote_agent_info_.username, this);

  generate_candidate_pairs();
}

void Checklist::reset()
{
  fix_foundations();

  for (ConnectivityChecksType::const_iterator pos = connectivity_checks_.begin(),
       limit = connectivity_checks_.end(); pos != limit; ++pos) {
    endpoint_manager_->unset_responsible_checklist(pos->request().transaction_id, this);
  }

  frozen_.clear();
  waiting_.clear();
  in_progress_.clear();
  succeeded_.clear();
  failed_.clear();
  triggered_check_queue_.clear();
  valid_list_.clear();
  nominating_ = valid_list_.end();
  nominated_ = valid_list_.end();
  nominated_is_live_ = false;
  check_interval_ = TimeDuration::zero_value;
  max_check_interval_ = TimeDuration::zero_value;
  connectivity_checks_.clear();
}

void Checklist::generate_candidate_pairs()
{
  // Add the candidate pairs.
  AgentInfo::CandidatesType::const_iterator local_pos = local_agent_info_.candidates.begin();
  AgentInfo::CandidatesType::const_iterator local_limit = local_agent_info_.candidates.end();
  for (; local_pos != local_limit; ++local_pos) {
    AgentInfo::CandidatesType::const_iterator remote_pos = remote_agent_info_.candidates.begin();
    AgentInfo::CandidatesType::const_iterator remote_limit = remote_agent_info_.candidates.end();
    for (; remote_pos != remote_limit; ++remote_pos) {
      frozen_.push_back(CandidatePair(*local_pos, *remote_pos, local_is_controlling_));
    }
  }

  // Sort by priority.
  frozen_.sort(CandidatePair::priority_sorted);

  // Eliminate duplicates.
  for (CandidatePairsType::iterator pos = frozen_.begin(), limit = frozen_.end(); pos != limit; ++pos) {
    CandidatePairsType::iterator test_pos = pos;
    ++test_pos;

    while (test_pos != limit) {
      if (pos->local.base == test_pos->local.base && pos->remote == test_pos->remote) {
        frozen_.erase(test_pos++);
      }

      else {
        ++test_pos;
      }
    }
  }

  if (frozen_.size() != 0) {
    check_interval_ = endpoint_manager_->agent_impl->get_configuration().T_a();
    double s = static_cast<double>(frozen_.size());
    max_check_interval_ = endpoint_manager_->agent_impl->get_configuration().checklist_period() * (1.0 / s);
  }
}

void Checklist::compute_active_foundations(ActiveFoundationSet& active_foundations) const
{
  for (CandidatePairsType::const_iterator pos = waiting_.begin(), limit = waiting_.end(); pos != limit; ++pos) {
    active_foundations.add(pos->foundation);
  }

  for (CandidatePairsType::const_iterator pos = in_progress_.begin(), limit = in_progress_.end(); pos != limit; ++pos) {
    active_foundations.add(pos->foundation);
  }
}

void Checklist::check_invariants() const
{
  for (CandidatePairsType::const_iterator pos = valid_list_.begin(), limit = valid_list_.end(); pos != limit; ++pos) {
    OPENDDS_ASSERT(pos->use_candidate);
  }
}

void Checklist::unfreeze()
{
  bool flag = false;

  for (CandidatePairsType::iterator pos = frozen_.begin(), limit = frozen_.end(); pos != limit;) {
    const CandidatePair& cp = *pos;

    if (!endpoint_manager_->agent_impl->contains(cp.foundation)) {
      endpoint_manager_->agent_impl->add(cp.foundation);
      waiting_.push_back(cp);
      waiting_.sort(CandidatePair::priority_sorted);
      frozen_.erase(pos++);
      flag = true;
    } else {
      ++pos;
    }
  }

  if (flag) {
    enqueue(MonotonicTimePoint::now());
  }
}

void Checklist::unfreeze(const FoundationType& a_foundation)
{
  bool flag = false;

  for (CandidatePairsType::iterator pos = frozen_.begin(), limit = frozen_.end(); pos != limit;) {
    const CandidatePair& cp = *pos;

    if (cp.foundation == a_foundation) {
      endpoint_manager_->agent_impl->add(cp.foundation);
      waiting_.push_back(cp);
      waiting_.sort(CandidatePair::priority_sorted);
      frozen_.erase(pos++);
      flag = true;
    } else {
      ++pos;
    }
  }

  if (flag) {
    enqueue(MonotonicTimePoint::now());
  }
}

void Checklist::add_valid_pair(const CandidatePair& valid_pair)
{
  OPENDDS_ASSERT(valid_pair.use_candidate);
  valid_list_.push_back(valid_pair);
  valid_list_.sort(CandidatePair::priority_sorted);
}

void Checklist::fix_foundations()
{
  for (CandidatePairsType::const_iterator pos = waiting_.begin(), limit = waiting_.end(); pos != limit; ++pos) {
    endpoint_manager_->agent_impl->remove(pos->foundation);
  }

  for (CandidatePairsType::const_iterator pos = in_progress_.begin(), limit = in_progress_.end(); pos != limit; ++pos) {
    endpoint_manager_->agent_impl->remove(pos->foundation);
  }
}

bool Checklist::get_local_candidate(const ACE_INET_Addr& address, Candidate& candidate)
{
  for (AgentInfo::const_iterator pos = local_agent_info_.begin(), limit = local_agent_info_.end(); pos != limit; ++pos) {
    if (pos->address == address) {
      candidate = *pos;
      return true;
    }
  }

  return false;
}

bool Checklist::get_remote_candidate(const ACE_INET_Addr& address, Candidate& candidate)
{
  for (AgentInfo::const_iterator pos = remote_agent_info_.begin(), limit = remote_agent_info_.end(); pos != limit; ++pos) {
    if (pos->address == address) {
      candidate = *pos;
      return true;
    }
  }

  return false;
}

void Checklist::add_triggered_check(const CandidatePair& a_candidate_pair)
{
  if (nominated_ != valid_list_.end()) {
    // Don't generate a check when we are done.
    return;
  }

  CandidatePairsType::iterator pos;

  pos = std::find(frozen_.begin(), frozen_.end(), a_candidate_pair);

  if (pos != frozen_.end()) {
    frozen_.erase(pos);
    endpoint_manager_->agent_impl->add(a_candidate_pair.foundation);
    waiting_.push_back(a_candidate_pair);
    waiting_.sort(CandidatePair::priority_sorted);
    triggered_check_queue_.push_back(a_candidate_pair);
    return;
  }

  pos = std::find(waiting_.begin(), waiting_.end(), a_candidate_pair);

  if (pos != waiting_.end()) {
    // Done.
    return;
  }

  pos = std::find(in_progress_.begin(), in_progress_.end(), a_candidate_pair);

  if (pos != in_progress_.end()) {
    // Duplicating to waiting.
    endpoint_manager_->agent_impl->add(a_candidate_pair.foundation);
    waiting_.push_back(a_candidate_pair);
    waiting_.sort(CandidatePair::priority_sorted);
    triggered_check_queue_.push_back(a_candidate_pair);
    return;
  }

  pos = std::find(succeeded_.begin(), succeeded_.end(), a_candidate_pair);

  if (pos != succeeded_.end()) {
    // Done.
    return;
  }

  pos = std::find(failed_.begin(), failed_.end(), a_candidate_pair);

  if (pos != failed_.end()) {
    failed_.erase(pos);
    endpoint_manager_->agent_impl->add(a_candidate_pair.foundation);
    waiting_.push_back(a_candidate_pair);
    waiting_.sort(CandidatePair::priority_sorted);
    triggered_check_queue_.push_back(a_candidate_pair);
    return;
  }

  // Not in checklist.
  endpoint_manager_->agent_impl->add(a_candidate_pair.foundation);
  waiting_.push_back(a_candidate_pair);
  waiting_.sort(CandidatePair::priority_sorted);
  triggered_check_queue_.push_back(a_candidate_pair);
}

void Checklist::remove_from_in_progress(const CandidatePair& a_candidate_pair)
{
  endpoint_manager_->agent_impl->remove(a_candidate_pair.foundation);
  // Candidates can be in progress multiple times.
  CandidatePairsType::iterator pos = std::find(in_progress_.begin(), in_progress_.end(), a_candidate_pair);
  in_progress_.erase(pos);
}

void Checklist::generate_triggered_check(const ACE_INET_Addr& local_address,
                                         const ACE_INET_Addr& remote_address,
                                         ACE_UINT32 priority,
                                         bool use_candidate)
{
  Candidate remote;

  if (!get_remote_candidate(remote_address, remote)) {
    // 7.3.1.3
    remote = make_peer_reflexive_candidate(remote_address, priority, endpoint_manager_->agent_impl->remote_peer_reflexive_counter());
    remote_agent_info_.candidates.push_back(remote);
    std::sort(remote_agent_info_.candidates.begin(), remote_agent_info_.candidates.end(), candidates_sorted);
  }

  // 7.3.1.4
  Candidate local;
  bool flag = get_local_candidate(local_address, local);
  OPENDDS_ASSERT(flag);

  CandidatePair cp(local, remote, local_is_controlling_, use_candidate);

  if (is_succeeded(cp)) {
    return;
  }

  if (is_in_progress(cp)) {
    ConnectivityChecksType::iterator pos = std::find(connectivity_checks_.begin(), connectivity_checks_.end(), cp);
    pos->cancel();
  }

  add_triggered_check(cp);
  // This can move something from failed to in progress.
  // In that case, we need to schedule.
  check_interval_ = endpoint_manager_->agent_impl->get_configuration().T_a();
  enqueue(MonotonicTimePoint::now());
}

void Checklist::succeeded(const ConnectivityCheck& cc)
{
  const CandidatePair& cp = cc.candidate_pair();

  // 7.2.5.3.3
  // 7.2.5.4

  remove_from_in_progress(cp);
  succeeded_.push_back(cp);
  succeeded_.sort(CandidatePair::priority_sorted);

  if (cp.use_candidate) {
    if (local_is_controlling_) {
      nominated_ = nominating_;
      nominating_ = valid_list_.end();
      OPENDDS_ASSERT(frozen_.empty());
      OPENDDS_ASSERT(waiting_.empty());
    }

    else {
      nominated_ = std::find(valid_list_.begin(), valid_list_.end(), cp);

      // This is the case where the use_candidate check succeeded before the normal check.
      if (nominated_ == valid_list_.end()) {
        valid_list_.push_front(cp);
        nominated_ = valid_list_.begin();
      }

      while (!frozen_.empty()) {
        CandidatePair cp = frozen_.front();
        frozen_.pop_front();
        failed_.push_back(cp);
      }

      while (!waiting_.empty()) {
        CandidatePair cp = waiting_.front();
        waiting_.pop_front();
        endpoint_manager_->agent_impl->remove(cp.foundation);
        failed_.push_back(cp);
      }

      triggered_check_queue_.clear();
    }

    nominated_is_live_ = true;
    endpoint_manager_->ice_connect(guids_, nominated_->remote.address);
    last_indication_.set_to_now();

    while (!connectivity_checks_.empty()) {
      ConnectivityCheck cc = connectivity_checks_.front();
      connectivity_checks_.pop_front();

      if (!cc.cancelled()) {
        failed(cc);
      }

      else {
        remove_from_in_progress(cc.candidate_pair());
      }
    }

    OPENDDS_ASSERT(frozen_.empty());
    OPENDDS_ASSERT(waiting_.empty());
    OPENDDS_ASSERT(triggered_check_queue_.empty());
    OPENDDS_ASSERT(in_progress_.empty());
    OPENDDS_ASSERT(connectivity_checks_.empty());
  }

  endpoint_manager_->agent_impl->unfreeze(cp.foundation);
}

void Checklist::failed(const ConnectivityCheck& cc)
{
  const CandidatePair& cp = cc.candidate_pair();
  // 7.2.5.4
  remove_from_in_progress(cp);
  failed_.push_back(cp);
  failed_.sort(CandidatePair::priority_sorted);

  if (cp.use_candidate && local_is_controlling_) {
    valid_list_.pop_front();
    nominating_ = valid_list_.end();
  }
}

void Checklist::success_response(const ACE_INET_Addr& local_address,
                                 const ACE_INET_Addr& remote_address,
                                 const STUN::Message& a_message)
{
  ConnectivityChecksType::iterator pos = std::find(connectivity_checks_.begin(), connectivity_checks_.end(), a_message.transaction_id);
  OPENDDS_ASSERT(pos != connectivity_checks_.end());

  ConnectivityCheck const cc = *pos;

  std::vector<STUN::AttributeType> unknown_attributes = a_message.unknown_comprehension_required_attributes();

  if (!unknown_attributes.empty()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING Unknown comprehension required attributes\n")));
    failed(cc);
    connectivity_checks_.erase(pos);
    endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);
    return;
  }

  if (!a_message.has_fingerprint()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING No FINGERPRINT attribute\n")));
    failed(cc);
    connectivity_checks_.erase(pos);
    endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);
    return;
  }

  ACE_INET_Addr mapped_address;

  if (!a_message.get_mapped_address(mapped_address)) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING No (XOR_)MAPPED_ADDRESS attribute\n")));
    failed(cc);
    connectivity_checks_.erase(pos);
    endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);
    return;
  }

  if (!a_message.has_message_integrity()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING No MESSAGE_INTEGRITY attribute\n")));
    failed(cc);
    connectivity_checks_.erase(pos);
    endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);
    return;
  }

  // Require integrity for checks.
  if (!a_message.verify_message_integrity(cc.request().password)) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::success_response: WARNING MESSAGE_INTEGRITY check failed\n")));
    failed(cc);
    connectivity_checks_.erase(pos);
    endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);
    return;
  }

  // At this point the check will either succeed or fail so remove from the list.
  connectivity_checks_.erase(pos);
  endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);

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
    ACE_UINT32 priority;
    // Our message, no need to check.
    cc.request().get_priority(priority);
    local = make_peer_reflexive_candidate(mapped_address, cp.local.base, cp.remote.address, priority);
    local_agent_info_.candidates.push_back(local);
    std::sort(local_agent_info_.candidates.begin(), local_agent_info_.candidates.end(), candidates_sorted);
  }

  // The valid pair
  CandidatePair vp(local, cp.remote, local_is_controlling_, true);

  add_valid_pair(vp);
}

void Checklist::error_response(const ACE_INET_Addr& /*local_address*/,
                               const ACE_INET_Addr& /*remote_address*/,
                               const STUN::Message& a_message)
{
  ConnectivityChecksType::iterator pos = std::find(connectivity_checks_.begin(), connectivity_checks_.end(), a_message.transaction_id);
  OPENDDS_ASSERT(pos != connectivity_checks_.end());

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
    connectivity_checks_.erase(pos);
    endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);
    return;
  }

  if (!a_message.has_fingerprint()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING No FINGERPRINT attribute\n")));
    failed(cc);
    connectivity_checks_.erase(pos);
    endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);
    return;
  }

  if (a_message.has_error_code()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING ")
      ACE_TEXT("STUN error response code=%d reason=%s\n"),
      a_message.get_error_code(),
      a_message.get_error_reason().c_str()));

    if (a_message.get_error_code() == STUN::UNKNOWN_ATTRIBUTE && a_message.has_unknown_attributes()) {
      std::vector<STUN::AttributeType> unknown_attributes = a_message.get_unknown_attributes();

      for (std::vector<STUN::AttributeType>::const_iterator pos = unknown_attributes.begin(),
           limit = unknown_attributes.end(); pos != limit; ++pos) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING Unknown STUN attribute %d\n"), *pos));
      }
    }

    if (a_message.get_error_code() == STUN::BAD_REQUEST && a_message.get_error_code() == STUN::UNKNOWN_ATTRIBUTE) {
      // Waiting and/or resending won't fix these errors.
      failed(cc);
      connectivity_checks_.erase(pos);
      endpoint_manager_->unset_responsible_checklist(cc.request().transaction_id, this);
    }
  }

  else {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) Checklist::error_response: WARNING STUN error response (no code)\n")));
  }
}

void Checklist::do_next_check(const MonotonicTimePoint& a_now)
{
  // Triggered checks.
  if (!triggered_check_queue_.empty()) {
    CandidatePair cp = triggered_check_queue_.front();
    triggered_check_queue_.pop_front();

    ConnectivityCheck cc(cp, local_agent_info_, remote_agent_info_, ice_tie_breaker_, a_now + endpoint_manager_->agent_impl->get_configuration().connectivity_check_ttl());

    waiting_.remove(cp);
    in_progress_.push_back(cp);
    in_progress_.sort(CandidatePair::priority_sorted);

    endpoint_manager_->send(cc.candidate_pair().remote.address, cc.request());
    connectivity_checks_.push_back(cc);
    endpoint_manager_->set_responsible_checklist(cc.request().transaction_id, this);
    check_interval_ = endpoint_manager_->agent_impl->get_configuration().T_a();
    return;
  }

  // Ordinary check.
  if (!waiting_.empty()) {
    CandidatePair cp = waiting_.front();
    waiting_.pop_front();

    ConnectivityCheck cc(cp, local_agent_info_, remote_agent_info_, ice_tie_breaker_, a_now + endpoint_manager_->agent_impl->get_configuration().connectivity_check_ttl());

    in_progress_.push_back(cp);
    in_progress_.sort(CandidatePair::priority_sorted);

    endpoint_manager_->send(cc.candidate_pair().remote.address, cc.request());
    connectivity_checks_.push_back(cc);
    endpoint_manager_->set_responsible_checklist(cc.request().transaction_id, this);
    check_interval_ = endpoint_manager_->agent_impl->get_configuration().T_a();
    return;
  }

  // Retry.
  while (!connectivity_checks_.empty()) {
    ConnectivityCheck cc = connectivity_checks_.front();
    connectivity_checks_.pop_front();

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
      cc.password(remote_agent_info_.password);
      endpoint_manager_->send(cc.candidate_pair().remote.address, cc.request());
    }

    connectivity_checks_.push_back(cc);

    // Backoff.
    check_interval_ = std::min(check_interval_ * 2, max_check_interval_);
    break;
  }

  // Waiting for the remote or frozen.
  check_interval_ = endpoint_manager_->agent_impl->get_configuration().checklist_period();
}

void Checklist::execute(const MonotonicTimePoint& a_now)
{
  if (scheduled_for_destruction_) {
    delete this;
    return;
  }

  // Nominating check.
  if (frozen_.empty() &&
      waiting_.empty() &&
      // in_progress_.empty() &&
      local_is_controlling_ &&
      !valid_list_.empty() &&
      nominating_ == valid_list_.end() &&
      nominated_ == valid_list_.end()) {
    add_triggered_check(valid_list_.front());
    nominating_ = valid_list_.begin();
  }

  bool flag = false;
  TimeDuration interval = std::max(check_interval_, endpoint_manager_->agent_impl->get_configuration().indication_period());

  if (!triggered_check_queue_.empty() ||
      !frozen_.empty() ||
      !waiting_.empty() ||
      !connectivity_checks_.empty()) {
    do_next_check(a_now);
    flag = true;
    interval = std::min(interval, check_interval_);
  }

  if (nominated_ != valid_list_.end()) {
    // Send an indication.
    STUN::Message message;
    message.class_ = STUN::INDICATION;
    message.method = STUN::BINDING;
    message.generate_transaction_id();
    message.append_attribute(STUN::make_username(remote_agent_info_.username + ":" + local_agent_info_.username));
    message.password = remote_agent_info_.password;
    message.append_attribute(STUN::make_message_integrity());
    message.append_attribute(STUN::make_fingerprint());
    endpoint_manager_->send(selected_address(), message);
    flag = true;
    interval = std::min(interval, endpoint_manager_->agent_impl->get_configuration().indication_period());

    // Check that we are receiving indications.
    const bool before = nominated_is_live_;
    nominated_is_live_ = (a_now - last_indication_) < endpoint_manager_->agent_impl->get_configuration().nominated_ttl();
    if (before && !nominated_is_live_) {
      endpoint_manager_->ice_disconnect(guids_);
    } else if (!before && nominated_is_live_) {
      endpoint_manager_->ice_connect(guids_, nominated_->remote.address);
    }
  }

  if (flag) {
    enqueue(MonotonicTimePoint::now() + interval);
  }

  // The checklist has failed.  Don't schedule.
}

void Checklist::add_guid(const GuidPair& a_guid_pair)
{
  guids_.insert(a_guid_pair);
  endpoint_manager_->set_responsible_checklist(a_guid_pair, this);
}

void Checklist::remove_guid(const GuidPair& a_guid_pair)
{
  guids_.erase(a_guid_pair);
  endpoint_manager_->unset_responsible_checklist(a_guid_pair, this);

  if (guids_.empty()) {
    // Cleanup this checklist.
    endpoint_manager_->unset_responsible_checklist(remote_agent_info_.username, this);
    reset();
    scheduled_for_destruction_ = true;

    // Flush ourselves out of the task queue.
    // Schedule for now but it may be later.
    enqueue(MonotonicTimePoint::now());
  }
}

void Checklist::add_guids(const GuidSetType& a_guids)
{
  for (GuidSetType::const_iterator pos = a_guids.begin(), limit = a_guids.end(); pos != limit; ++pos) {
    add_guid(*pos);
  }
}

void Checklist::remove_guids()
{
  GuidSetType guids = guids_;

  for (GuidSetType::const_iterator pos = guids.begin(), limit = guids.end(); pos != limit; ++pos) {
    remove_guid(*pos);
  }
}

ACE_INET_Addr Checklist::selected_address() const
{
  if (nominated_is_live_ && nominated_ != valid_list_.end()) {
    return nominated_->remote.address;
  }

  return ACE_INET_Addr();
}

void Checklist::indication()
{
  last_indication_.set_to_now();
}


} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif /* OPENDDS_SECURITY */
