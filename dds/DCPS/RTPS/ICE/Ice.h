/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY
#ifndef OPENDDS_RTPS_ICE_H
#define OPENDDS_RTPS_ICE_H

#include "dds/DCPS/Ice.h"
#include "dds/DCPS/RTPS/ICE/Stun.h"
#include "dds/DCPS/TimeTypes.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

typedef OPENDDS_VECTOR(ACE_INET_Addr) AddressListType;

bool candidates_equal(const Candidate& x, const Candidate& y);
bool candidates_sorted(const Candidate& x, const Candidate& y);

ACE_UINT32 local_priority(const ACE_INET_Addr& addr);

Candidate make_host_candidate(const ACE_INET_Addr& address);
Candidate make_server_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address);
Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address, ACE_UINT32 priority);
Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, ACE_UINT32 priority, size_t q);

struct OpenDDS_Rtps_Export GuidPair {
  DCPS::RepoId local;
  DCPS::RepoId remote;

  GuidPair(const DCPS::RepoId& a_local, const DCPS::RepoId& a_remote) : local(a_local), remote(a_remote) {}

  bool operator<(const GuidPair& a_other) const
  {
    if (DCPS::GUID_tKeyLessThan()(this->local, a_other.local)) return true;

    if (DCPS::GUID_tKeyLessThan()(a_other.local, this->local)) return false;

    if (DCPS::GUID_tKeyLessThan()(this->remote, a_other.remote)) return true;

    if (DCPS::GUID_tKeyLessThan()(a_other.remote, this->remote)) return false;

    return false;
  }
};

typedef std::set<GuidPair> GuidSetType;

class OpenDDS_Rtps_Export Endpoint {
public:
  virtual ~Endpoint() {}
  virtual AddressListType host_addresses() const = 0;
  virtual void send(const ACE_INET_Addr& address, const STUN::Message& message) = 0;
  virtual ACE_INET_Addr stun_server_address() const = 0;
  virtual void ice_connect(const GuidSetType&, const ACE_INET_Addr&) {}
  virtual void ice_disconnect(const GuidSetType&, const ACE_INET_Addr&) {}
};

class OpenDDS_Rtps_Export AgentInfoListener {
public:
  virtual ~AgentInfoListener() {}
  virtual void update_agent_info(const DCPS::RepoId& a_local_guid,
                                 const AgentInfo& a_agent_info) = 0;
  virtual void remove_agent_info(const DCPS::RepoId& a_local_guid) = 0;
};

class OpenDDS_Rtps_Export Configuration {
public:
  Configuration() :
    T_a_(0, 50000),
    connectivity_check_ttl_(5 * 60),
    checklist_period_(10),
    indication_period_(15),
    nominated_ttl_(5 * 60),
    server_reflexive_address_period_(30),
    server_reflexive_indication_count_(10),
    deferred_triggered_check_ttl_(5 * 60),
    change_password_period_(5 * 60)
  {}

  void T_a(const DCPS::TimeDuration& x)
  {
    T_a_ = x;
  }
  DCPS::TimeDuration T_a() const
  {
    return T_a_;
  }

  void connectivity_check_ttl(const DCPS::TimeDuration& x)
  {
    connectivity_check_ttl_ = x;
  }
  DCPS::TimeDuration connectivity_check_ttl() const
  {
    return connectivity_check_ttl_;
  }

  void checklist_period(const DCPS::TimeDuration& x)
  {
    checklist_period_ = x;
  }
  DCPS::TimeDuration checklist_period() const
  {
    return checklist_period_;
  }

  void indication_period(const DCPS::TimeDuration& x)
  {
    indication_period_ = x;
  }
  DCPS::TimeDuration indication_period() const
  {
    return indication_period_;
  }

  void nominated_ttl(const DCPS::TimeDuration& x)
  {
    nominated_ttl_ = x;
  }
  DCPS::TimeDuration nominated_ttl() const
  {
    return nominated_ttl_;
  }

  void server_reflexive_address_period(const DCPS::TimeDuration& x)
  {
    server_reflexive_address_period_ = x;
  }
  DCPS::TimeDuration server_reflexive_address_period() const
  {
    return server_reflexive_address_period_;
  }

  void server_reflexive_indication_count(size_t x)
  {
    server_reflexive_indication_count_ = x;
  }
  size_t server_reflexive_indication_count() const
  {
    return server_reflexive_indication_count_;
  }

  void deferred_triggered_check_ttl(const DCPS::TimeDuration& x)
  {
    deferred_triggered_check_ttl_ = x;
  }
  DCPS::TimeDuration deferred_triggered_check_ttl() const
  {
    return deferred_triggered_check_ttl_;
  }

  void change_password_period(const DCPS::TimeDuration& x)
  {
    change_password_period_ = x;
  }
  DCPS::TimeDuration change_password_period() const
  {
    return change_password_period_;
  }

