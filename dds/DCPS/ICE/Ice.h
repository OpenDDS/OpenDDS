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
#include "dds/DCPS/ICE/Stun.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"

#include <cassert>
#include <sstream>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  template <typename T>
  std::string stringify(T x) {
    std::stringstream str;
    str << x;
    return str.str();
  }

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

  struct OpenDDS_Ice_Export Candidate {
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

  bool candidates_sorted(const Candidate& x, const Candidate& y);
  bool candidates_equal(const Candidate& x, const Candidate& y);

  Candidate make_host_candidate(const ACE_INET_Addr& address);
  Candidate make_server_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address);
  Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address, uint32_t priority);
  Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, uint32_t priority, size_t q);

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
      return
        this->username == other.username &&
        this->password == other.password &&
        this->type == other.type &&
        this->candidates == other.candidates;
    }
    bool operator!=(const AgentInfo& other) const { return !(*this == other); }
  };

  typedef std::vector<ACE_INET_Addr> AddressListType;

  class OpenDDS_Ice_Export Endpoint {
  public:
    virtual ~Endpoint() {}
    virtual AddressListType host_addresses() const = 0;
    virtual void send(const ACE_INET_Addr& address, const STUN::Message& message) = 0;
    virtual ACE_INET_Addr stun_server_address() const = 0;
  };

  class OpenDDS_Ice_Export AgentInfoListener {
  public:
    virtual ~AgentInfoListener() {}
    virtual void update_agent_info(DCPS::RepoId const & a_local_guid,
                                   AgentInfo const & a_agent_info) = 0;
  };

  class OpenDDS_Ice_Export Configuration {
  public:
    Configuration() :
      m_T_a(0, 50000),
      m_connectivity_check_ttl(5 * 60),
      m_checklist_period(5),
      m_indication_period(15),
      m_nominated_ttl(5 * 60),
      m_server_reflexive_address_period(30),
      m_server_reflexive_indication_count(10),
      m_deferred_triggered_check_ttl(5 * 60),
      m_change_password_period(5 * 60)
    {}

    void T_a(ACE_Time_Value const & x) { m_T_a = x; }
    ACE_Time_Value T_a() const { return m_T_a; }

    void connectivity_check_ttl(ACE_Time_Value const & x) { m_connectivity_check_ttl = x; }
    ACE_Time_Value connectivity_check_ttl() const { return m_connectivity_check_ttl; }

    void checklist_period(ACE_Time_Value const & x) { m_checklist_period = x; }
    ACE_Time_Value checklist_period() const { return m_checklist_period; }

    void indication_period(ACE_Time_Value const & x) { m_indication_period = x; }
    ACE_Time_Value indication_period() const { return m_indication_period; }

    void nominated_ttl(ACE_Time_Value const & x) { m_nominated_ttl = x; }
    ACE_Time_Value nominated_ttl() const { return m_nominated_ttl; }

    void server_reflexive_address_period(ACE_Time_Value const & x) { m_server_reflexive_address_period = x; }
    ACE_Time_Value server_reflexive_address_period() const { return m_server_reflexive_address_period; }

    void server_reflexive_indication_count(size_t x) { m_server_reflexive_indication_count = x; }
    size_t server_reflexive_indication_count() const { return m_server_reflexive_indication_count; }

    void deferred_triggered_check_ttl(ACE_Time_Value const & x) { m_deferred_triggered_check_ttl = x; }
    ACE_Time_Value deferred_triggered_check_ttl() const { return m_deferred_triggered_check_ttl; }

    void change_password_period(ACE_Time_Value const & x) { m_change_password_period = x; }
    ACE_Time_Value change_password_period() const { return m_change_password_period; }

  private:
    // Mininum time between consecutive sends.
    // RFC 8445 Section 14.2
    ACE_Time_Value m_T_a;
    // Repeat a check for this long before failing it.
    ACE_Time_Value m_connectivity_check_ttl;
    // Run all of the ordinary checks in a checklist in this amount of time.
    ACE_Time_Value m_checklist_period;
    // Send an indication at this interval once an address is selected.
    ACE_Time_Value m_indication_period;
    // The nominated pair will still be valid if an indication has been received within this amount of time.
    ACE_Time_Value m_nominated_ttl;
    // Perform server-reflexive candidate gathering this often.
    ACE_Time_Value m_server_reflexive_address_period;
    // Send this many binding indications to the STUN server before sending a binding request.
    size_t m_server_reflexive_indication_count;
    // Lifetime of a deferred triggered check.
    ACE_Time_Value m_deferred_triggered_check_ttl;
    // Change the password this often.
    ACE_Time_Value m_change_password_period;
  };
  
  class OpenDDS_Ice_Export Agent {
  public:
    virtual ~Agent() {}
    virtual Configuration & get_configuration() = 0;
    virtual void add_endpoint(Endpoint * a_endpoint) = 0;
    virtual void remove_endpoint(Endpoint * a_endpoint) = 0;
    virtual AgentInfo get_local_agent_info(Endpoint * a_endpoint) const = 0;
    virtual void add_local_agent_info_listener(Endpoint * a_endpoint,
                                               DCPS::RepoId const & a_local_guid,
                                               AgentInfoListener * a_agent_info_listener) = 0;
    virtual void remove_local_agent_info_listener(Endpoint * a_endpoint,
                                                  DCPS::RepoId const & a_local_guid) = 0;
    virtual void start_ice(Endpoint * a_endpoint,
                           DCPS::RepoId const & a_local_guid,
                           DCPS::RepoId const & a_remote_guid,
                           AgentInfo const & a_remote_agent_info) = 0;
    virtual void stop_ice(Endpoint * a_endpoint,
                          DCPS::RepoId const & a_local_guid,
                          DCPS::RepoId const & a_remote_guid) = 0;
    virtual ACE_INET_Addr get_address(Endpoint * a_endpoint,
                                      DCPS::RepoId const & a_local_guid,
                                      DCPS::RepoId const & a_remote_guid) const = 0;

    // Receive a STUN message.
    virtual void receive(Endpoint * a_endpoint,
                         ACE_INET_Addr const & a_local_address,
                         ACE_INET_Addr const & a_remote_address,
                         STUN::Message const & a_message) = 0;

    static Agent* instance();
  };

  std::ostream& operator<<(std::ostream& stream, const ACE_INET_Addr& address);
  std::ostream& operator<<(std::ostream& stream, const STUN::TransactionId& tid);
  std::ostream& operator<<(std::ostream& stream, const ICE::Candidate& candidate);
  std::ostream& operator<<(std::ostream& stream, const ICE::AgentInfo& agent_info);

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_H */
