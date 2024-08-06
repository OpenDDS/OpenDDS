/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY

#include "Ice.h"

#include "AgentImpl.h"

#include "dds/DCPS/SafetyProfileStreams.h"
#include "dds/DCPS/debug.h"
#include <dds/DCPS/LogAddr.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

const char ICE_TA[] = "ICE_TA";
const char ICE_CONNECTIVITY_CHECK_TTL[] = "ICE_CONNECTIVITY_CHECK_TTL";
const char ICE_CHECKLIST_PERIOD[] = "ICE_CHECKLIST_PERIOD";
const char ICE_INDICATION_PERIOD[] = "ICE_INDICATION_PERIOD";
const char ICE_NOMINATED_TTL[] = "ICE_NOMINATED_TTL";
const char ICE_SERVER_REFLEXIVE_ADDRESS_PERIOD[] = "ICE_SERVER_REFLEXIVE_ADDRESS_PERIOD";
const char ICE_SERVER_REFLEXIVE_INDICATION_COUNT[] = "ICE_SERVER_REFLEXIVE_INDICATION_COUNT";
const char ICE_DEFERRED_TRIGGERED_CHECK_TTL[] = "ICE_DEFERRED_TRIGGERED_CHECK_TTL";
const char ICE_CHANGE_PASSWORD_PERIOD[] = "ICE_CHANGE_PASSWORD_PERIOD";

