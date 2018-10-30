/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EndpointManager.h"

#include <ace/Reverse_Lock_T.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include "Checklist.h"

#include <iostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  std::string stringify(unsigned char x[16]) {
    char retval[32];
    for (size_t idx = 0; idx != 16; ++idx) {
      retval[2 * idx] = (x[idx] & 0x0F) + 97;
      retval[2 * idx + 1] = ((x[idx] & 0xF0) >> 4) + 97;
    }
    return std::string(retval, 32);
  }

  EndpointManager::EndpointManager(AgentImpl * a_agent_impl, Endpoint * a_endpoint) :
    agent_impl(a_agent_impl),
    endpoint(a_endpoint),
    m_scheduled_for_destruction(false),
    m_requesting(true),
    m_send_count(0),
    m_server_reflexive_task(this),
    m_change_password_task(this) {

    // Set the type.
    m_agent_info.type = FULL;

    int rc =  RAND_bytes(reinterpret_cast<unsigned char*>(&m_ice_tie_breaker), sizeof(m_ice_tie_breaker));
    if (rc != 1) {
      unsigned long err = ERR_get_error();
      char msg[256] = { 0 };
      ERR_error_string_n(err, msg, sizeof(msg));

      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) EndpointManager::EndpointManager: ERROR '%C' returned by RAND_bytes(...)\n"),
                 msg));
    }

    change_username();
    set_host_addresses(endpoint->host_addresses());
  }

  void EndpointManager::start_ice(DCPS::RepoId const & a_local_guid,
                                  DCPS::RepoId const & a_remote_guid,
                                  AgentInfo const & a_remote_agent_info) {
    // Check for username collision.
    if (a_remote_agent_info.username == m_agent_info.username) {
      change_username();
    }

    GuidPair guidp(a_local_guid, a_remote_guid);

    // Try to find by guid.
    Checklist * guid_checklist = 0;
    {
      GuidPairToChecklistType::const_iterator pos = m_guid_pair_to_checklist.find(guidp);
      if (pos != m_guid_pair_to_checklist.end()) {
        guid_checklist = pos->second;
      }
    }

    // Try to find by username.
    Checklist * username_checklist = 0;
    {
      UsernameToChecklistType::const_iterator pos = m_username_to_checklist.find(a_remote_agent_info.username);
      if (pos != m_username_to_checklist.end()) {
        username_checklist = pos->second;
      } else {
        std::cout << "New checklist for " << a_remote_agent_info.username << std::endl;
        username_checklist = create_checklist(a_remote_agent_info);
      }
    }

    if (guid_checklist != username_checklist) {
      if (guid_checklist != 0) {
        guid_checklist->remove_guid(guidp);
      }
      username_checklist->add_guid(guidp);
    }

    AgentInfo old_remote_agent_info = username_checklist->original_remote_agent_info();
    if (old_remote_agent_info == a_remote_agent_info) {
      // No change.
      return;
    }

    old_remote_agent_info.password = a_remote_agent_info.password;
    if (old_remote_agent_info == a_remote_agent_info) {
      // Password change.
      username_checklist->set_remote_password(a_remote_agent_info.password);
      return;
    }

    // The remote agent changed its info without changing its username.
    GuidSetType const guids = username_checklist->guids();
    username_checklist->remove_guids();
    std::cout << "Modified checklist for " << a_remote_agent_info.username << std::endl;
    username_checklist = create_checklist(a_remote_agent_info);
    username_checklist->add_guids(guids);
  }

  void EndpointManager::stop_ice(DCPS::RepoId const & a_local_guid,
                                 DCPS::RepoId const & a_remote_guid) {
    GuidPair guidp(a_local_guid, a_remote_guid);

    GuidPairToChecklistType::const_iterator pos = m_guid_pair_to_checklist.find(guidp);
    if (pos != m_guid_pair_to_checklist.end()) {
      Checklist * guid_checklist = pos->second;
      guid_checklist->remove_guid(guidp);
    }
  }

  ACE_INET_Addr EndpointManager::get_address(DCPS::RepoId const & a_local_guid,
                                             DCPS::RepoId const & a_remote_guid) const {
    GuidPair guidp(a_local_guid, a_remote_guid);
    GuidPairToChecklistType::const_iterator pos = m_guid_pair_to_checklist.find(guidp);
    if (pos != m_guid_pair_to_checklist.end()) {
      return pos->second->selected_address();
    }

    return ACE_INET_Addr();
  }


  void EndpointManager::receive(ACE_INET_Addr const & a_local_address,
                                ACE_INET_Addr const & a_remote_address,
                                STUN::Message const & a_message) {
    switch (a_message.class_) {
    case STUN::REQUEST:
      request(a_local_address, a_remote_address, a_message);
      return;
    case STUN::INDICATION:
      indication(a_message);
      return;
    case STUN::SUCCESS_RESPONSE:
      success_response(a_local_address, a_remote_address, a_message);
      return;
    case STUN::ERROR_RESPONSE:
      error_response(a_local_address, a_remote_address, a_message);
      return;
    }

    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::receive: WARNING Unknown ICE message class %d\n"), a_message.class_));
  }

  void EndpointManager::change_username() {
    // Generate the username.
    unsigned char username[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int rc = RAND_bytes(username, sizeof(username));
    if (rc != 1) {
      unsigned long err = ERR_get_error();
      char msg[256] = { 0 };
      ERR_error_string_n(err, msg, sizeof(msg));

      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) EndpointManager::EndpointManager: ERROR '%C' returned by RAND_bytes(...)\n"),
                 msg));
    }
    m_agent_info.username = stringify(username);
    change_password(false);
  }

  void EndpointManager::change_password(bool password_only) {
    std::cout << "Changing password" << std::endl;
    unsigned char password[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int rc = RAND_bytes(password, sizeof(password));
    if (rc != 1) {
      unsigned long err = ERR_get_error();
      char msg[256] = { 0 };
      ERR_error_string_n(err, msg, sizeof(msg));

      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) EndpointManager::change_password: ERROR '%C' returned by RAND_bytes(...)\n"),
                 msg));
    }
    m_agent_info.password = stringify(password);
    regenerate_agent_info(password_only);
  }

  void EndpointManager::set_host_addresses(AddressListType const & a_host_addresses) {
    // Section IETF RFC 8445 5.1.1.1
    // TODO(jrw972):  Handle IPv6.
    AddressListType host_addresses;
    for (AddressListType::const_iterator pos = a_host_addresses.begin(), limit = a_host_addresses.end();
         pos != limit; ++pos) {
      if (pos->is_loopback()) {
        continue;
      }
      host_addresses.push_back(*pos);
    }

    if (m_host_addresses != host_addresses) {
      m_host_addresses = host_addresses;
      regenerate_agent_info(false);
    }
  }

  void EndpointManager::set_server_reflexive_address(ACE_INET_Addr const & a_server_reflexive_address,
                                                     ACE_INET_Addr const & a_stun_server_address) {
    if (m_server_reflexive_address != a_server_reflexive_address ||
        m_stun_server_address != a_stun_server_address) {
      m_server_reflexive_address = a_server_reflexive_address;
      m_stun_server_address = a_stun_server_address;
      regenerate_agent_info(false);
    }
  }

  void EndpointManager::regenerate_agent_info(bool password_only) {
    if (!password_only) {
      // Populate candidates.
      m_agent_info.candidates.clear();
      for (AddressListType::const_iterator pos = m_host_addresses.begin(), limit = m_host_addresses.end(); pos != limit; ++pos) {
        m_agent_info.candidates.push_back(make_host_candidate(*pos));
        if (m_server_reflexive_address != ACE_INET_Addr() &&
            m_stun_server_address != ACE_INET_Addr()) {
          m_agent_info.candidates.push_back(make_server_reflexive_candidate(m_server_reflexive_address, *pos, m_stun_server_address));
        }
      }

      // Eliminate duplicates.
      std::sort(m_agent_info.candidates.begin (), m_agent_info.candidates.end (), candidates_sorted);
      AgentInfo::CandidatesType::iterator last = std::unique(m_agent_info.candidates.begin (), m_agent_info.candidates.end (), candidates_equal);
      m_agent_info.candidates.erase(last, m_agent_info.candidates.end());

      // Start over.
      UsernameToChecklistType old_checklists = m_username_to_checklist;
      for (UsernameToChecklistType::const_iterator pos = old_checklists.begin(),
             limit = old_checklists.end();
           pos != limit; ++pos) {
        Checklist * old_checklist = pos->second;
        AgentInfo const remote_agent_info = old_checklist->original_remote_agent_info();
        GuidSetType const guids = old_checklist->guids();
        old_checklist->remove_guids();
        std::cout << "Starting over for " << remote_agent_info.username << std::endl;
        Checklist * new_checklist = create_checklist(remote_agent_info);
        new_checklist->add_guids(guids);
      }
    }

    {
      // Propagate changed agent info.

      // Make a copy.
      AgentInfoListenersType agent_info_listeners = m_agent_info_listeners;
      AgentInfo agent_info = m_agent_info;

      // Release the lock.
      ACE_Reverse_Lock<ACE_Recursive_Thread_Mutex> rev_lock(agent_impl->mutex);
      ACE_GUARD(ACE_Reverse_Lock<ACE_Recursive_Thread_Mutex>, rev_guard, rev_lock);

      for (AgentInfoListenersType::const_iterator pos = agent_info_listeners.begin(),
             limit =  agent_info_listeners.end(); pos != limit; ++pos) {
        pos->second->update_agent_info(pos->first, agent_info);
      }
    }
  }

  void EndpointManager::server_reflexive_task(ACE_Time_Value const & a_now) {
    // Request and maintain a server-reflexive address.
    m_next_stun_server_address = endpoint->stun_server_address();
    if (m_next_stun_server_address != ACE_INET_Addr()) {
      m_binding_request = STUN::Message();
      m_binding_request.class_ = m_requesting ? STUN::REQUEST : STUN::INDICATION;
      m_binding_request.method = STUN::BINDING;
      m_binding_request.generate_transaction_id();
      m_binding_request.append_attribute(STUN::make_fingerprint());

      endpoint->send(m_next_stun_server_address, m_binding_request);
      if (!m_requesting && m_send_count == agent_impl->get_configuration().server_reflexive_indication_count() - 1) {
        m_requesting = true;
      }
      m_send_count = (m_send_count + 1) % agent_impl->get_configuration().server_reflexive_indication_count();
    } else {
      m_requesting = true;
      m_send_count = 0;
    }

    // Remove expired deferred checks.
    for (DeferredTriggeredChecksType::iterator pos = m_deferred_triggered_checks.begin(),
           limit = m_deferred_triggered_checks.end(); pos != limit;) {
      DeferredTriggeredCheckListType & list = pos->second;
      while (!list.empty() && list.front().expiration_date < a_now) {
        list.pop_front();
      }
      if (list.empty()) {
        m_deferred_triggered_checks.erase(pos++);
      } else {
        pos++;
      }
    }

    // Repopulate the host addresses.
    set_host_addresses(endpoint->host_addresses());
  }

  bool EndpointManager::success_response(STUN::Message const & a_message) {
    if (a_message.transaction_id != m_binding_request.transaction_id) {
      return false;
    }

    std::vector<STUN::AttributeType> unknown_attributes = a_message.unknown_comprehension_required_attributes();
    if (!unknown_attributes.empty()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::success_response: WARNING Unknown comprehension required attributes\n")));
      return true;
    }

    ACE_INET_Addr server_reflexive_address;
    if (a_message.get_mapped_address(server_reflexive_address)) {
      set_server_reflexive_address(server_reflexive_address, m_next_stun_server_address);
      m_requesting = false;
      m_send_count = 0;
    } else {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::success_response: WARNING No (XOR)_MAPPED_ADDRESS attribute\n")));
      set_server_reflexive_address(ACE_INET_Addr(), ACE_INET_Addr());
      m_requesting = true;
      m_send_count = 0;
    }
    return true;
  }

  bool EndpointManager::error_response(STUN::Message const & a_message) {
    if (a_message.transaction_id != m_binding_request.transaction_id) {
      return false;
    }

    if (a_message.has_error_code()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::error_response: WARNING STUN error response code=%d reason=%s\n"), a_message.get_error_code(), a_message.get_error_reason().c_str()));
      if (a_message.get_error_code() == 420 && a_message.has_unknown_attributes()) {
        std::vector<STUN::AttributeType> unknown_attributes = a_message.get_unknown_attributes();
        for (std::vector<STUN::AttributeType>::const_iterator pos = unknown_attributes.begin(),
               limit = unknown_attributes.end(); pos != limit; ++pos) {
          ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::error_response: WARNING Unknown STUN attribute %d\n"), *pos));
        }
      }
    } else {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::error_response: WARNING STUN error response (no code)\n")));
    }

    return true;
  }

  Checklist * EndpointManager::create_checklist(AgentInfo const & remote_agent_info) {
    Checklist* checklist = new Checklist(this, m_agent_info, remote_agent_info, m_ice_tie_breaker);
    // Add the deferred triggered first in case there was a nominating check.
    DeferredTriggeredChecksType::iterator pos = m_deferred_triggered_checks.find(remote_agent_info.username);
    if (pos != m_deferred_triggered_checks.end()) {
      const DeferredTriggeredCheckListType& list = pos->second;
      for (DeferredTriggeredCheckListType::const_iterator pos = list.begin(), limit = list.end(); pos != limit; ++pos) {
        checklist->generate_triggered_check(pos->local_address, pos->remote_address, pos->priority, pos->use_candidate);
      }
      m_deferred_triggered_checks.erase(pos);
    }
    checklist->unfreeze();

    return checklist;
  }

  STUN::Message EndpointManager::make_unknown_attributes_error_response(STUN::Message const & a_message,
                                                                        std::vector<STUN::AttributeType> const & a_unknown_attributes) {
    STUN::Message response;
    response.class_ = STUN::ERROR_RESPONSE;
    response.method = a_message.method;
    memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
    response.append_attribute(STUN::make_error_code(420, "Unknown Attributes"));
    response.append_attribute(STUN::make_unknown_attributes(a_unknown_attributes));
    response.append_attribute(STUN::make_message_integrity());
    response.password = m_agent_info.password;
    response.append_attribute(STUN::make_fingerprint());
    return response;
  }

  STUN::Message EndpointManager::make_bad_request_error_response(STUN::Message const & a_message,
                                                                 std::string const & a_reason) {
    STUN::Message response;
    response.class_ = STUN::ERROR_RESPONSE;
    response.method = a_message.method;
    memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
    response.append_attribute(STUN::make_error_code(400, a_reason));
    response.append_attribute(STUN::make_message_integrity());
    response.password = m_agent_info.password;
    response.append_attribute(STUN::make_fingerprint());
    return response;
  }

  STUN::Message EndpointManager::make_unauthorized_error_response(STUN::Message const & a_message) {
    STUN::Message response;
    response.class_ = STUN::ERROR_RESPONSE;
    response.method = a_message.method;
    memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
    response.append_attribute(STUN::make_error_code(401, "Unauthorized"));
    response.append_attribute(STUN::make_message_integrity());
    response.password = m_agent_info.password;
    response.append_attribute(STUN::make_fingerprint());
    return response;
  }

  // STUN Message processing.
  void EndpointManager::request(ACE_INET_Addr const & a_local_address,
                                ACE_INET_Addr const & a_remote_address,
                                STUN::Message const & a_message) {
    std::string username;
    if (!a_message.get_username(username)) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No USERNAME attribute\n")));
      endpoint->send(a_remote_address,
                     make_bad_request_error_response(a_message, "Bad Request: USERNAME must be present"));
      return;
    }
    if (!a_message.has_message_integrity()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No MESSAGE_INTEGRITY attribute\n")));
      endpoint->send(a_remote_address,
                     make_bad_request_error_response(a_message, "Bad Request: MESSAGE_INTEGRITY must be present"));
      return;
    }

    size_t idx = username.find(':');
    if (idx == std::string::npos) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING USERNAME does not contain a colon\n")));
      endpoint->send(a_remote_address,
                     make_bad_request_error_response(a_message, "Bad Request: USERNAME must be colon-separated"));
      return;
    }

    if (username.substr(0, idx) != m_agent_info.username) {
      // We expect this to happen.
      endpoint->send(a_remote_address,
                     make_unauthorized_error_response(a_message));
      return;
    }

    const std::string remote_username = username.substr(++idx);

    // Check the message_integrity.
    if (!a_message.verify_message_integrity(m_agent_info.password)) {
      // We expect this to happen.
      endpoint->send(a_remote_address,
                     make_unauthorized_error_response(a_message));
      return;
    }

    std::vector<STUN::AttributeType> unknown_attributes = a_message.unknown_comprehension_required_attributes();
    if (!unknown_attributes.empty()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING Unknown comprehension required attributes\n")));
      endpoint->send(a_remote_address,
                     make_unknown_attributes_error_response(a_message, unknown_attributes));
      return;
    }

    if (!a_message.has_fingerprint()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No FINGERPRINT attribute\n")));
      endpoint->send(a_remote_address,
                     make_bad_request_error_response(a_message, "Bad Request: FINGERPRINT must be present"));
      return;
    }

    if (!a_message.has_ice_controlled() && !a_message.has_ice_controlling()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No ICE_CONTROLLED/ICE_CONTROLLING attribute\n")));
      endpoint->send(a_remote_address,
                     make_bad_request_error_response(a_message, "Bad Request: Either ICE_CONTROLLED or ICE_CONTROLLING must be present"));
      return;
    }

    bool use_candidate = a_message.has_use_candidate();
    if (use_candidate && a_message.has_ice_controlled()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING USE_CANDIDATE without ICE_CONTROLLED\n")));
      endpoint->send(a_remote_address,
                     make_bad_request_error_response(a_message, "Bad Request: USE_CANDIDATE can only be present when ICE_CONTROLLED is present"));
      return;
    }

    uint32_t priority;
    if (!a_message.get_priority(priority)) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No PRIORITY attribute\n")));
      endpoint->send(a_remote_address,
                     make_bad_request_error_response(a_message, "Bad Request: PRIORITY must be present"));
      return;
    }

    switch (a_message.method) {
    case STUN::BINDING:
      {
        // 7.3
        STUN::Message response;
        response.class_ = STUN::SUCCESS_RESPONSE;
        response.method = STUN::BINDING;
        memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
        response.append_attribute(STUN::make_mapped_address(a_remote_address));
        response.append_attribute(STUN::make_xor_mapped_address(a_remote_address));
        response.append_attribute(STUN::make_message_integrity());
        response.password = m_agent_info.password;
        response.append_attribute(STUN::make_fingerprint());
        endpoint->send(a_remote_address, response);

        // 7.3.1.3
        UsernameToChecklistType::const_iterator pos = m_username_to_checklist.find(remote_username);
        if (pos != m_username_to_checklist.end()) {
          // We have a checklist.
          Checklist* checklist = pos->second;
          checklist->generate_triggered_check(a_local_address, a_remote_address, priority, use_candidate);
        } else {
          std::pair<DeferredTriggeredChecksType::iterator, bool> x = m_deferred_triggered_checks.insert(std::make_pair(remote_username, DeferredTriggeredCheckListType()));
          x.first->second.push_back(DeferredTriggeredCheck(a_local_address, a_remote_address, priority, use_candidate, ACE_Time_Value().now() + agent_impl->get_configuration().deferred_triggered_check_ttl()));
        }
      }
      break;
    default:
      // Unknown method.  Stop processing.
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING Unknown ICE method\n")));
      endpoint->send(a_remote_address,
                     make_bad_request_error_response(a_message, "Bad Request: Unknown method"));
      break;
    }
  }

  void EndpointManager::indication(STUN::Message const & a_message) {
    std::string username;
    if (!a_message.get_username(username)) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::indication: WARNING No USERNAME attribute\n")));
      return;
    }
    if (!a_message.has_message_integrity()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::indication: WARNING No MESSAGE_INTEGRITY attribute\n")));
      return;
    }

    size_t idx = username.find(':');
    if (idx == std::string::npos) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::indication: WARNING USERNAME does not contain a colon\n")));
      return;
    }

    if (username.substr(0, idx) != m_agent_info.username) {
      // We expect this to happen.
      return;
    }

    const std::string remote_username = username.substr(++idx);

    // Check the message_integrity.
    if (!a_message.verify_message_integrity(m_agent_info.password)) {
      // We expect this to happen.
      return;
    }

    std::vector<STUN::AttributeType> unknown_attributes = a_message.unknown_comprehension_required_attributes();
    if (!unknown_attributes.empty()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::indication: WARNING Unknown comprehension required attributes\n")));
      return;
    }

    if (!a_message.has_fingerprint()) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::indication: WARNING No FINGERPRINT attribute\n")));
      return;
    }

    switch (a_message.method) {
    case STUN::BINDING:
      {
        // Section 11
        UsernameToChecklistType::const_iterator pos = m_username_to_checklist.find(remote_username);
        if (pos != m_username_to_checklist.end()) {
          // We have a checklist.
          pos->second->indication();
        }
      }
      break;
    default:
      // Unknown method.  Stop processing.
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::indication: WARNING Unknown ICE method\n")));
      break;
    }
  }

  void EndpointManager::success_response(ACE_INET_Addr const & a_local_address,
                                         ACE_INET_Addr const & a_remote_address,
                                         STUN::Message const & a_message) {
    switch (a_message.method) {
    case STUN::BINDING:
      {
        if (success_response(a_message)) {
          return;
        }

        TransactionIdToChecklistType::const_iterator pos = m_transaction_id_to_checklist.find(a_message.transaction_id);
        if (pos == m_transaction_id_to_checklist.end()) {
          // Probably a check that got cancelled.
          return;
        }

        // Checklist is responsible for updating the map.
        // Checklist also checks for required parameters.
        pos->second->success_response(a_local_address, a_remote_address, a_message);
      }
      break;
    default:
      // Unknown method.  Stop processing.
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::success_response: WARNING Unknown ICE method\n")));
      break;
    }
  }

  void EndpointManager::error_response(ACE_INET_Addr const & a_local_address,
                                       ACE_INET_Addr const & a_remote_address,
                                       STUN::Message const & a_message) {
    switch (a_message.method) {
    case STUN::BINDING:
      {
        if (error_response(a_message)) {
          return;
        }

        TransactionIdToChecklistType::const_iterator pos = m_transaction_id_to_checklist.find(a_message.transaction_id);
        if (pos == m_transaction_id_to_checklist.end()) {
          // Probably a check that got cancelled.
          return;
        }

        // Checklist is responsible for updating the map.
        // Checklist also checks for required parameters.
        pos->second->error_response(a_local_address, a_remote_address, a_message);
      }
      break;
    default:
      // Unknown method.  Stop processing.
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::error_response: WARNING Unknown ICE method\n")));
      break;
    }
  }

  void EndpointManager::compute_active_foundations(ActiveFoundationSet & a_active_foundations) const {
    for (UsernameToChecklistType::const_iterator pos = m_username_to_checklist.begin(),
           limit = m_username_to_checklist.end(); pos != limit; ++pos) {
      const Checklist* checklist = pos->second;
      checklist->compute_active_foundations(a_active_foundations);
    }
  }

  void EndpointManager::check_invariants() const {
    for (UsernameToChecklistType::const_iterator pos = m_username_to_checklist.begin(),
           limit = m_username_to_checklist.end(); pos != limit; ++pos) {
      const Checklist* checklist = pos->second;
      checklist->check_invariants();
    }
  }

  void EndpointManager::schedule_for_destruction() {
    m_scheduled_for_destruction = true;
    UsernameToChecklistType old_checklists = m_username_to_checklist;
    for (UsernameToChecklistType::const_iterator pos = old_checklists.begin(),
           limit = old_checklists.end(); pos != limit; ++pos) {
      pos->second->remove_guids();
    }
  }

  void EndpointManager::unfreeze(FoundationType const & a_foundation) {
    for (UsernameToChecklistType::const_iterator pos = m_username_to_checklist.begin(),
           limit = m_username_to_checklist.end(); pos != limit; ++pos) {
      pos->second->unfreeze(a_foundation);
    }
  }

  EndpointManager::ServerReflexiveTask::ServerReflexiveTask(EndpointManager * a_endpoint_manager)
    : Task(a_endpoint_manager->agent_impl),
      endpoint_manager(a_endpoint_manager) {
    enqueue(ACE_Time_Value().now());
  }

  void EndpointManager::ServerReflexiveTask::execute(ACE_Time_Value const & a_now) {
    if (endpoint_manager->m_scheduled_for_destruction) {
      delete endpoint_manager;
      return;
    }
    endpoint_manager->server_reflexive_task(a_now);
    enqueue(a_now + endpoint_manager->agent_impl->get_configuration().server_reflexive_address_period());
  }

  EndpointManager::ChangePasswordTask::ChangePasswordTask(EndpointManager * a_endpoint_manager)
    : Task(a_endpoint_manager->agent_impl),
      endpoint_manager(a_endpoint_manager) {
    enqueue(ACE_Time_Value().now() + endpoint_manager->agent_impl->get_configuration().change_password_period());
  }

  void EndpointManager::ChangePasswordTask::execute(ACE_Time_Value const & a_now) {
    endpoint_manager->change_password(true);
    enqueue(a_now + endpoint_manager->agent_impl->get_configuration().change_password_period());
  }

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