  static Configuration* instance();

private:
  // Mininum time between consecutive sends.
  // RFC 8445 Section 14.2
  DCPS::TimeDuration T_a_;
  // Repeat a check for this long before failing it.
  DCPS::TimeDuration connectivity_check_ttl_;
  // Run all of the ordinary checks in a checklist in this amount of time.
  DCPS::TimeDuration checklist_period_;
  // Send an indication at this interval once an address is selected.
  DCPS::TimeDuration indication_period_;
  // The nominated pair will still be valid if an indication has been received within this amount of time.
  DCPS::TimeDuration nominated_ttl_;
  // Perform server-reflexive candidate gathering this often.
  DCPS::TimeDuration server_reflexive_address_period_;
  // Send this many binding indications to the STUN server before sending a binding request.
  size_t server_reflexive_indication_count_;
  // Lifetime of a deferred triggered check.
  DCPS::TimeDuration deferred_triggered_check_ttl_;
  // Change the password this often.
  DCPS::TimeDuration change_password_period_;
};

class OpenDDS_Rtps_Export Agent {
public:
  virtual ~Agent() {}
  virtual void add_endpoint(Endpoint* a_endpoint) = 0;
  virtual void remove_endpoint(Endpoint* a_endpoint) = 0;
  virtual AgentInfo get_local_agent_info(Endpoint* a_endpoint) const = 0;
  virtual void add_local_agent_info_listener(Endpoint* a_endpoint,
                                             const DCPS::RepoId& a_local_guid,
                                             AgentInfoListener* a_agent_info_listener) = 0;
  virtual void remove_local_agent_info_listener(Endpoint* a_endpoint,
                                                const DCPS::RepoId& a_local_guid) = 0;
  virtual void start_ice(Endpoint* a_endpoint,
                         const DCPS::RepoId& a_local_guid,
                         const DCPS::RepoId& a_remote_guid,
                         const AgentInfo& a_remote_agent_info) = 0;
  virtual void stop_ice(Endpoint* a_endpoint,
                        const DCPS::RepoId& a_local_guid,
                        const DCPS::RepoId& a_remote_guid) = 0;
  virtual ACE_INET_Addr get_address(Endpoint* a_endpoint,
                                    const DCPS::RepoId& a_local_guid,
                                    const DCPS::RepoId& a_remote_guid) const = 0;

  // Receive a STUN message.
  virtual void receive(Endpoint* a_endpoint,
                       const ACE_INET_Addr& a_local_address,
                       const ACE_INET_Addr& a_remote_address,
                       const STUN::Message& a_message) = 0;

  virtual void shutdown() = 0;

  static Agent* instance();
};

class ServerReflexiveStateMachine {
public:
  enum StateChange {
    SRSM_None,
    SRSM_Set,
    SRSM_Unset,
    SRSM_Change
  };

  ServerReflexiveStateMachine()
    : indication_count_(0)
  {}

  // Return Unset if transitioning from a determined SRA to an undetermined SRA.
  // Return None otherwise.
  StateChange send(const ACE_INET_Addr& address, size_t indication_count_limit, const DCPS::GuidPrefix_t& guid_prefix)
  {
    if (stun_server_address_ == ACE_INET_Addr() &&
        address == ACE_INET_Addr()) {
      // Do nothing.
      return SRSM_None;
    } else if (stun_server_address_ == ACE_INET_Addr() &&
               address != ACE_INET_Addr()) {
      return start(address, guid_prefix);
    } else if (stun_server_address_ != ACE_INET_Addr() &&
               address == ACE_INET_Addr()) {
      return stop();
    } else {
      if (stun_server_address_ != address) {
        const StateChange retval = stop();
        start(address, guid_prefix);
        return retval;
      } else {
        return next_send(indication_count_limit, guid_prefix);
      }
    }
  }

  // Return Set if transitioning from an undetermined SRA to a determined SRA.
  // Return Change if transitioning from a determined SRA to a different SRA.
  // Return None otherwise.
  StateChange receive(const STUN::Message& message)
  {
    switch (message.class_) {
    case STUN::SUCCESS_RESPONSE:
      return success_response(message);

    case STUN::ERROR_RESPONSE:
      return error_response(message);

    case STUN::REQUEST:
    case STUN::INDICATION:
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::receive: WARNING Unsupported STUN message class %d\n"), message.class_));
      return SRSM_None;
    }

    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::receive: WARNING Unknown STUN message class %d\n"), message.class_));
    return SRSM_None;
  }

  const STUN::Message& message() const { return message_; }
  const ACE_INET_Addr& unset_stun_server_address() const { return unset_stun_server_address_; }
  const ACE_INET_Addr& stun_server_address() const { return stun_server_address_; }