void Configuration::T_a(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(ICE_TA,
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

DCPS::TimeDuration Configuration::T_a() const
{
  return TheServiceParticipant->config_store()->get(ICE_TA,
                                                    DCPS::TimeDuration(0, 50000),
                                                    DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

void Configuration::connectivity_check_ttl(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(ICE_CONNECTIVITY_CHECK_TTL,
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration Configuration::connectivity_check_ttl() const
{
  return TheServiceParticipant->config_store()->get(ICE_CONNECTIVITY_CHECK_TTL,
                                                    DCPS::TimeDuration(5 * 60),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void Configuration::checklist_period(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(ICE_CHECKLIST_PERIOD,
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration Configuration::checklist_period() const
{
  return TheServiceParticipant->config_store()->get(ICE_CHECKLIST_PERIOD,
                                                    DCPS::TimeDuration(10),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void Configuration::indication_period(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(ICE_INDICATION_PERIOD,
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration Configuration::indication_period() const
{
  return TheServiceParticipant->config_store()->get(ICE_INDICATION_PERIOD,
                                                    DCPS::TimeDuration(15),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void Configuration::nominated_ttl(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(ICE_NOMINATED_TTL,
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration Configuration::nominated_ttl() const
{
  return TheServiceParticipant->config_store()->get(ICE_NOMINATED_TTL,
                                                    DCPS::TimeDuration(5 * 60),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void Configuration::server_reflexive_address_period(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(ICE_SERVER_REFLEXIVE_ADDRESS_PERIOD,
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration Configuration::server_reflexive_address_period() const
{
  return TheServiceParticipant->config_store()->get(ICE_SERVER_REFLEXIVE_ADDRESS_PERIOD,
                                                    DCPS::TimeDuration(30),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void Configuration::server_reflexive_indication_count(size_t x)
{
  TheServiceParticipant->config_store()->set_uint32(ICE_SERVER_REFLEXIVE_INDICATION_COUNT,
                                                    static_cast<DDS::UInt32>(x));
}

size_t Configuration::server_reflexive_indication_count() const
{
  return TheServiceParticipant->config_store()->get_uint32(ICE_SERVER_REFLEXIVE_INDICATION_COUNT,
                                                           10);
}

void Configuration::deferred_triggered_check_ttl(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(ICE_DEFERRED_TRIGGERED_CHECK_TTL,
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration Configuration::deferred_triggered_check_ttl() const
{
  return TheServiceParticipant->config_store()->get(ICE_DEFERRED_TRIGGERED_CHECK_TTL,
                                                    DCPS::TimeDuration(5 * 60),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void Configuration::change_password_period(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(ICE_CHANGE_PASSWORD_PERIOD,
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration Configuration::change_password_period() const
{
  return TheServiceParticipant->config_store()->get(ICE_CHANGE_PASSWORD_PERIOD,
                                                    DCPS::TimeDuration(5 * 60),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

Configuration* Configuration::instance()
{
  return ACE_Singleton<Configuration, ACE_Thread_Mutex>::instance();
}

bool candidates_sorted(const Candidate& x, const Candidate& y)
{
  if (x.address != y.address) {
    return x.address < y.address;
  }

  if (x.base != y.base) {
    return x.base < y.base;
  }

  return x.priority > y.priority;
}

bool candidates_equal(const Candidate& x, const Candidate& y)
{
  return x.address == y.address && x.base == y.base;
}

// TODO(jrw972): Implement RFC8421.

// TODO(jrw972): Implement NAT64 and DNS64 considerations.

// TODO(jrw972): For IPV6, prefer temporary addresses to permanent addresses.

// TODO(jrw972): If gathering one or more host candidates that
// correspond to an IPv6 address that was generated using a mechanism
// that prevents location tracking [RFC7721], host candidates that
// correspond to IPv6 addresses that do allow location tracking, are
// configured on the same interface, and are part of the same network
// prefix MUST NOT be gathered.  Similarly, when host candidates
// corresponding to an IPv6 address generated using a mechanism that
// prevents location tracking are gathered, then host candidates
// corresponding to IPv6 link-local addresses [RFC4291] MUST NOT be
// gathered.

ACE_UINT32 local_priority(const ACE_INET_Addr& addr)
{
  if (addr.get_type() == AF_INET6) {
    return 65535;
  }
  return 65534;
}

Candidate make_host_candidate(const ACE_INET_Addr& address)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("H") + DCPS::LogAddr::ip(address) + "U";
  // See https://tools.ietf.org/html/rfc8445#section-5.1.2.1 for an explanation of the formula below.
  candidate.priority = (126 << 24) + (local_priority(address) << 8) + ((256 - 1) << 0); // No local preference, component 1.
  candidate.type = HOST;
  candidate.base = address;
  return candidate;
}

Candidate make_server_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("S") + DCPS::LogAddr::ip(base) + "_" + DCPS::LogAddr::ip(server_address) + "U";
  // See https://tools.ietf.org/html/rfc8445#section-5.1.2.1 for an explanation of the formula below.
  candidate.priority = (100 << 24) + (local_priority(address) << 8) + ((256 - 1) << 0); // No local preference, component 1.
  candidate.type = SERVER_REFLEXIVE;
  candidate.base = base;
  return candidate;
}

Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address, ACE_UINT32 priority)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("P") + DCPS::LogAddr::ip(base) + "_" + DCPS::LogAddr::ip(server_address) + "U";
  candidate.priority = priority;
  candidate.type = PEER_REFLEXIVE;
  candidate.base = base;
  return candidate;
}

Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, ACE_UINT32 priority, size_t q)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("Q") + OpenDDS::DCPS::to_dds_string(q) + "U";
  candidate.priority = priority;
  candidate.type = PEER_REFLEXIVE;
  return candidate;
}

struct AgentHolder {
  AgentHolder()
    : agent_impl(DCPS::make_rch<AgentImpl>())
  {}

  DCPS::RcHandle<AgentImpl> agent_impl;
};

DCPS::RcHandle<Agent> Agent::instance()
{
  return DCPS::static_rchandle_cast<Agent>(ACE_Singleton<AgentHolder, ACE_Thread_Mutex>::instance()->agent_impl);
}

ServerReflexiveStateMachine::StateChange
ServerReflexiveStateMachine::send(const ACE_INET_Addr& address,
                                  size_t indication_count_limit,
                                  const DCPS::GuidPrefix_t& guid_prefix)
{
  timestamp_ = DCPS::MonotonicTimePoint::now();

  if (stun_server_address_ == ACE_INET_Addr() &&
      address == ACE_INET_Addr()) {
    // Do nothing.
    return SRSM_None;
  } else if (stun_server_address_ == ACE_INET_Addr() &&
             address != ACE_INET_Addr()) {
    return start(address, indication_count_limit, guid_prefix);
  } else if (stun_server_address_ != ACE_INET_Addr() &&
             address == ACE_INET_Addr()) {
    return stop();
  } else {
    if (stun_server_address_ != address) {
      const StateChange retval = stop();
      start(address, indication_count_limit, guid_prefix);
      return retval;
    } else {
      return next_send(indication_count_limit, guid_prefix);
    }
  }
}

ServerReflexiveStateMachine::StateChange
ServerReflexiveStateMachine::receive(const STUN::Message& message)
{
  latency_ = DCPS::MonotonicTimePoint::now() - timestamp_;
  latency_available_ = true;

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

ServerReflexiveStateMachine::StateChange
ServerReflexiveStateMachine::start(const ACE_INET_Addr& address,
                                   size_t indication_count_limit,
                                   const DCPS::GuidPrefix_t& guid_prefix)
{
  OPENDDS_ASSERT(address != ACE_INET_Addr());
  OPENDDS_ASSERT(stun_server_address_ == ACE_INET_Addr());

  // Send a binding request.
  message_class_ = STUN::REQUEST;
  send_count_ = 0;

  stun_server_address_ = address;
  server_reflexive_address_ = ACE_INET_Addr();

  return next_send(indication_count_limit, guid_prefix);
}

ServerReflexiveStateMachine::StateChange
ServerReflexiveStateMachine::stop()
{
  OPENDDS_ASSERT(stun_server_address_ != ACE_INET_Addr());
  const StateChange retval = server_reflexive_address_ != ACE_INET_Addr() ? SRSM_Unset : SRSM_None;
  unset_stun_server_address_ = stun_server_address_;
  stun_server_address_ = ACE_INET_Addr();
  server_reflexive_address_ = ACE_INET_Addr();
  send_count_ = 0;
  return retval;
}

ServerReflexiveStateMachine::StateChange
ServerReflexiveStateMachine::next_send(size_t indication_count_limit,
                                       const DCPS::GuidPrefix_t& guid_prefix)
{
  StateChange retval = SRSM_None;

  if (message_class_ == STUN::REQUEST &&
      server_reflexive_address_ != ACE_INET_Addr() &&
      send_count_ == indication_count_limit) {
    // Reset.
    retval = SRSM_Unset;
    server_reflexive_address_ = ACE_INET_Addr();
    unset_stun_server_address_ = stun_server_address_;
  }

  // indication_count_limit is offset by 1 to account for sending the request.
  if ((server_reflexive_address_ == ACE_INET_Addr()) ||
      (message_class_ == STUN::INDICATION && send_count_ >= indication_count_limit + 1)) {
    message_class_ = STUN::REQUEST;
    send_count_ = 0;
  }

  message_ = STUN::Message();
  message_.class_ = message_class_;
  message_.method = STUN::BINDING;
  message_.generate_transaction_id();
  message_.append_attribute(STUN::make_guid_prefix(guid_prefix));
  message_.append_attribute(STUN::make_fingerprint());

  ++send_count_;

  return retval;
}

ServerReflexiveStateMachine::StateChange
ServerReflexiveStateMachine::success_response(const STUN::Message& message)
{
  std::vector<STUN::AttributeType> unknown_attributes = message.unknown_comprehension_required_attributes();

  if (!unknown_attributes.empty()) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::success_response: "
                          "WARNING Unknown comprehension required attributes\n")));
    }
    return SRSM_None;
  }

  ACE_INET_Addr server_reflexive_address;

  if (!message.get_mapped_address(server_reflexive_address)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::success_response: "
                          "WARNING No (XOR)_MAPPED_ADDRESS attribute\n")));
    }
    return SRSM_None;
  }

  if (server_reflexive_address == ACE_INET_Addr()) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::success_response: "
                          "WARNING (XOR)_MAPPED_ADDRESS is not valid\n")));
    }
    return SRSM_None;
  }

  message_class_ = STUN::INDICATION;
  if (server_reflexive_address == server_reflexive_address_) {
    return SRSM_None;
  } else if (server_reflexive_address_ == ACE_INET_Addr()) {
    server_reflexive_address_ = server_reflexive_address;
    return SRSM_Set;
  } else {
    server_reflexive_address_ = server_reflexive_address;
    return SRSM_Change;
  }
}

ServerReflexiveStateMachine::StateChange
ServerReflexiveStateMachine::error_response(const STUN::Message& message)
{
  if (message.method != STUN::BINDING) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::error_response: "
                          "WARNING Unsupported STUN method\n")));
    }
    return SRSM_None;
  }


  if (!message.has_error_code()) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::error_response: "
                                      "WARNING No error code\n")));
    }
    return SRSM_None;
  }

  if (DCPS::DCPS_debug_level > 0) {
    ACE_ERROR((LM_WARNING,
               ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::error_response: "
                        "WARNING STUN error response code=%d reason=%C\n"),
               message.get_error_code(),
               message.get_error_reason().c_str()));

    if (message.get_error_code() == STUN::UNKNOWN_ATTRIBUTE && message.has_unknown_attributes()) {
      std::vector<STUN::AttributeType> unknown_attributes = message.get_unknown_attributes();

      for (std::vector<STUN::AttributeType>::const_iterator pos = unknown_attributes.begin(),
             limit = unknown_attributes.end(); pos != limit; ++pos) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) ServerReflexiveStateMachine::error_response: "
                            "WARNING Unknown STUN attribute %d\n"),
                   *pos));
      }
    }
  }

  return SRSM_None;
}

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
