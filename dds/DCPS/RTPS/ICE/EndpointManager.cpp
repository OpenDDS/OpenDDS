/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EndpointManager.h"

#include <ace/Reverse_Lock_T.h>
#include "dds/DCPS/security/framework/SecurityRegistry.h"
#include "dds/DCPS/security/framework/SecurityConfig.h"
#include "dds/DCPS/SafetyProfileStreams.h"

#include "Checklist.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

#ifdef OPENDDS_SECURITY

using DCPS::MonotonicTimePoint;

EndpointManager::EndpointManager(AgentImpl* a_agent_impl, Endpoint* a_endpoint) :
  agent_impl(a_agent_impl),
  endpoint(a_endpoint),
  scheduled_for_destruction_(false),
  requesting_(true),
  send_count_(0),
  server_reflexive_task_(this),
  change_password_task_(this)
{

  binding_request_.clear_transaction_id();

  // Set the type.
  agent_info_.type = FULL;

  TheSecurityRegistry->fix_empty_default()->get_utility()->generate_random_bytes(&ice_tie_breaker_, sizeof(ice_tie_breaker_));

  change_username();
  set_host_addresses(endpoint->host_addresses());
}

void EndpointManager::start_ice(const DCPS::RepoId& a_local_guid,
                                const DCPS::RepoId& a_remote_guid,
                                const AgentInfo& a_remote_agent_info)
{
  // Check for username collision.
  if (a_remote_agent_info.username == agent_info_.username) {
    change_username();
  }

  GuidPair guidp(a_local_guid, a_remote_guid);

  // Try to find by guid.
  Checklist* guid_checklist = 0;
  {
    GuidPairToChecklistType::const_iterator pos = guid_pair_to_checklist_.find(guidp);

    if (pos != guid_pair_to_checklist_.end()) {
      guid_checklist = pos->second;
    }
  }

  // Try to find by username.
  Checklist* username_checklist = 0;
  {
    UsernameToChecklistType::const_iterator pos = username_to_checklist_.find(a_remote_agent_info.username);

    if (pos != username_to_checklist_.end()) {
      username_checklist = pos->second;
    }

    else {
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
  username_checklist = create_checklist(a_remote_agent_info);
  username_checklist->add_guids(guids);
}

void EndpointManager::stop_ice(const DCPS::RepoId& a_local_guid,
                               const DCPS::RepoId& a_remote_guid)
{
  GuidPair guidp(a_local_guid, a_remote_guid);

  GuidPairToChecklistType::const_iterator pos = guid_pair_to_checklist_.find(guidp);

  if (pos != guid_pair_to_checklist_.end()) {
    Checklist* guid_checklist = pos->second;
    guid_checklist->remove_guid(guidp);
  }
}

ACE_INET_Addr EndpointManager::get_address(const DCPS::RepoId& a_local_guid,
                                           const DCPS::RepoId& a_remote_guid) const
{
  GuidPair guidp(a_local_guid, a_remote_guid);
  GuidPairToChecklistType::const_iterator pos = guid_pair_to_checklist_.find(guidp);

  if (pos != guid_pair_to_checklist_.end()) {
    return pos->second->selected_address();
  }

  return ACE_INET_Addr();
}

void EndpointManager::receive(const ACE_INET_Addr& a_local_address,
                              const ACE_INET_Addr& a_remote_address,
                              const STUN::Message& a_message)
{
  switch (a_message.class_) {
  case STUN::REQUEST:
    request(a_local_address, a_remote_address, a_message);
    return;

  case STUN::INDICATION:
    indication(a_local_address, a_remote_address, a_message);
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

void EndpointManager::change_username()
{
  // Generate the username.
  unsigned char username[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  TheSecurityRegistry->fix_empty_default()->get_utility()->generate_random_bytes(username, sizeof(username));
  agent_info_.username = OpenDDS::DCPS::to_hex_dds_string(username, 16, 0, 0);
  change_password(false);
}

void EndpointManager::change_password(bool password_only)
{
  unsigned char password[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  TheSecurityRegistry->fix_empty_default()->get_utility()->generate_random_bytes(password, sizeof(password));
  agent_info_.password = OpenDDS::DCPS::to_hex_dds_string(password, 16, 0, 0);
  regenerate_agent_info(password_only);
}

void EndpointManager::set_host_addresses(const AddressListType& a_host_addresses)
{
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

  if (host_addresses_ != host_addresses) {
    host_addresses_ = host_addresses;
    regenerate_agent_info(false);
  }
}

void EndpointManager::set_server_reflexive_address(const ACE_INET_Addr& a_server_reflexive_address,
                                                   const ACE_INET_Addr& a_stun_server_address)
{
  if (server_reflexive_address_ != a_server_reflexive_address ||
      stun_server_address_ != a_stun_server_address) {
    server_reflexive_address_ = a_server_reflexive_address;
    stun_server_address_ = a_stun_server_address;
    regenerate_agent_info(false);
  }
}

void EndpointManager::regenerate_agent_info(bool password_only)
{
  if (!password_only) {
    // Populate candidates.
    agent_info_.candidates.clear();

    for (AddressListType::const_iterator pos = host_addresses_.begin(), limit = host_addresses_.end(); pos != limit; ++pos) {
      agent_info_.candidates.push_back(make_host_candidate(*pos));

      if (server_reflexive_address_ != ACE_INET_Addr() &&
          stun_server_address_ != ACE_INET_Addr()) {
        agent_info_.candidates.push_back(make_server_reflexive_candidate(server_reflexive_address_, *pos, stun_server_address_));
      }
    }

    // Eliminate duplicates.
    std::sort(agent_info_.candidates.begin(), agent_info_.candidates.end(), candidates_sorted);
    AgentInfo::CandidatesType::iterator last = std::unique(agent_info_.candidates.begin(), agent_info_.candidates.end(), candidates_equal);
    agent_info_.candidates.erase(last, agent_info_.candidates.end());

    // Start over.
    UsernameToChecklistType old_checklists = username_to_checklist_;

    for (UsernameToChecklistType::const_iterator pos = old_checklists.begin(),
         limit = old_checklists.end();
         pos != limit; ++pos) {
      Checklist* old_checklist = pos->second;
      AgentInfo const remote_agent_info = old_checklist->original_remote_agent_info();
      GuidSetType const guids = old_checklist->guids();
      old_checklist->remove_guids();
      Checklist* new_checklist = create_checklist(remote_agent_info);
      new_checklist->add_guids(guids);
    }
  }

  {
    // Propagate changed agent info.

    // Make a copy.
    AgentInfoListenersType agent_info_listeners = agent_info_listeners_;
    AgentInfo agent_info = agent_info_;

    // Release the lock.
    ACE_Reverse_Lock<ACE_Recursive_Thread_Mutex> rev_lock(agent_impl->mutex);
    ACE_GUARD(ACE_Reverse_Lock<ACE_Recursive_Thread_Mutex>, rev_guard, rev_lock);

    for (AgentInfoListenersType::const_iterator pos = agent_info_listeners.begin(),
         limit =  agent_info_listeners.end(); pos != limit; ++pos) {
      pos->second->update_agent_info(pos->first, agent_info);
    }
  }
}

void EndpointManager::server_reflexive_task(const MonotonicTimePoint& a_now)
{
  // Request and maintain a server-reflexive address.
  next_stun_server_address_ = endpoint->stun_server_address();

  if (next_stun_server_address_ != ACE_INET_Addr()) {
    binding_request_ = STUN::Message();
    binding_request_.class_ = requesting_ ? STUN::REQUEST : STUN::INDICATION;
    binding_request_.method = STUN::BINDING;
    binding_request_.generate_transaction_id();
    binding_request_.append_attribute(STUN::make_fingerprint());

    send(next_stun_server_address_, binding_request_);

    if (!requesting_ && send_count_ == agent_impl->get_configuration().server_reflexive_indication_count() - 1) {
      requesting_ = true;
    }

    send_count_ = (send_count_ + 1) % agent_impl->get_configuration().server_reflexive_indication_count();
  }

  else {
    requesting_ = true;
    send_count_ = 0;
  }

  // Remove expired deferred checks.
  for (DeferredTriggeredChecksType::iterator pos = deferred_triggered_checks_.begin(),
       limit = deferred_triggered_checks_.end(); pos != limit;) {
    DeferredTriggeredCheckListType& list = pos->second;

    while (!list.empty() && list.front().expiration_date < a_now) {
      list.pop_front();
    }

    if (list.empty()) {
      deferred_triggered_checks_.erase(pos++);
    }

    else {
      ++pos;
    }
  }

  // Repopulate the host addresses.
  set_host_addresses(endpoint->host_addresses());
}

bool EndpointManager::success_response(const STUN::Message& a_message)
{
  if (a_message.transaction_id != binding_request_.transaction_id) {
    return false;
  }

  std::vector<STUN::AttributeType> unknown_attributes = a_message.unknown_comprehension_required_attributes();

  if (!unknown_attributes.empty()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::success_response: WARNING Unknown comprehension required attributes\n")));
    return true;
  }

  ACE_INET_Addr server_reflexive_address;

  if (a_message.get_mapped_address(server_reflexive_address)) {
    set_server_reflexive_address(server_reflexive_address, next_stun_server_address_);
    requesting_ = false;
    send_count_ = 0;
  }

  else {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::success_response: WARNING No (XOR)_MAPPED_ADDRESS attribute\n")));
    set_server_reflexive_address(ACE_INET_Addr(), ACE_INET_Addr());
    requesting_ = true;
    send_count_ = 0;
  }

  return true;
}

bool EndpointManager::error_response(const STUN::Message& a_message)
{
  if (a_message.transaction_id != binding_request_.transaction_id) {
    return false;
  }

  if (a_message.has_error_code()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::error_response: WARNING STUN error response code=%d reason=%s\n"), a_message.get_error_code(), a_message.get_error_reason().c_str()));

    if (a_message.get_error_code() == STUN::UNKNOWN_ATTRIBUTE && a_message.has_unknown_attributes()) {
      std::vector<STUN::AttributeType> unknown_attributes = a_message.get_unknown_attributes();

      for (std::vector<STUN::AttributeType>::const_iterator pos = unknown_attributes.begin(),
           limit = unknown_attributes.end(); pos != limit; ++pos) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::error_response: WARNING Unknown STUN attribute %d\n"), *pos));
      }
    }
  }

  else {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::error_response: WARNING STUN error response (no code)\n")));
  }

  return true;
}

Checklist* EndpointManager::create_checklist(const AgentInfo& remote_agent_info)
{
  Checklist* checklist = new Checklist(this, agent_info_, remote_agent_info, ice_tie_breaker_);
  // Add the deferred triggered first in case there was a nominating check.
  DeferredTriggeredChecksType::iterator pos = deferred_triggered_checks_.find(remote_agent_info.username);

  if (pos != deferred_triggered_checks_.end()) {
    const DeferredTriggeredCheckListType& list = pos->second;

    for (DeferredTriggeredCheckListType::const_iterator pos2 = list.begin(), limit2 = list.end(); pos2 != limit2; ++pos2) {
      checklist->generate_triggered_check(pos2->local_address, pos2->remote_address, pos2->priority, pos2->use_candidate);
    }

    deferred_triggered_checks_.erase(pos);
  }

  checklist->unfreeze();

  return checklist;
}

STUN::Message EndpointManager::make_unknown_attributes_error_response(const STUN::Message& a_message,
                                                                      const std::vector<STUN::AttributeType>& a_unknown_attributes)
{
  STUN::Message response;
  response.class_ = STUN::ERROR_RESPONSE;
  response.method = a_message.method;
  memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
  response.append_attribute(STUN::make_error_code(STUN::UNKNOWN_ATTRIBUTE, "Unknown Attributes"));
  response.append_attribute(STUN::make_unknown_attributes(a_unknown_attributes));
  response.append_attribute(STUN::make_message_integrity());
  response.password = agent_info_.password;
  response.append_attribute(STUN::make_fingerprint());
  return response;
}

STUN::Message EndpointManager::make_bad_request_error_response(const STUN::Message& a_message,
                                                               const std::string& a_reason)
{
  STUN::Message response;
  response.class_ = STUN::ERROR_RESPONSE;
  response.method = a_message.method;
  memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
  response.append_attribute(STUN::make_error_code(STUN::BAD_REQUEST, a_reason));
  response.append_attribute(STUN::make_message_integrity());
  response.password = agent_info_.password;
  response.append_attribute(STUN::make_fingerprint());
  return response;
}

STUN::Message EndpointManager::make_unauthorized_error_response(const STUN::Message& a_message)
{
  STUN::Message response;
  response.class_ = STUN::ERROR_RESPONSE;
  response.method = a_message.method;
  memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
  response.append_attribute(STUN::make_error_code(STUN::UNAUTHORIZED, "Unauthorized"));
  response.append_attribute(STUN::make_message_integrity());
  response.password = agent_info_.password;
  response.append_attribute(STUN::make_fingerprint());
  return response;
}

// STUN Message processing.
void EndpointManager::request(const ACE_INET_Addr& a_local_address,
                              const ACE_INET_Addr& a_remote_address,
                              const STUN::Message& a_message)
{
  std::string username;

  if (!a_message.get_username(username)) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No USERNAME attribute\n")));
    send(a_remote_address,
         make_bad_request_error_response(a_message, "Bad Request: USERNAME must be present"));
    return;
  }

  if (!a_message.has_message_integrity()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No MESSAGE_INTEGRITY attribute\n")));
    send(a_remote_address,
         make_bad_request_error_response(a_message, "Bad Request: MESSAGE_INTEGRITY must be present"));
    return;
  }

  size_t idx = username.find(':');

  if (idx == std::string::npos) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING USERNAME does not contain a colon\n")));
    send(a_remote_address,
         make_bad_request_error_response(a_message, "Bad Request: USERNAME must be colon-separated"));
    return;
  }

  if (username.substr(0, idx) != agent_info_.username) {
    // We expect this to happen.
    send(a_remote_address,
         make_unauthorized_error_response(a_message));
    return;
  }

  const std::string remote_username = username.substr(++idx);

  // Check the message_integrity.
  if (!a_message.verify_message_integrity(agent_info_.password)) {
    // We expect this to happen.
    send(a_remote_address,
         make_unauthorized_error_response(a_message));
    return;
  }

  std::vector<STUN::AttributeType> unknown_attributes = a_message.unknown_comprehension_required_attributes();

  if (!unknown_attributes.empty()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING Unknown comprehension required attributes\n")));
    send(a_remote_address,
         make_unknown_attributes_error_response(a_message, unknown_attributes));
    return;
  }

  if (!a_message.has_fingerprint()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No FINGERPRINT attribute\n")));
    send(a_remote_address,
         make_bad_request_error_response(a_message, "Bad Request: FINGERPRINT must be present"));
    return;
  }

  if (!a_message.has_ice_controlled() && !a_message.has_ice_controlling()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No ICE_CONTROLLED/ICE_CONTROLLING attribute\n")));
    send(a_remote_address,
         make_bad_request_error_response(a_message, "Bad Request: Either ICE_CONTROLLED or ICE_CONTROLLING must be present"));
    return;
  }

  bool use_candidate = a_message.has_use_candidate();

  if (use_candidate && a_message.has_ice_controlled()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING USE_CANDIDATE without ICE_CONTROLLED\n")));
    send(a_remote_address,
         make_bad_request_error_response(a_message, "Bad Request: USE_CANDIDATE can only be present when ICE_CONTROLLED is present"));
    return;
  }

  ACE_UINT32 priority;

  if (!a_message.get_priority(priority)) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING No PRIORITY attribute\n")));
    send(a_remote_address,
         make_bad_request_error_response(a_message, "Bad Request: PRIORITY must be present"));
    return;
  }

  switch (a_message.method) {
  case STUN::BINDING: {
    // 7.3
    STUN::Message response;
    response.class_ = STUN::SUCCESS_RESPONSE;
    response.method = STUN::BINDING;
    memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
    response.append_attribute(STUN::make_mapped_address(a_remote_address));
    response.append_attribute(STUN::make_xor_mapped_address(a_remote_address));
    response.append_attribute(STUN::make_message_integrity());
    response.password = agent_info_.password;
    response.append_attribute(STUN::make_fingerprint());
    send(a_remote_address, response);

    // 7.3.1.3
    UsernameToChecklistType::const_iterator pos = username_to_checklist_.find(remote_username);

    if (pos != username_to_checklist_.end()) {
      // We have a checklist.
      Checklist* checklist = pos->second;
      checklist->generate_triggered_check(a_local_address, a_remote_address, priority, use_candidate);
    }

    else {
      std::pair<DeferredTriggeredChecksType::iterator, bool> x =
        deferred_triggered_checks_.insert(std::make_pair(remote_username, DeferredTriggeredCheckListType()));
      x.first->second.push_back(DeferredTriggeredCheck(
        a_local_address, a_remote_address, priority, use_candidate,
        MonotonicTimePoint::now() + agent_impl->get_configuration().deferred_triggered_check_ttl()));
    }
  }
  break;

  default:
    // Unknown method.  Stop processing.
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) EndpointManager::request: WARNING Unknown ICE method\n")));
    send(a_remote_address,
         make_bad_request_error_response(a_message, "Bad Request: Unknown method"));
    break;
  }
}

void EndpointManager::indication(const ACE_INET_Addr& /*a_local_address*/,
                                 const ACE_INET_Addr& /*a_remote_address*/,
                                 const STUN::Message& a_message)
{
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

  if (username.substr(0, idx) != agent_info_.username) {
    // We expect this to happen.
    return;
  }

  const std::string remote_username = username.substr(++idx);

  // Check the message_integrity.
  if (!a_message.verify_message_integrity(agent_info_.password)) {
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
  case STUN::BINDING: {
    // Section 11
    UsernameToChecklistType::const_iterator pos = username_to_checklist_.find(remote_username);

    if (pos != username_to_checklist_.end()) {
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

void EndpointManager::success_response(const ACE_INET_Addr& a_local_address,
                                       const ACE_INET_Addr& a_remote_address,
                                       const STUN::Message& a_message)
{
  switch (a_message.method) {
  case STUN::BINDING: {
    if (success_response(a_message)) {
      return;
    }

    TransactionIdToChecklistType::const_iterator pos = transaction_id_to_checklist_.find(a_message.transaction_id);

    if (pos == transaction_id_to_checklist_.end()) {
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

void EndpointManager::error_response(const ACE_INET_Addr& a_local_address,
                                     const ACE_INET_Addr& a_remote_address,
                                     const STUN::Message& a_message)
{
  switch (a_message.method) {
  case STUN::BINDING: {
    if (error_response(a_message)) {
      return;
    }

    TransactionIdToChecklistType::const_iterator pos = transaction_id_to_checklist_.find(a_message.transaction_id);

    if (pos == transaction_id_to_checklist_.end()) {
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

void EndpointManager::compute_active_foundations(ActiveFoundationSet& a_active_foundations) const
{
  for (UsernameToChecklistType::const_iterator pos = username_to_checklist_.begin(),
       limit = username_to_checklist_.end(); pos != limit; ++pos) {
    const Checklist* checklist = pos->second;
    checklist->compute_active_foundations(a_active_foundations);
  }
}

void EndpointManager::check_invariants() const
{
  for (UsernameToChecklistType::const_iterator pos = username_to_checklist_.begin(),
       limit = username_to_checklist_.end(); pos != limit; ++pos) {
    const Checklist* checklist = pos->second;
    checklist->check_invariants();
  }
}

void EndpointManager::schedule_for_destruction()
{
  scheduled_for_destruction_ = true;
  UsernameToChecklistType old_checklists = username_to_checklist_;

  for (UsernameToChecklistType::const_iterator pos = old_checklists.begin(),
       limit = old_checklists.end(); pos != limit; ++pos) {
    pos->second->remove_guids();
  }
}

void EndpointManager::unfreeze()
{
  for (UsernameToChecklistType::const_iterator pos = username_to_checklist_.begin(),
       limit = username_to_checklist_.end(); pos != limit; ++pos) {
    pos->second->unfreeze();
  }
}

void EndpointManager::unfreeze(const FoundationType& a_foundation)
{
  for (UsernameToChecklistType::const_iterator pos = username_to_checklist_.begin(),
       limit = username_to_checklist_.end(); pos != limit; ++pos) {
    pos->second->unfreeze(a_foundation);
  }
}

EndpointManager::ServerReflexiveTask::ServerReflexiveTask(EndpointManager* a_endpoint_manager)
  : Task(a_endpoint_manager->agent_impl),
    endpoint_manager(a_endpoint_manager)
{
  enqueue(MonotonicTimePoint::now());
}

void EndpointManager::ServerReflexiveTask::execute(const MonotonicTimePoint& a_now)
{
  if (endpoint_manager->scheduled_for_destruction_) {
    delete endpoint_manager;
    return;
  }

  endpoint_manager->server_reflexive_task(a_now);
  enqueue(a_now + endpoint_manager->agent_impl->get_configuration().server_reflexive_address_period());
}

EndpointManager::ChangePasswordTask::ChangePasswordTask(EndpointManager* a_endpoint_manager)
  : Task(a_endpoint_manager->agent_impl),
    endpoint_manager(a_endpoint_manager)
{
  enqueue(MonotonicTimePoint::now() + endpoint_manager->agent_impl->get_configuration().change_password_period());
}

void EndpointManager::ChangePasswordTask::execute(const MonotonicTimePoint& a_now)
{
  endpoint_manager->change_password(true);
  enqueue(a_now + endpoint_manager->agent_impl->get_configuration().change_password_period());
}

void EndpointManager::network_change()
{
  set_host_addresses(endpoint->host_addresses());
}

void EndpointManager::send(const ACE_INET_Addr& address, const STUN::Message& message)
{
  endpoint->send(address, message);
}

#endif /* OPENDDS_SECURITY */

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