  bool is_response(const STUN::Message& message) const
  {
    return message.transaction_id == message_.transaction_id;
  }

private:
  StateChange start(const ACE_INET_Addr& address, const DCPS::GuidPrefix_t& guid_prefix)
  {
    OPENDDS_ASSERT(address != ACE_INET_Addr());
    OPENDDS_ASSERT(stun_server_address_ == ACE_INET_Addr());

    // Send a binding request.
    message_ = STUN::Message();
    message_.class_ = STUN::REQUEST;
    message_.method = STUN::BINDING;
    message_.generate_transaction_id();
    message_.append_attribute(STUN::make_guid_prefix(guid_prefix));
    message_.append_attribute(STUN::make_fingerprint());

    stun_server_address_ = address;
    server_reflexive_address_ = ACE_INET_Addr();
    indication_count_ = 0;

    return SRSM_None;
  }

  StateChange stop()
  {
    OPENDDS_ASSERT(stun_server_address_ != ACE_INET_Addr());
    const StateChange retval = server_reflexive_address_ != ACE_INET_Addr() ? SRSM_Unset : SRSM_None;
    unset_stun_server_address_ = stun_server_address_;
    stun_server_address_ = ACE_INET_Addr();
    server_reflexive_address_ = ACE_INET_Addr();
    indication_count_ = 0;
    return retval;
  }

  StateChange next_send(size_t indication_count_limit, const DCPS::GuidPrefix_t& guid_prefix)
  {
    StateChange retval = SRSM_None;

    if (message_.class_ == STUN::REQUEST &&
        server_reflexive_address_ != ACE_INET_Addr()) {
      // Two consecutive sends in a row causes a reset.
      retval = SRSM_Unset;
      server_reflexive_address_ = ACE_INET_Addr();
      unset_stun_server_address_ = stun_server_address_;
    }

    if (server_reflexive_address_ == ACE_INET_Addr() || indication_count_ >= indication_count_limit) {
      // Send a request.
      message_ = STUN::Message();
      message_.class_ = STUN::REQUEST;
      message_.method = STUN::BINDING;
      message_.generate_transaction_id();
      message_.append_attribute(STUN::make_guid_prefix(guid_prefix));
      message_.append_attribute(STUN::make_fingerprint());
      indication_count_ = 0;
    } else {
      // Send an indication.
      message_ = STUN::Message();
      message_.class_ = STUN::INDICATION;
      message_.method = STUN::BINDING;
      message_.generate_transaction_id();
      message_.append_attribute(STUN::make_guid_prefix(guid_prefix));
      message_.append_attribute(STUN::make_fingerprint());
      ++indication_count_;
    }

    return retval;
  }

  StateChange success_response(const STUN::Message& message)
  {
    std::vector<STUN::AttributeType> unknown_attributes = message.unknown_comprehension_required_attributes();

    if (!unknown_attributes.empty()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::success_response: WARNING Unknown comprehension required attributes\n")));
      return SRSM_None;
    }

    ACE_INET_Addr server_reflexive_address;

    if (!message.get_mapped_address(server_reflexive_address)) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::success_response: WARNING No (XOR)_MAPPED_ADDRESS attribute\n")));
      return SRSM_None;
    }

    if (server_reflexive_address == ACE_INET_Addr()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::success_response: WARNING (XOR)_MAPPED_ADDRESS is not valid\n")));
      return SRSM_None;
    }

    message_.class_ = STUN::INDICATION;
    if (server_reflexive_address == server_reflexive_address_) {
      return SRSM_None;
    } else {
      server_reflexive_address_ = server_reflexive_address;
      return SRSM_Change;
    }
  }

  StateChange error_response(const STUN::Message& message)
  {
    if (message.method != STUN::BINDING) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::error_response: WARNING Unsupported STUN method\n")));
      return SRSM_None;
    }


    if (!message.has_error_code()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::error_response: WARNING No error code\n")));
      return SRSM_None;
    }

    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::error_response: WARNING STUN error response code=%d reason=%s\n"), message.get_error_code(), message.get_error_reason().c_str()));

    if (message.get_error_code() == STUN::UNKNOWN_ATTRIBUTE && message.has_unknown_attributes()) {
      std::vector<STUN::AttributeType> unknown_attributes = message.get_unknown_attributes();

      for (std::vector<STUN::AttributeType>::const_iterator pos = unknown_attributes.begin(),
             limit = unknown_attributes.end(); pos != limit; ++pos) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::error_response: WARNING Unknown STUN attribute %d\n"), *pos));
      }
    }

    return SRSM_None;
  }

  STUN::Message message_;
  ACE_INET_Addr unset_stun_server_address_;
  ACE_INET_Addr stun_server_address_;
  ACE_INET_Addr server_reflexive_address_;
  size_t indication_count_;
 };

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_H */
#endif /* OPENDDS_SECURITY */
