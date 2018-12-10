/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Ice.h"

#include <iostream>
#include <sstream>

#include <openssl/rand.h>
#include <openssl/err.h>

#include "ace/Reactor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  namespace {
    template <typename T>
    std::string stringify(T x) {
      std::stringstream str;
      str << x;
      return str.str();
    }

    bool candidates_sorted(const Candidate& x, const Candidate& y) {
      if (x.address != y.address) {
        return x.address < y.address;
      }
      if (x.base != y.base) {
        return x.base < y.base;
      }
      return x.priority > y.priority;
    }

    bool candidates_equal(const Candidate& x, const Candidate& y) {
      return x.address == y.address && x.base == y.base;
    }

    Candidate make_host_candidate(const ACE_INET_Addr& address) {
      Candidate candidate;
      candidate.address = address;
      candidate.foundation += "H" + std::string(address.get_host_addr()) + "U";
      candidate.priority = (126 << 24) + (65535 << 8) + ((256 - 1) << 0); // No local preference, component 1.
      candidate.type = HOST;
      candidate.base = address;
      return candidate;
    }

    Candidate make_server_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address) {
      Candidate candidate;
      candidate.address = address;
      candidate.foundation += "S" + std::string(base.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
      candidate.priority = (100 << 24) + (65535 << 8) + ((256 - 1) << 0); // No local preference, component 1.
      candidate.type = SERVER_REFLEXIVE;
      candidate.base = base;
      return candidate;
    }

    const uint32_t PEER_REFLEXIVE_PRIORITY = (110 << 24) + (65535 << 8) + ((256 - 1) << 0);  // No local preference, component 1.

    Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address, uint32_t priority) {
      Candidate candidate;
      candidate.address = address;
      candidate.foundation += "P" + std::string(base.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
      candidate.priority = priority;
      candidate.type = PEER_REFLEXIVE;
      candidate.base = base;
      return candidate;
    }

    Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, uint32_t priority, size_t q) {
      Candidate candidate;
      candidate.address = address;
      candidate.foundation += "Q" + stringify(q) + "U";
      candidate.priority = priority;
      candidate.type = PEER_REFLEXIVE;
      return candidate;
    }

    // Candidate make_relayed_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& server_address) {
    //   Candidate candidate;
    //   candidate.address = address;
    //   candidate.foundation = "R" + std::string(address.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
    //   candidate.priority = (0 << 24) + (65535 << 8) + ((256 - 1) << 0); // No local preference, component 1.
    //   candidate.type = RELAYED;
    //   candidate.base = address;
    //   return candidate;
    // }
  }

  bool Candidate::operator==(const Candidate& other) const {
    return
      this->address == other.address &&
      this->foundation == other.foundation &&
      this->priority == other.priority &&
      this->type == other.type;
  }

  CandidatePair::CandidatePair(const Candidate& a_local, const Candidate& a_remote, bool a_local_is_controlling, bool a_use_candidate)
    : local(a_local), remote(a_remote), foundation(std::make_pair(a_local.foundation, a_remote.foundation)),
      local_is_controlling(a_local_is_controlling), priority(compute_priority()), use_candidate(a_use_candidate) {
    assert(!a_local.foundation.empty());
    assert(!a_remote.foundation.empty());
  }

  bool CandidatePair::operator==(const CandidatePair& other) const {
    return
      this->local == other.local &&
      this->remote == other.remote &&
      this->use_candidate == other.use_candidate;
  }

  ConnectivityCheck::ConnectivityCheck(Checklist* checklist, const CandidatePair& candidate_pair,
                                       const AgentInfo& local_agent_info, const AgentInfo& remote_agent_info,
                                       uint64_t ice_tie_breaker)
    : checklist_(checklist), candidate_pair_(candidate_pair), send_count_(0), cancelled_(false) {
    request_.class_ = STUN::REQUEST;
    request_.method = STUN::BINDING;
    request_.generate_transaction_id();
    request_.append_attribute(STUN::make_priority(PEER_REFLEXIVE_PRIORITY));
    if (candidate_pair.local_is_controlling) {
      request_.append_attribute(STUN::make_ice_controlling(ice_tie_breaker));
    } else {
      request_.append_attribute(STUN::make_ice_controlled(ice_tie_breaker));
    }
    if (candidate_pair.local_is_controlling && candidate_pair.use_candidate) {
      request_.append_attribute(STUN::make_use_candidate());
    }
    request_.append_attribute(STUN::make_username(remote_agent_info.username + ":" + local_agent_info.username));
    request_.password = remote_agent_info.password;
    request_.append_attribute(STUN::make_message_integrity());
    request_.append_attribute(STUN::make_fingerprint());
  }

  bool operator==(const ConnectivityCheck& cc, const STUN::TransactionId& tid) {
    return cc.request().transaction_id == tid;
  }

  bool operator==(const ConnectivityCheck& cc, Checklist* checklist) {
    return cc.checklist() == checklist;
  }

  struct Checklist {
    bool const local_is_controlling;

    Checklist(const AgentInfo& local, const AgentInfo& remote)
      : local_is_controlling(local.username < remote.username)
      , local_(local)
      , remote_(remote)
      , nominating_(valid_list_.end())
      , nominated_(valid_list_.end())
    {
      // Add the candidate pairs.
      for (AgentInfo::CandidatesType::const_iterator local_pos = local.candidates.begin(), local_limit = local.candidates.end(); local_pos != local_limit; ++local_pos) {
        for (AgentInfo::CandidatesType::const_iterator remote_pos = remote.candidates.begin(), remote_limit = remote.candidates.end(); remote_pos != remote_limit; ++remote_pos) {
          frozen_.push_back(CandidatePair(*local_pos, *remote_pos, local_is_controlling));
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
          } else {
            ++test_pos;
          }
        }
      }
    }

    size_t size() const {
      return frozen_.size() + waiting_.size() + in_progress_.size();
    }

    void compute_active_foundations(ActiveFoundationSet& active_foundations) const {
      for (CandidatePairsType::const_iterator pos = waiting_.begin(), limit = waiting_.end(); pos != limit; ++pos) {
	active_foundations.add(pos->foundation);
      }
      for (CandidatePairsType::const_iterator pos = in_progress_.begin(), limit = in_progress_.end(); pos != limit; ++pos) {
	active_foundations.add(pos->foundation);
      }
    }

    void check() const {
      for (CandidatePairsType::const_iterator pos = valid_list_.begin(), limit = valid_list_.end(); pos != limit; ++pos) {
        assert(pos->use_candidate);
      }
    }

    void unfreeze(ActiveFoundationSet& active_foundations) {
      for (CandidatePairsType::const_iterator pos = frozen_.begin(), limit = frozen_.end(); pos != limit;) {
        const CandidatePair& cp = *pos;
        if (!active_foundations.contains(cp.foundation)) {
          active_foundations.add(cp.foundation);
          waiting_.push_back(cp);
          waiting_.sort(CandidatePair::priority_sorted);
          frozen_.erase(pos++);
        } else {
          ++pos;
        }
      }
    }

    void unfreeze(ActiveFoundationSet& active_foundations, const FoundationType& foundation) {
      for (CandidatePairsType::const_iterator pos = frozen_.begin(), limit = frozen_.end(); pos != limit;) {
        const CandidatePair& cp = *pos;
        if (cp.foundation == foundation) {
          active_foundations.add(cp.foundation);
          waiting_.push_back(cp);
          waiting_.sort(CandidatePair::priority_sorted);
          frozen_.erase(pos++);
        } else {
          ++pos;
        }
      }
    }

    void fix_foundations(ActiveFoundationSet& active_foundations) {
      for (CandidatePairsType::const_iterator pos = waiting_.begin(), limit = waiting_.end(); pos != limit; ++pos) {
        active_foundations.remove(pos->foundation);
      }
      for (CandidatePairsType::const_iterator pos = in_progress_.begin(), limit = in_progress_.end(); pos != limit; ++pos) {
        active_foundations.remove(pos->foundation);
      }
    }

    bool has_ordinary_check() const { return !waiting_.empty(); }

    ConnectivityCheck get_ordinary_check(uint64_t ice_tie_breaker) {
      CandidatePair cp = waiting_.front();
      waiting_.pop_front();

      ConnectivityCheck cc(this, cp, local_, remote_, ice_tie_breaker);

      in_progress_.push_back(cp);
      in_progress_.sort(CandidatePair::priority_sorted);

      return cc;
    }

    bool has_triggered_check() const { return !triggered_check_queue_.empty(); }

    ConnectivityCheck get_triggered_check(uint64_t ice_tie_breaker) {
      CandidatePair cp = triggered_check_queue_.front();
      triggered_check_queue_.pop_front();

      ConnectivityCheck cc(this, cp, local_, remote_, ice_tie_breaker);

      waiting_.remove(cp);
      in_progress_.push_back(cp);
      in_progress_.sort(CandidatePair::priority_sorted);

      return cc;
    }

    bool add_valid_pair(const CandidatePair& valid_pair) {
      assert(valid_pair.use_candidate);
      // Has to match a candidate.
      if (std::find(frozen_.begin(), frozen_.end(), valid_pair) != frozen_.end() ||
          std::find(waiting_.begin(), waiting_.end(), valid_pair) != waiting_.end() ||
          std::find(in_progress_.begin(), in_progress_.end(), valid_pair) != in_progress_.end() ||
          std::find(succeeded_.begin(), succeeded_.end(), valid_pair) != succeeded_.end() ||
          std::find(failed_.begin(), failed_.end(), valid_pair) != failed_.end()) {
        valid_list_.push_back(valid_pair);
        valid_list_.sort(CandidatePair::priority_sorted);
        check();
        return true;
      }
      return false;
    }

    void add_random_valid_pair(const CandidatePair& valid_pair, uint32_t binding_request_priority) {
      assert(valid_pair.use_candidate);
      check();
      // Local priority is determined beforehand.
      // Compute the remote priority.
      AgentInfo::const_iterator pos = std::find(remote_.begin(), remote_.end(), valid_pair.remote);
      Candidate new_remote = valid_pair.remote;
      if (pos != remote_.end()) {
        new_remote.priority = pos->priority;
      } else {
        new_remote.priority = binding_request_priority;
      }

      CandidatePair vp(valid_pair.local, new_remote, valid_pair.local_is_controlling, true);
      assert (vp.use_candidate);
      valid_list_.push_back(vp);
      valid_list_.sort(CandidatePair::priority_sorted);
      check();
    }

    void succeeded(const CandidatePair& cp, ActiveFoundationSet& active_foundations) {
      remove_from_in_progress(cp, active_foundations);
      succeeded_.push_back(cp);
      succeeded_.sort(CandidatePair::priority_sorted);

      if (cp.use_candidate) {
        if (local_is_controlling) {
          nominated_ = nominating_;
          nominating_ = valid_list_.end();
        } else {
          nominated_ = std::find(valid_list_.begin(), valid_list_.end(), cp);
          // TODO:  This more about the case where the use_candidate check succeeded before the normal check.
          if (nominated_ == valid_list_.end()) {
            valid_list_.push_front(cp);
            nominated_ = valid_list_.begin();
          }
        }
        selected_address_ = nominated_->remote.address;
      }
      check();

      //std::cout << local_.username << " succeeded " << this->size() << " remaining for " << remote_.username << std::endl;
    }

    void failed(const CandidatePair& cp, ActiveFoundationSet& active_foundations) {
      remove_from_in_progress(cp, active_foundations);
      failed_.push_back(cp);
      failed_.sort(CandidatePair::priority_sorted);

      if (cp.use_candidate) {
        if (local_is_controlling) {
          valid_list_.pop_front();
          nominating_ = valid_list_.end();
        } else {
          std::cerr << "TODO: FAIL THIS CHECKLIST" << std::endl;
        }
      }
      check();
    }

    bool has_nominating_check() const {
      return frozen_.empty() && waiting_.empty() && in_progress_.empty() &&
        !is_failed() && local_is_controlling && nominating_ == valid_list_.end() && nominated_ == valid_list_.end();
    }

    void enqueue_nominating_check(ActiveFoundationSet& active_foundations) {
      add_triggered_check(valid_list_.front(), active_foundations);
      nominating_ = valid_list_.begin();
    }

    // 6.1.2.1
    bool is_running() const {
      return !is_completed() && !is_failed();
    }

    bool is_completed() const {
      return nominated_ != valid_list_.end();
    }

    bool is_failed() const {
      return valid_list_.empty() && frozen_.empty() && waiting_.empty() && in_progress_.empty();
    }

    const AgentInfo& local_agent_info() const { return local_; }
    const AgentInfo& remote_agent_info() const { return remote_; }

    bool get_local_candidate(const ACE_INET_Addr& address, Candidate& candidate) {
      for (AgentInfo::const_iterator pos = local_.begin(), limit = local_.end(); pos != limit; ++pos) {
        if (pos->address == address) {
          candidate = *pos;
          return true;
        }
      }
      return false;
    }

    void add_local_candidate(const Candidate& candidate) {
      local_.candidates.push_back(candidate);
      std::sort(local_.candidates.begin (), local_.candidates.end (), candidates_sorted);
    }

    bool get_remote_candidate(const ACE_INET_Addr& address, Candidate& candidate) {
      for (AgentInfo::const_iterator pos = remote_.begin(), limit = remote_.end(); pos != limit; ++pos) {
        if (pos->address == address) {
          candidate = *pos;
          return true;
        }
      }
      return false;
    }

    void add_remote_candidate(const Candidate& candidate) {
      remote_.candidates.push_back(candidate);
      std::sort(remote_.candidates.begin (), remote_.candidates.end (), candidates_sorted);
    }

    bool is_succeeded(const CandidatePair& cp) const {
      return std::find(succeeded_.begin(), succeeded_.end(), cp) != succeeded_.end();
    }

    bool is_in_progress(const CandidatePair& cp) const {
      return std::find(in_progress_.begin(), in_progress_.end(), cp) != in_progress_.end();
    }

    void add_triggered_check(const CandidatePair& cp, ActiveFoundationSet& active_foundations) {
      CandidatePairsType::const_iterator pos;

      pos = std::find(frozen_.begin(), frozen_.end(), cp);
      if (pos != frozen_.end()) {
        frozen_.erase(pos);
        active_foundations.add(cp.foundation);
        waiting_.push_back(cp);
        waiting_.sort(CandidatePair::priority_sorted);
        triggered_check_queue_.push_back(cp);
        return;
      }

      pos = std::find(waiting_.begin(), waiting_.end(), cp);
      if (pos != waiting_.end()) {
        // Done.
        return;
      }

      pos = std::find(in_progress_.begin(), in_progress_.end(), cp);
      if (pos != in_progress_.end()) {
        // Duplicating to waiting.
        active_foundations.add(cp.foundation);
        waiting_.push_back(cp);
        waiting_.sort(CandidatePair::priority_sorted);
        triggered_check_queue_.push_back(cp);
        return;
      }

      pos = std::find(succeeded_.begin(), succeeded_.end(), cp);
      if (pos != succeeded_.end()) {
        // Done.
        return;
      }

      pos = std::find(failed_.begin(), failed_.end(), cp);
      if (pos != failed_.end()) {
        failed_.erase(pos);
        active_foundations.add(cp.foundation);
        waiting_.push_back(cp);
        waiting_.sort(CandidatePair::priority_sorted);
        triggered_check_queue_.push_back(cp);
        return;
      }

      // Not in checklist.
      active_foundations.add(cp.foundation);
      waiting_.push_back(cp);
      waiting_.sort(CandidatePair::priority_sorted);
      triggered_check_queue_.push_back(cp);
    }

    void remove_from_in_progress(const CandidatePair& cp, ActiveFoundationSet& active_foundations) {
      active_foundations.remove(cp.foundation);
      // Candidates can be in progress multiple times.
      CandidatePairsType::const_iterator pos = std::find(in_progress_.begin(), in_progress_.end(), cp);
      in_progress_.erase(pos);
    }

    const GuidSetType& guids() const { return guids_; }
    GuidSetType& guids() { return guids_; }
    const ACE_INET_Addr& selected_address() const { return selected_address_; }

  private:
    GuidSetType guids_;
    AgentInfo local_;
    AgentInfo remote_;
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
    ACE_INET_Addr selected_address_;
  };

  bool operator==(Checklist* checklist, const GuidPair& guidp) {
    return checklist->guids().count(guidp) != 0;
  }

  bool operator==(Checklist* checklist, const std::string& username) {
    return checklist->remote_agent_info().username == username;
  }

  bool operator==(const ConnectivityCheck& cc, const CandidatePair& cp) {
    return cc.candidate_pair() == cp;
  }

  Agent::Agent(StunSender* stun_sender, const ACE_INET_Addr& stun_server_address, ACE_Reactor* reactor, ACE_thread_t owner)
    : stun_sender_(stun_sender)
    , stun_server_address_(stun_server_address)
    , remote_peer_reflexive_counter_(0)
    , candidate_gatherer_(*this, reactor, owner)
    , connectivity_checker_(*this, reactor, owner)
    , info_sender_(*this, reactor, owner)
  {
    //int rc =
    RAND_bytes(reinterpret_cast<unsigned char*>(&ice_tie_breaker_), sizeof(ice_tie_breaker_));
    // unsigned long err = ERR_get_error();
    // if (rc != 1) {
    //   /* RAND_bytes failed */
    //   /* `err` is valid    */
    // }

    // TODO:  Filter out addresses not allowed by the spec.
    host_addresses_ = stun_sender->host_addresses();
    regenerate_local_agent_info();
  }

  void Agent::start_ice(const GuidPair& guidp, SignalingChannel* signaling_channel) {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(mutex_);
    check_invariant();
    if (std::find(checklists_.begin(), checklists_.end(), guidp) != checklists_.end()) {
      // We have a checklist for this key.
      return;
    }

    if (unknown_guids_.count(guidp) != 0) {
      // We have already heard about this key.
      return;
    }

    unknown_guids_[guidp] = signaling_channel;
    std::cout << local_agent_info_.username << " is waiting for agent info from " << guidp.local << ':' << guidp.remote << std::endl;

    candidate_gatherer_.start();
    check_invariant();
  }

  void Agent::update_remote_agent_info(const GuidPair& guidp, const AgentInfo& remote_agent_info) {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(mutex_);
    check_invariant();
    start_ice(guidp);

    SignalingChannel* signaling_channel = unknown_guids_[guidp];
    unknown_guids_.erase(guidp);

    // Try to find by guid.
    Checklist* guid_checklist = 0;
    ChecklistSetType::const_iterator pos = std::find(checklists_.begin(), checklists_.end(), guidp);
    if (pos != checklists_.end()) {
      guid_checklist = *pos;
    }

    // Try to find by username.
    Checklist* username_checklist = 0;
    pos = std::find(checklists_.begin(), checklists_.end(), remote_agent_info.username);
    if (pos != checklists_.end()) {
      username_checklist = *pos;
    } else {
      username_checklist = add_checklist(remote_agent_info);
    }

    if (guid_checklist != username_checklist && guid_checklist != 0) {
      // Move all of the guids to the new checklist.
      for (GuidSetType::const_iterator pos = guid_checklist->guids().begin(), limit = guid_checklist->guids().end(); pos != limit; ++pos) {
        username_checklist->guids().insert(*pos);
      }
      remove_checklist(guid_checklist);
    }

    username_checklist->guids()[guidp] = signaling_channel;
    if (username_checklist->is_completed()) {
      ACE_INET_Addr addr = username_checklist->selected_address();
      selected_addresses_[guidp.remote] = addr;
    }

    if (remote_agent_info != username_checklist->remote_agent_info()) {
      Checklist* checklist = add_checklist(remote_agent_info);
      checklist->guids() = username_checklist->guids();
      remove_checklist(username_checklist);
    }

    check_invariant();
  }

  void Agent::stop_ice(const GuidPair& /*guidp*/) {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(mutex_);
    // TODO
    check_invariant();
  }

  AgentInfo Agent::get_local_agent_info() const {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(const_cast<ACE_Recursive_Thread_Mutex&>(mutex_));
    return local_agent_info_;
  }

  Checklist* Agent::add_checklist(const AgentInfo& remote_agent_info) {
    Checklist* checklist = new Checklist(local_agent_info_, remote_agent_info);
    checklists_.push_back(checklist);
    //std::cout << local_agent_info_.username << " new checklist for " << remote_agent_info.username << std::endl;
    //std::cout << local_agent_info_.username << " now has " << checklists_.size() << " checklists " << std::endl;
    // Add the deferred triggered first in case there was a nominating check.
    DeferredTriggeredChecksType::iterator pos = deferred_triggered_checks_.find(remote_agent_info.username);
    if (pos != deferred_triggered_checks_.end()) {
      const DeferredTriggeredCheckListType& list = pos->second;
      for (DeferredTriggeredCheckListType::const_iterator pos = list.begin(), limit = list.end(); pos != limit; ++pos) {
        generate_triggered_check(checklist, pos->local_address, pos->remote_address, pos->priority, pos->use_candidate);
      }
      deferred_triggered_checks_.erase(pos);
    }
    checklist->unfreeze(active_foundations_);

    connectivity_checker_.start();
    return checklist;
  }

  struct ConnectivityCheckChecklistPred {
    Checklist* checklist;
    ConnectivityCheckChecklistPred(Checklist* a_checklist) : checklist(a_checklist) {}
    bool operator() (const ConnectivityCheck& cc) const { return cc.checklist() == checklist; }
  };

  void Agent::remove_checklist(Checklist* checklist) {
    checklist->fix_foundations(active_foundations_);
    checklists_.remove(checklist);
    // std::cout << local_agent_info_.username << " remove checklist " << checklist->remote_agent_info().username << std::endl;
    // std::cout << local_agent_info_.username << " now has " << checklists_.size() << " checklists " << std::endl;
    connectivity_checks_.remove_if(ConnectivityCheckChecklistPred(checklist));
    for (GuidSetType::const_iterator pos = checklist->guids().begin(), limit = checklist->guids().end(); pos != limit; ++pos) {
      selected_addresses_.erase(pos->first.remote);
    }
    delete checklist;
  }

  void Agent::generate_triggered_check(Checklist* checklist,
                                       const ACE_INET_Addr& local_address, const ACE_INET_Addr& remote_address,
                                       uint32_t priority,
                                       bool use_candidate) {
    Candidate remote;

    if (!checklist->get_remote_candidate(remote_address, remote)) {
      // 7.3.1.3
      remote = make_peer_reflexive_candidate(remote_address, priority, ++remote_peer_reflexive_counter_);
      checklist->add_remote_candidate(remote);
    }

    // 7.3.1.4
    Candidate local;
    bool flag = checklist->get_local_candidate(local_address, local);
    assert(flag);

    CandidatePair cp(local, remote, checklist->local_is_controlling, use_candidate);

    if (checklist->is_succeeded(cp)) {
      return;
    }

    if (checklist->is_in_progress(cp)) {
      ConnectivityChecksType::iterator pos = std::find(connectivity_checks_.begin(), connectivity_checks_.end(), cp);
      pos->cancel();
    }

    checklist->add_triggered_check(cp, active_foundations_);
  }

  struct Running {
    bool operator() (Checklist* checklist) {
      return checklist->is_running();
    }
  };

  bool Agent::is_running() const {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(const_cast<ACE_Recursive_Thread_Mutex&>(mutex_));
    bool r = std::find_if(checklists_.begin(), checklists_.end(), Running()) != checklists_.end();;
    return stun_sender_ && (!unknown_guids_.empty() || r || !deferred_triggered_checks_.empty());
  }

  ACE_INET_Addr Agent::get_address(const DCPS::RepoId& guid) const {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(const_cast<ACE_Recursive_Thread_Mutex&>(mutex_));
    SelectedAddressesType::const_iterator pos = selected_addresses_.find(guid);
    if (pos != selected_addresses_.end()) {
      return pos->second;
    }

    return ACE_INET_Addr();
  }

  void Agent::receive(const ACE_INET_Addr& local_address, const ACE_INET_Addr& remote_address, const STUN::Message& message) {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(mutex_);
    check_invariant();
    switch (message.class_) {
    case STUN::REQUEST:
      request(local_address, remote_address, message);
      break;
    case STUN::INDICATION:
      // Do nothing.
      break;
    case STUN::SUCCESS_RESPONSE:
      success_response(local_address, remote_address, message);
      break;
    case STUN::ERROR_RESPONSE:
      error_response(remote_address, message);
      break;
    }
    check_invariant();
  }

  std::ostream& operator<<(std::ostream& stream, const ACE_INET_Addr& address) {
    stream << address.get_host_addr() << ':' << address.get_port_number();
    return stream;
  }

  std::ostream& operator<<(std::ostream& stream, const STUN::TransactionId& tid) {
    for (size_t idx = 0; idx != 12; ++idx) {
      stream << int(tid.data[idx]);
    }
    return stream;
  }

  void Agent::request(const ACE_INET_Addr& local_address,
                      const ACE_INET_Addr& remote_address,
                      const STUN::Message& message) {
    if (message.contains_unknown_comprehension_required_attributes()) {
      std::cerr << "TODO: Send 420 with unknown attributes" << std::endl;
      return;
    }

    if (!message.has_fingerprint()) {
      std::cerr << "TODO: Send 400 (Bad Request)" << std::endl;
      return;
    }

    if (!message.has_ice_controlled() && !message.has_ice_controlling()) {
      std::cerr << "TODO: Send 400 (Bad Request)" << std::endl;
      return;
    }

    bool use_candidate = message.has_use_candidate();
    if (use_candidate && message.has_ice_controlled()) {
      std::cerr << "TODO: Send 400 (Bad Request)" << std::endl;
      return;
    }

    uint32_t priority;
    if (!message.get_priority(priority)) {
      std::cerr << "TODO: Send 400 (Bad Request)" << std::endl;
      return;
    }

    std::string username;
    if (!message.get_username(username)) {
      std::cerr << "TODO: Send 400 (Bad Request)" << std::endl;
      return;
    }
    if (!message.has_message_integrity()) {
      std::cerr << "TODO: Send 400 (Bad Request)" << std::endl;
      return;
    }

    size_t idx = username.find(':');
    if (idx == std::string::npos) {
      std::cerr << "TODO: Send 400 (Bad Request)" << std::endl;
      return;
    }

    if (username.substr(0, idx) != local_agent_info_.username) {
      std::cerr << "TODO: Send 401 (Unauthorized)" << std::endl;
      return;
    }

    const std::string remote_username = username.substr(++idx);

    // Check the message_integrity.
    if (!message.verify_message_integrity(local_agent_info_.password)) {
      std::cerr << "TODO: Send 401 (Unauthorized)" << std::endl;
      return;
    }

    switch (message.method) {
    case STUN::BINDING:
      {
        // 7.3
        STUN::Message response;
        response.class_ = STUN::SUCCESS_RESPONSE;
        response.method = STUN::BINDING;
        memcpy(response.transaction_id.data, message.transaction_id.data, sizeof(message.transaction_id.data));
        response.append_attribute(STUN::make_mapped_address(remote_address));
        response.append_attribute(STUN::make_xor_mapped_address(remote_address));
        response.append_attribute(STUN::make_message_integrity());
        response.password = local_agent_info_.password;
        response.append_attribute(STUN::make_fingerprint());
        stun_sender_->send(remote_address, response);

        // std::cout << local_agent_info_.username << " respond to " << remote_username << ' ' << remote_address << ' ' << message.transaction_id << " use_candidate=" << use_candidate << std::endl;

        // Hack to get local port.
        const_cast<ACE_INET_Addr&>(local_address).set(host_addresses_[0].get_port_number(), local_address.get_ip_address());

        // 7.3.1.3
        ChecklistSetType::const_iterator pos = std::find(checklists_.begin(), checklists_.end(), remote_username);
        if (pos != checklists_.end()) {
          // We have a checklist.
          Checklist* checklist = *pos;
          generate_triggered_check(checklist, local_address, remote_address, priority, use_candidate);
          connectivity_checker_.start();
        } else {
          std::pair<DeferredTriggeredChecksType::iterator, bool> x = deferred_triggered_checks_.insert(std::make_pair(remote_username, DeferredTriggeredCheckListType()));
          x.first->second.push_back(DeferredTriggeredCheck(local_address, remote_address, priority, use_candidate));
        }
      }
      break;
    default:
      // Unknown method.  Stop processing.
      std::cerr << "TODO: Send error for unsupported method" << std::endl;
      break;
    }
  }

  void Agent::success_response(const ACE_INET_Addr& local_address,
                               const ACE_INET_Addr& remote_address,
                               const STUN::Message& message) {
    if (message.contains_unknown_comprehension_required_attributes()) {
      std::cerr << "TODO: Success response with unknown attributes" << std::endl;
      return;
    }

    switch (message.method) {
    case STUN::BINDING:
      {
        if (candidate_gatherer_.success_response(message)) {
          return;
        }

        // std::cout << local_agent_info_.username << " response from " << remote_address << ' ' << message.transaction_id << std::endl;

        ConnectivityChecksType::const_iterator pos = std::find(connectivity_checks_.begin(), connectivity_checks_.end(), message.transaction_id);
        if (pos == connectivity_checks_.end()) {
          // Probably a check that got cancelled.
          return;
        }

        const ConnectivityCheck cc = *pos;

        if (!message.has_fingerprint()) {
          // Let retry logic take over.
          return;
        }

        ACE_INET_Addr mapped_address;
        if (!message.get_mapped_address(mapped_address)) {
          // Let retry logic take over.
          return;
        }

        // Require integrity for checks.
        if (!message.verify_message_integrity(cc.request().password)) {
          // Let retry logic take over.
          return;
        }

        // At this point the check will either succeed or fail so remove from the list.
        connectivity_checks_.erase(pos);

        Checklist* checklist = cc.checklist();
        const CandidatePair& cp = cc.candidate_pair();

        // Hack to get local port.
        const_cast<ACE_INET_Addr&>(local_address).set(host_addresses_[0].get_port_number(), local_address.get_ip_address());

        if (remote_address != cp.remote.address || local_address != cp.local.base) {
          // 7.2.5.2.1 Non-Symmetric Transport Addresses
          failed(cc);
          return;
        }

        succeeded(cc);

        if (cp.use_candidate) {
          return;
        }

        Checklist* valid_checklist;

        // 7.2.5.3.2 Constructing a Valid Pair
        Candidate local;

        if (!checklist->get_local_candidate(mapped_address, local)) {
          // 7.2.5.3.1 Discovering Peer-Reflexive Candidates
          uint32_t priority;
          // Our message, no need to check.
          cc.request().get_priority(priority);
          local = make_peer_reflexive_candidate(mapped_address, cp.local.base, cp.remote.address, priority);
          checklist->add_local_candidate(local);
          valid_checklist = checklist;
        }

        // The valid pair
        CandidatePair vp(local, cp.remote, checklist->local_is_controlling, true);
        
        bool found = false;
        if (vp == cp) {
          checklist->add_valid_pair(vp);
          found = true;
        }

        // Check if another checklist has it.
        // Not sure why this would happen.
        for (ChecklistSetType::const_iterator pos = checklists_.begin(), limit = checklists_.end(); !found && pos != limit; ++pos) {
          Checklist* checklist = *pos;
          found = checklist->add_valid_pair(vp);
          if (found) {
            valid_checklist = checklist;
          }
        }

        if (!found) {
          // Not sure about this.  The spec is vague.
          uint32_t priority;
          cc.triggering_request().get_priority(priority);
          checklist->add_random_valid_pair(vp, priority);
          valid_checklist = checklist;
        }
      }
      break;
    default:
      // Unknown method.  Stop processing.
      std::cerr << "TODO: Send error for unsupported method" << std::endl;
      break;
    }
  }

  void Agent::error_response(const ACE_INET_Addr& /*address*/, const STUN::Message& message) {
    if (message.contains_unknown_comprehension_required_attributes()) {
      std::cerr << "TODO: Error response with unknown attributes" << std::endl;
      return;
    }

    // See section 7.2.5.2.4
    std::cerr << "TODO: Agent::error_response" << std::endl;
  }

  void Agent::regenerate_local_agent_info() {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(mutex_);

    // Populate candidates.
    local_agent_info_.candidates.clear();
    for (AddressListType::const_iterator pos = host_addresses_.begin(), limit = host_addresses_.end(); pos != limit; ++pos) {
      local_agent_info_.candidates.push_back(make_host_candidate(*pos));
      if (server_reflexive_address_ != ACE_INET_Addr()) {
        local_agent_info_.candidates.push_back(make_server_reflexive_candidate(server_reflexive_address_, *pos, stun_server_address_));
      }
    }

    // Eliminate duplicates.
    std::sort(local_agent_info_.candidates.begin (), local_agent_info_.candidates.end (), candidates_sorted);
    AgentInfo::CandidatesType::iterator last = std::unique(local_agent_info_.candidates.begin (), local_agent_info_.candidates.end (), candidates_equal);
    local_agent_info_.candidates.erase(last, local_agent_info_.candidates.end());

    // Set the type.
    local_agent_info_.type = FULL;

    // Generate username and password.
    uint32_t username = 0;
    int rc = RAND_bytes(reinterpret_cast<unsigned char*>(&username), sizeof(username));
    unsigned long err = ERR_get_error();
    if (rc != 1) {
      /* RAND_bytes failed */
      /* `err` is valid    */
    }

    local_agent_info_.username = stringify(username);

    uint64_t password[2] = { 0, 0 };
    rc = RAND_bytes(reinterpret_cast<unsigned char*>(&password[0]), sizeof(password));
    err = ERR_get_error();
    if (rc != 1) {
      /* RAND_bytes failed */
      /* `err` is valid    */
    }
    local_agent_info_.password = stringify(password[0]) + stringify(password[1]);

    // Start over.
    ChecklistSetType old_checklists = checklists_;
    for (ChecklistSetType::const_iterator pos = old_checklists.begin(), limit = old_checklists.end(); pos != limit; ++pos) {
      Checklist* old_checklist = *pos;
      const AgentInfo ai = old_checklist->remote_agent_info();
      const GuidSetType guids = old_checklist->guids();
      remove_checklist(old_checklist);
      Checklist* new_checklist = add_checklist(ai);
      new_checklist->guids() = guids;
    }

    propagate();
    info_sender_.start();
  }

  const size_t RETRY_LIMIT = 5;

  bool Agent::do_next_check() {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(mutex_);
    check_invariant();

    for (size_t n = 0; n != checklists_.size(); ++n) {
      Checklist* checklist = checklists_.front();
      checklists_.pop_front();
      checklists_.push_back(checklist);

      if (checklist->has_nominating_check()) {
        checklist->enqueue_nominating_check(active_foundations_);
        check_invariant();
      }

      if (checklist->has_triggered_check()) {
        ConnectivityCheck cc = checklist->get_triggered_check(ice_tie_breaker_);
        // std::cout << local_agent_info_.username << " triggered send to " << checklist->remote_agent_info().username << ' ' << cc.candidate_pair().remote.address << ' ' << cc.request().transaction_id << ' ' << cc.send_count() << " use_candidate=" << cc.candidate_pair().use_candidate << std::endl;
        stun_sender_->send(cc.candidate_pair().remote.address, cc.request());
        cc.increment_send_count();
        connectivity_checks_.push_back(cc);
        check_invariant();
        return true;
      }

      checklist->unfreeze(active_foundations_);

      if (checklist->has_ordinary_check()) {
        ConnectivityCheck cc = checklist->get_ordinary_check(ice_tie_breaker_);
        //std::cout << local_agent_info_.username << " ordinary send to " << checklist->remote_agent_info().username << ' ' << cc.candidate_pair().remote.address << ' ' << cc.request().transaction_id << ' ' << cc.send_count() << std::endl;
        stun_sender_->send(cc.candidate_pair().remote.address, cc.request());
        cc.increment_send_count();
        connectivity_checks_.push_back(cc);
        check_invariant();
        return true;
      }
    }

    bool flag = false;
    while (!connectivity_checks_.empty()) {
      ConnectivityCheck cc = connectivity_checks_.front();
      connectivity_checks_.pop_front();
      if (cc.send_count() < RETRY_LIMIT) {
        if (!cc.cancelled()) {
          //std::cout << local_agent_info_.username << " retry send to " << cc.checklist()->remote_agent_info().username << ' ' << cc.candidate_pair().remote.address << ' ' << cc.request().transaction_id << ' ' << cc.send_count() << std::endl;
          stun_sender_->send(cc.candidate_pair().remote.address, cc.request());
        }
        cc.increment_send_count();
        connectivity_checks_.push_back(cc);
        check_invariant();
        return true;
      } else {
        if (!cc.cancelled()) {
          //std::cout << local_agent_info_.username << " failed " << cc.checklist()->remote_agent_info().username << ' ' << cc.candidate_pair().remote.address << ' ' << cc.request().transaction_id << ' ' << cc.send_count() << std::endl;
          failed(cc);
        } else {
          cc.checklist()->remove_from_in_progress(cc.candidate_pair(), active_foundations_);
        }
        // Failing can allow nomination.
        flag = true;
      }
    }

    check_invariant();
    return flag;
  }

  void Agent::succeeded(const ConnectivityCheck& cc) {
    Checklist* checklist = cc.checklist();
    const CandidatePair& cp = cc.candidate_pair();

    // 7.2.5.3.3
    // 7.2.5.4
    checklist->succeeded(cp, active_foundations_);

    if (cp.use_candidate) {
      for (GuidSetType::const_iterator pos = checklist->guids().begin(), limit = checklist->guids().end(); pos != limit; ++pos) {
        ACE_INET_Addr addr = checklist->selected_address();
        selected_addresses_[pos->first.remote] = addr;
        std::cout << local_agent_info_.username << " nominate " << addr.get_host_addr() << ':' << addr.get_port_number() << " for " << checklist->remote_agent_info().username << ' ' << pos->first.remote << " local_is_controlling=" << checklist->local_is_controlling << std::endl;
      }
    }

    // TODO:  Do we really need to set the valid pair to succeeded?
    for (ChecklistSetType::const_iterator pos = checklists_.begin(), limit = checklists_.end(); pos != limit; ++pos) {
      (*pos)->unfreeze(active_foundations_, cp.foundation);
    }
  }

  void Agent::failed(const ConnectivityCheck& cc) {
    Checklist* checklist = cc.checklist();
    const CandidatePair& cp = cc.candidate_pair();
    // 7.2.5.4
    checklist->failed(cp, active_foundations_);
    std::cout << local_agent_info_.username << " failed " << checklist->size() << " remaining for " << checklist->remote_agent_info().username << std::endl;
    if (checklist->is_failed()) {
      std::cout << local_agent_info_.username << " failed checklist for " << checklist->remote_agent_info().username << std::endl;
    }
  }

  ACE_INET_Addr Agent::stun_server_address() const {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(const_cast<ACE_Recursive_Thread_Mutex&>(mutex_));
    return stun_server_address_;
  }

  ACE_INET_Addr Agent::server_reflexive_address() const {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(const_cast<ACE_Recursive_Thread_Mutex&>(mutex_));
    return server_reflexive_address_;
  }

  void Agent::server_reflexive_address(const ACE_INET_Addr& address) {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(mutex_);
    server_reflexive_address_ = address;
    //std::cout << local_agent_info_.username << " received server reflexive address " << server_reflexive_address_.get_host_addr() << ':' << server_reflexive_address_.get_port_number() << std::endl;
  }

  void Agent::send(const ACE_INET_Addr& address, const STUN::Message message) {
    // No lock.
    stun_sender_->send(address, message);
  }

  bool Agent::reactor_is_shut_down() const {
    return stun_sender_->reactor_is_shut_down();
  }

  void Agent::propagate() const {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(const_cast<ACE_Recursive_Thread_Mutex&>(mutex_));
    // Send to all interested signaling channels.
    for (GuidSetType::const_iterator pos2 = unknown_guids_.begin(), limit2 = unknown_guids_.end(); pos2 != limit2; ++pos2) {
      if (pos2->second) {
        pos2->second->update_agent_info(pos2->first, local_agent_info_);
      }
    }
    for (ChecklistSetType::const_iterator pos = checklists_.begin(), limit = checklists_.end(); pos != limit; ++pos) {
      for (GuidSetType::const_iterator pos2 = (*pos)->guids().begin(), limit2 = (*pos)->guids().end(); pos2 != limit2; ++pos2) {
        if (pos2->second) {
          pos2->second->update_agent_info(pos2->first, local_agent_info_);
        }
      }
    }
  }

  const ACE_Time_Value T_a(0, 50000); // Mininum time between consecutive sends.
  const ACE_Time_Value T_r(30, 0);    // Send new binding request this often.

  struct CancelTimerCommand : public DCPS::ReactorInterceptor::Command {
    ACE_Reactor* reactor;
    ACE_Event_Handler* event_handler;

    CancelTimerCommand(ACE_Reactor* a_reactor, ACE_Event_Handler* a_event_handler) :
      reactor(a_reactor), event_handler(a_event_handler) {}

    void execute() {
      reactor->cancel_timer(event_handler);
    }
  };

  struct ScheduleTimerCommand : public DCPS::ReactorInterceptor::Command {
    ACE_Reactor* reactor;
    ACE_Event_Handler* event_handler;
    ACE_Time_Value delay;

    ScheduleTimerCommand(ACE_Reactor* a_reactor, ACE_Event_Handler* a_event_handler, const ACE_Time_Value& a_delay) :
      reactor(a_reactor), event_handler(a_event_handler), delay(a_delay) {}

    void execute() {
      reactor->schedule_timer(event_handler, 0, delay);
    }
  };

  void Agent::CandidateGatherer::start() {
    if (agent_.is_running() &&
        state_ == STOPPED &&
        agent_.stun_server_address() != ACE_INET_Addr()) {
      schedule (RETRANSMITTING, gather_RTO());
      form_request();
      send_request();
    }
  }

  ACE_Time_Value Agent::CandidateGatherer::gather_RTO() {
    // 14.3
    return std::max(ACE_Time_Value(0, 500000), T_a * 1);
  }

  bool Agent::CandidateGatherer::success_response(const STUN::Message& message) {
    if (message.transaction_id != binding_request_.transaction_id) {
      return false;
    }

    CancelTimerCommand c(reactor(), this);
    execute_or_enqueue(c);
    ACE_INET_Addr server_reflexive_address;
    if (!message.get_mapped_address(server_reflexive_address)) {
      //  Go again.
      schedule(RETRANSMITTING, T_a);
    } else {
      // Success.
      schedule(MAINTAINING, T_r);
    }

    if (server_reflexive_address != agent_.server_reflexive_address()) {
      agent_.server_reflexive_address(server_reflexive_address);
      agent_.regenerate_local_agent_info();
    }

    return true;
  }

  int Agent::CandidateGatherer::handle_timeout(const ACE_Time_Value& /*now*/, const void* /*act*/) {
    switch (state_) {
    case STOPPED:
      return 0;
    case RETRANSMITTING:
      schedule(RETRANSMITTING, gather_RTO());
      send_request();
      break;
    case MAINTAINING:
      schedule(MAINTAINING, gather_RTO());
      form_request();
      send_request();
      break;
    }

    return 0;
  }

  void Agent::CandidateGatherer::form_request() {
    if (state_ != STOPPED) {
      binding_request_ = STUN::Message();
      binding_request_.class_ = STUN::REQUEST;
      binding_request_.method = STUN::BINDING;
      binding_request_.generate_transaction_id();
      // TODO:  Consider using fingerprint.
    }
  }

  void Agent::CandidateGatherer::send_request() {
    if (state_ != STOPPED) {
      agent_.send(agent_.stun_server_address(), binding_request_);
    }
  }

  void Agent::CandidateGatherer::schedule(Agent::CandidateGatherer::State next_state, const ACE_Time_Value& delay) {
    if (!agent_.is_running()) {
      next_state = STOPPED;
    }

    if (next_state != STOPPED) {
      ScheduleTimerCommand c(reactor(), this, delay);
      execute_or_enqueue(c);
    } else {
      std::cout << "CandidateGatherer stopped" << std::endl;
    }

    state_ = next_state;
  }

  bool Agent::CandidateGatherer::reactor_is_shut_down() const {
    return agent_.reactor_is_shut_down();
  }

  void Agent::ConnectivityChecker::start() {
    if (state_ != CHECKING) {
      schedule (CHECKING, ACE_Time_Value(0,0));
    }
  }

  int Agent::ConnectivityChecker::handle_timeout(const ACE_Time_Value& /*now*/, const void* /*act*/) {
    switch (state_) {
    case STOPPED:
      break;
    case CHECKING:
      if (agent_.do_next_check()) {
        schedule(CHECKING, T_a);
      } else {
        schedule(STOPPED, T_a);
      }
      break;
    }

    return 0;
  }

  void Agent::ConnectivityChecker::schedule(Agent::ConnectivityChecker::State next_state, const ACE_Time_Value& delay) {
    if (!agent_.is_running()) {
      next_state = STOPPED;
    }

    if (next_state != STOPPED) {
      ScheduleTimerCommand c(reactor(), this, delay);
      execute_or_enqueue(c);
    } else {
      std::cout << "ConnectivityChecker stopped" << std::endl;
    }

    state_ = next_state;
  }

  bool Agent::ConnectivityChecker::reactor_is_shut_down() const {
    return agent_.reactor_is_shut_down();
  }

  const ACE_Time_Value thirty(30, 0);

  void Agent::InfoSender::start() {
    count_ = 5;
    schedule (SENDING, thirty);
  }

  int Agent::InfoSender::handle_timeout(const ACE_Time_Value& /*now*/, const void* /*act*/) {
    switch (state_) {
    case STOPPED:
      break;
    case SENDING:
      agent_.propagate();
      --count_;
      schedule(SENDING, thirty);
      break;
    }

    return 0;
  }

  void Agent::InfoSender::schedule(Agent::InfoSender::State next_state, const ACE_Time_Value& delay) {
    if (count_ == 0) {
      next_state = STOPPED;
    }

    if (next_state != STOPPED) {
      ScheduleTimerCommand c(reactor(), this, delay);
      execute_or_enqueue(c);
    } else {
      std::cout << "InfoSender stopped" << std::endl;
    }

    state_ = next_state;
  }

  bool Agent::InfoSender::reactor_is_shut_down() const {
    return agent_.reactor_is_shut_down();
  }

  bool GuidPair::operator< (const GuidPair& other) const
  {
    if (this->local != other.local) {
      return DCPS::GUID_tKeyLessThan() (this->local, other.local);
    }
    return DCPS::GUID_tKeyLessThan() (this->remote, other.remote);
  }

  void Agent::check_invariant() const {
    ActiveFoundationSet expected;

    for (ChecklistSetType::const_iterator pos = checklists_.begin(), limit = checklists_.end(); pos != limit; ++pos) {
      const Checklist* checklist = *pos;
      checklist->compute_active_foundations(expected);
      assert(checklist->is_running() ^ checklist->is_completed() ^ checklist->is_failed());
      checklist->check();
    }
    assert(expected == active_foundations_);
  }

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
