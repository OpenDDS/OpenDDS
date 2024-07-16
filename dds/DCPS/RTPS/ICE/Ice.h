/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY
#ifndef OPENDDS_DCPS_RTPS_ICE_ICE_H
#define OPENDDS_DCPS_RTPS_ICE_ICE_H

#include "Stun.h"

#include <dds/DCPS/Ice.h>
#include <dds/DCPS/TimeTypes.h>
#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/GuidUtils.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

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

using DCPS::GuidPair;
typedef std::set<GuidPair> GuidSetType;

class OpenDDS_Rtps_Export Endpoint : public virtual DCPS::RcObject {
public:
  virtual ~Endpoint() {}
  virtual AddressListType host_addresses() const = 0;
  virtual void send(const ACE_INET_Addr& address, const STUN::Message& message) = 0;
  virtual ACE_INET_Addr stun_server_address() const = 0;
  virtual void ice_connect(const GuidSetType&, const ACE_INET_Addr&) {}
  virtual void ice_disconnect(const GuidSetType&, const ACE_INET_Addr&) {}
};

class OpenDDS_Rtps_Export AgentInfoListener : public virtual DCPS::RcObject {
public:
  virtual ~AgentInfoListener() {}
  virtual void update_agent_info(const DCPS::GUID_t& a_local_guid,
                                 const AgentInfo& a_agent_info) = 0;
  virtual void remove_agent_info(const DCPS::GUID_t& a_local_guid) = 0;
};

class OpenDDS_Rtps_Export Configuration {
public:
  // Mininum time between consecutive sends.
  // RFC 8445 Section 14.2
  void T_a(const DCPS::TimeDuration& x);
  DCPS::TimeDuration T_a() const;

  // Repeat a check for this long before failing it.
  void connectivity_check_ttl(const DCPS::TimeDuration& x);
  DCPS::TimeDuration connectivity_check_ttl() const;

  // Run all of the ordinary checks in a checklist in this amount of time.
  void checklist_period(const DCPS::TimeDuration& x);
  DCPS::TimeDuration checklist_period() const;

  // Send an indication at this interval once an address is selected.
  void indication_period(const DCPS::TimeDuration& x);
  DCPS::TimeDuration indication_period() const;

  // The nominated pair will still be valid if an indication has been received within this amount of time.
  void nominated_ttl(const DCPS::TimeDuration& x);
  DCPS::TimeDuration nominated_ttl() const;

  // Perform server-reflexive candidate gathering this often.
  void server_reflexive_address_period(const DCPS::TimeDuration& x);
  DCPS::TimeDuration server_reflexive_address_period() const;

  // Send this many binding indications to the STUN server before sending a binding request.
  void server_reflexive_indication_count(size_t x);
  size_t server_reflexive_indication_count() const;

  // Lifetime of a deferred triggered check.
  void deferred_triggered_check_ttl(const DCPS::TimeDuration& x);
  DCPS::TimeDuration deferred_triggered_check_ttl() const;

  // Change the password this often.
  void change_password_period(const DCPS::TimeDuration& x);
  DCPS::TimeDuration change_password_period() const;

  static Configuration* instance();
};

class OpenDDS_Rtps_Export Agent : public virtual DCPS::RcObject {
public:
  virtual ~Agent() {}
  virtual void add_endpoint(DCPS::WeakRcHandle<Endpoint> a_endpoint) = 0;
  virtual void remove_endpoint(DCPS::WeakRcHandle<Endpoint> a_endpoint) = 0;
  virtual AgentInfo get_local_agent_info(DCPS::WeakRcHandle<Endpoint> a_endpoint) const = 0;
  virtual void add_local_agent_info_listener(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                             const DCPS::GUID_t& a_local_guid,
                                             DCPS::WeakRcHandle<AgentInfoListener> a_agent_info_listener) = 0;
  virtual void remove_local_agent_info_listener(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                                const DCPS::GUID_t& a_local_guid) = 0;
  virtual void start_ice(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                         const DCPS::GUID_t& a_local_guid,
                         const DCPS::GUID_t& a_remote_guid,
                         const AgentInfo& a_remote_agent_info) = 0;
  virtual void stop_ice(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                        const DCPS::GUID_t& a_local_guid,
                        const DCPS::GUID_t& a_remote_guid) = 0;
  virtual ACE_INET_Addr get_address(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                    const DCPS::GUID_t& a_local_guid,
                                    const DCPS::GUID_t& a_remote_guid) const = 0;

  // Receive a STUN message.
  virtual void receive(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                       const ACE_INET_Addr& a_local_address,
                       const ACE_INET_Addr& a_remote_address,
                       const STUN::Message& a_message) = 0;

  virtual void shutdown() = 0;

  static DCPS::RcHandle<Agent> instance();
};

class OpenDDS_Rtps_Export ServerReflexiveStateMachine {
public:
  enum StateChange {
    SRSM_None,
    SRSM_Set,
    SRSM_Unset,
    SRSM_Change
  };

  ServerReflexiveStateMachine()
    : message_class_(STUN::REQUEST)
    , send_count_(0)
    , latency_available_(false)
  {}

  // Return Unset if transitioning from a determined SRA to an undetermined SRA.
  // Return None otherwise.
  StateChange send(const ACE_INET_Addr& address,
                   size_t indication_count_limit,
                   const DCPS::GuidPrefix_t& guid_prefix);

  // Return Set if transitioning from an undetermined SRA to a determined SRA.
  // Return Change if transitioning from a determined SRA to a different SRA.
  // Return None otherwise.
  StateChange receive(const STUN::Message& message);

  const STUN::Message& message() const { return message_; }
  const ACE_INET_Addr& unset_stun_server_address() const { return unset_stun_server_address_; }
  const ACE_INET_Addr& stun_server_address() const { return stun_server_address_; }

  bool is_response(const STUN::Message& message) const
  {
    return message.transaction_id == message_.transaction_id;
  }

  DCPS::TimeDuration latency() const
  {
    return latency_;
  }

  bool latency_available() const
  {
    return latency_available_;
  }

  void latency_available(bool flag)
  {
    latency_available_ = flag;
  }

  bool connected() const
  {
    return server_reflexive_address_ != ACE_INET_Addr();
  }

private:
  StateChange start(const ACE_INET_Addr& address, size_t indication_count_limit, const DCPS::GuidPrefix_t& guid_prefix);
  StateChange stop();
  StateChange next_send(size_t indication_count_limit, const DCPS::GuidPrefix_t& guid_prefix);
  StateChange success_response(const STUN::Message& message);
  StateChange error_response(const STUN::Message& message);

  STUN::Class message_class_;
  STUN::Message message_;
  ACE_INET_Addr unset_stun_server_address_;
  ACE_INET_Addr stun_server_address_;
  ACE_INET_Addr server_reflexive_address_;
  size_t send_count_;
  DCPS::MonotonicTimePoint timestamp_;
  DCPS::TimeDuration latency_;
  bool latency_available_;
 };

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_H */
#endif
