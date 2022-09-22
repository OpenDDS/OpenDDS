/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "AccessControlBuiltInImpl.h"

#include "AuthenticationBuiltInImpl.h"
#include "CommonUtilities.h"
#include "TokenWriter.h"
#include "SSL/SubjectName.h"

#include <dds/DCPS/Service_Participant.h>

#include <dds/DdsDcpsInfrastructureC.h>

#include <ace/ACE.h>
#include <ace/config-macros.h>
#include <ace/OS_NS_strings.h>
#include <ace/Reactor.h>

#include <time.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <iterator>
#include <cstring>
#include <iomanip>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

using DCPS::TimeDuration;

typedef Governance::GovernanceAccessRules::iterator gov_iter;
typedef Permissions::Grants::iterator grant_iter;
typedef Permissions::Rules::iterator perm_topic_rules_iter;
typedef Permissions::Actions::iterator perm_topic_actions_iter;

static const std::string PermissionsTokenClassId("DDS:Access:Permissions:1.0");
static const std::string AccessControl_Plugin_Name("DDS:Access:Permissions");
static const std::string AccessControl_Major_Version("1");
static const std::string AccessControl_Minor_Version("0");

static const std::string PermissionsCredentialTokenClassId("DDS:Access:PermissionsCredential");

bool AccessControlBuiltInImpl::pattern_match(const char* string, const char* pattern)
{
  return ACE::wild_match(string, pattern, true, true);
}

AccessControlBuiltInImpl::AccessControlBuiltInImpl()
  : handle_mutex_()
  , gen_handle_mutex_()
  , next_handle_(1)
  , listener_ptr_(0)
{  }

AccessControlBuiltInImpl::~AccessControlBuiltInImpl()
{
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::~AccessControlBuiltInImpl local_ac_perms_ %B local_identity_map_ %B\n"),
               local_ac_perms_.size(),
               local_identity_map_.size()));
  }
}

::DDS::Security::PermissionsHandle AccessControlBuiltInImpl::validate_local_permissions(
  ::DDS::Security::Authentication_ptr auth_plugin,
  ::DDS::Security::IdentityHandle identity,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::DomainParticipantQos & participant_qos,
  ::DDS::Security::SecurityException & ex)
{
  if (0 == auth_plugin) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Null Authentication plugin");
    return DDS::HANDLE_NIL;
  }

  if (DDS::HANDLE_NIL == identity) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Invalid identity");
    return DDS::HANDLE_NIL;
  }

  DDS::Security::IdentityToken id_token;

  if (!auth_plugin->get_identity_token(id_token, identity, ex)) {
    return DDS::HANDLE_NIL;
  }

  LocalAccessCredentialData::shared_ptr local_access_credential_data = DCPS::make_rch<LocalAccessCredentialData>();

  if (! local_access_credential_data->load(participant_qos.property.value, ex)) {
    return DDS::HANDLE_NIL;
  }

  const SSL::Certificate& local_ca = local_access_credential_data->get_ca_cert();
  const SSL::SignedDocument& local_gov = local_access_credential_data->get_governance_doc();

  if (local_gov.verify_signature(local_ca)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Governance signature not verified");
    return DDS::HANDLE_NIL;
  }

  Governance::shared_ptr governance = DCPS::make_rch<Governance>();

  if (governance->load(local_gov)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Invalid governance file");
    return DDS::HANDLE_NIL;
  }

  const SSL::SignedDocument& local_perm = local_access_credential_data->get_permissions_doc();
  Permissions::shared_ptr permissions = DCPS::make_rch<Permissions>();

  if (permissions->load(local_perm)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Invalid permission file");
    return DDS::HANDLE_NIL;
  }

  if (local_perm.verify_signature(local_ca)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Permissions signature not verified");
    return DDS::HANDLE_NIL;
  } else if (DCPS::DCPS_debug_level) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) AccessControlBuiltInImpl::validate_local_permissions: Permissions document verified.\n")));
  }

  TokenReader tr(id_token);
  const char* id_sn = tr.get_property_value(dds_cert_sn);

  OpenDDS::Security::SSL::SubjectName sn_id;

  if (!id_sn || sn_id.parse(id_sn) != 0 || !permissions->has_grant(sn_id)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: No permissions subject name matches identity subject name");
    return DDS::HANDLE_NIL;
  }

  // Set and store the permissions credential token while we have the raw content
  DDS::Security::PermissionsCredentialToken permissions_cred_token;
  TokenWriter pctWriter(permissions_cred_token, PermissionsCredentialTokenClassId);

  pctWriter.add_property("dds.perm.cert", local_perm.get_original());

  // Set and store the permissions token
  DDS::Security::PermissionsToken permissions_token;
  TokenWriter writer(permissions_token, PermissionsTokenClassId);

  // If all checks are successful load the content into cache
  permissions->perm_token_ = permissions_token;
  permissions->perm_cred_token_ = permissions_cred_token;

  const int perm_handle = generate_handle();

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, DDS::HANDLE_NIL);

  AccessData cache_this;
  cache_this.identity = identity;
  cache_this.subject = sn_id;
  cache_this.domain_id = domain_id;
  cache_this.perm = permissions;
  cache_this.gov = governance;
  cache_this.local_access_credential_data = local_access_credential_data;

  local_ac_perms_.insert(std::make_pair(perm_handle, cache_this));
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::validate_local_permissions local_ac_perms_ (total %B)\n"),
               local_ac_perms_.size()));
  }
  local_identity_map_.insert(std::make_pair(identity, perm_handle));
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::validate_local_permissions local_identity_map_ (total %B)\n"),
               local_identity_map_.size()));
  }

  return perm_handle;
}

::DDS::Security::PermissionsHandle AccessControlBuiltInImpl::validate_remote_permissions(
  ::DDS::Security::Authentication_ptr auth_plugin,
  ::DDS::Security::IdentityHandle local_identity_handle,
  ::DDS::Security::IdentityHandle remote_identity_handle,
  const ::DDS::Security::PermissionsToken & remote_permissions_token,
  const ::DDS::Security::AuthenticatedPeerCredentialToken & remote_credential_token,
  ::DDS::Security::SecurityException & ex)
{
  if (0 == auth_plugin) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Null Authentication plugin");
    return DDS::HANDLE_NIL;
  }

  if (DDS::HANDLE_NIL == local_identity_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Invalid Local Identity");
    return DDS::HANDLE_NIL;
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, DDS::HANDLE_NIL);

  ACIdentityMap::iterator iter = local_identity_map_.find(local_identity_handle);

  if (iter == local_identity_map_.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: No matching local identity handle present");
    return DDS::HANDLE_NIL;
  }

  ACPermsMap::iterator piter = local_ac_perms_.find(iter->second);

  if (piter == local_ac_perms_.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: No matching local permissions handle present");
    return DDS::HANDLE_NIL;
  }

  // permissions file
  TokenReader remote_perm_wrapper(remote_credential_token);
  SSL::SignedDocument remote_perm_doc;

  if (remote_perm_doc.deserialize(remote_perm_wrapper.get_bin_property_value("c.perm"))) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Failed to deserialize c.perm into signed-document");
    return DDS::HANDLE_NIL;
  }

  Permissions::shared_ptr remote_permissions = DCPS::make_rch<Permissions>();

  if (remote_permissions->load(remote_perm_doc)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Invalid permission file");
    return DDS::HANDLE_NIL;
  }

  const LocalAccessCredentialData::shared_ptr& local_access_credential_data = piter->second.local_access_credential_data;

  // Validate the signature of the remote permissions
  const SSL::Certificate& local_ca = local_access_credential_data->get_ca_cert();
  std::string ca_subject;

  local_ca.subject_name_to_str(ca_subject);

  if (remote_perm_doc.verify_signature(local_ca)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Remote permissions signature not verified");
    return DDS::HANDLE_NIL;
  }

  // The remote permissions signature is verified
  if (DCPS::DCPS_debug_level) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT(
      "(%P|%t) AccessControlBuiltInImpl::validate_remote_permissions: Remote permissions document verified.\n")));
  }

  //Extract and compare the remote subject name for validation
  TokenReader remote_credential_tr(remote_credential_token);
  const DDS::OctetSeq& cid = remote_credential_tr.get_bin_property_value("c.id");

  if (cid.length() == 0) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Invalid remote credential identity");
    return DDS::HANDLE_NIL;
  }

  SSL::Certificate::unique_ptr remote_cert(new SSL::Certificate);
  remote_cert->deserialize(cid);

  std::string remote_identity_sn;
  remote_cert->subject_name_to_str(remote_identity_sn);

  SSL::SubjectName sn_id_remote;

  if (remote_identity_sn.empty() || sn_id_remote.parse(remote_identity_sn) != 0 || !remote_permissions->has_grant(sn_id_remote)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: "
                                        "Remote identity subject name does not match any subject name in remote permissions grants");
    return DDS::HANDLE_NIL;
  }

  // Set and store the permissions credential token while we have the raw content
  remote_permissions->perm_token_ = remote_permissions_token;
  remote_permissions->perm_cred_token_ = remote_credential_token;


  AccessData cache_this;
  cache_this.identity = remote_identity_handle;
  cache_this.subject = sn_id_remote;
  cache_this.domain_id = piter->second.domain_id;
  cache_this.perm = remote_permissions;
  cache_this.gov = piter->second.gov;
  cache_this.local_access_credential_data = local_access_credential_data;

  const int perm_handle = generate_handle();
  local_ac_perms_.insert(std::make_pair(perm_handle, cache_this));
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::validate_remote_permissions local_ac_perms_ (total %B)\n"),
               local_ac_perms_.size()));
  }
  return perm_handle;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_create_participant(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::DomainParticipantQos & /*qos*/,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_participant: Invalid permissions handle");
  }

/*
 *  The rules of this method need to be evaluated in this order, however, we need to check
 *  to make sure the permission handle exists in our store prior to assessing these rules
*/
  /* From Table 63 of the spec.
     This operation shall use the permissions_handle to retrieve
     the cached Permissions and Governance information.
             If the Governance specifies any topics on the
     DomainParticipant domain_id with
     enable_read_access_control set to FALSE or with
     enable_write_access_control set to FALSE, then the
     operation shall succeed and return TRUE.
             If the ParticipantSecurityAttributes has
     is_access_protected set to FALSE, then the operation shall
     succeed and return TRUE.
             Otherwise the operation shall return FALSE.
 */

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator piter = local_ac_perms_.find(permissions_handle);

  if (piter == local_ac_perms_.end()) {
    return CommonUtilities::set_security_error(ex, -1, 0,
      "AccessControlBuiltInImpl::check_create_participant: "
      "No matching permissions handle present");
  }

  if (domain_id != piter->second.domain_id) {
    return CommonUtilities::set_security_error(ex, -1, 0,
      "AccessControlBuiltInImpl::check_create_participant: "
      "Domain does not match validated permissions handle");
  }

  gov_iter begin = piter->second.gov->access_rules().begin();
  gov_iter end = piter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(domain_id)) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (!tr_iter->topic_attrs.is_read_protected || !tr_iter->topic_attrs.is_write_protected) {
          return true;
        }
      }

      if (!giter->domain_attrs.is_access_protected) {
        return true;
      }
    }
  }

  return CommonUtilities::set_security_error(ex, -1, 0,
    "AccessControlBuiltInImpl::check_create_participant: "
    "No governance exists for this domain");
}

::CORBA::Boolean AccessControlBuiltInImpl::check_create_datawriter(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const char * topic_name,
  const ::DDS::DataWriterQos & /*qos*/,
  const ::DDS::PartitionQosPolicy & partition,
  const ::DDS::Security::DataTags & /*data_tag*/,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: Invalid permissions handle");
  }
  if (0 == topic_name) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: Invalid Topic Name");
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: No matching permissions handle present");
  }

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(domain_id)) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (pattern_match(topic_name, tr_iter->topic_expression.c_str())) {
          if (!tr_iter->topic_attrs.is_write_protected) {
            return true;
          }
        }
      }
    }
  }

  // Check the Permissions file

  const Permissions::Grant_rch grant = ac_iter->second.perm->find_grant(ac_iter->second.subject);
  if (!grant) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: Permissions grant not found");
  }

  if (!validate_date_time(grant->validity, ex)) {
    return false;
  }

  if (!search_permissions(topic_name, domain_id, partition, Permissions::PUBLISH, *grant, ex)) {
    return false;
  }

  make_task(local_rp_task_)->insert(permissions_handle, grant->validity.not_after);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_create_datareader(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const char * topic_name,
  const ::DDS::DataReaderQos & /*qos*/,
  const ::DDS::PartitionQosPolicy & partition,
  const ::DDS::Security::DataTags & /*data_tag*/,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datareader: Invalid permissions handle");
  }

  if (0 == topic_name) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datareader: Invalid Topic Name");
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datareader: No matching permissions handle present");
  }

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(domain_id)) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (pattern_match(topic_name, tr_iter->topic_expression.c_str())) {
          if (!tr_iter->topic_attrs.is_read_protected) {
            return true;
          }
        }
      }
    }
  }

  // Check the Permissions file

  const Permissions::Grant_rch grant = ac_iter->second.perm->find_grant(ac_iter->second.subject);
  if (!grant) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datareader: Permissions grant not found");
  }

  if (!validate_date_time(grant->validity, ex)) {
    return false;
  }

  if (!search_permissions(topic_name, domain_id, partition, Permissions::SUBSCRIBE, *grant, ex)) {
    return false;
  }

  make_task(local_rp_task_)->insert(permissions_handle, grant->validity.not_after);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_create_topic(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const char * topic_name,
  const ::DDS::TopicQos & /*qos*/,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_topic: Invalid permissions handle");
  }
  if (0 == topic_name) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_topic: Invalid Topic Name");
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_topic: No matching permissions handle present");
  }

  // Check the Governance file for allowable topic attributes

  if (domain_id != ac_iter->second.domain_id) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_topic: Requested domain ID does not match permissions handle");
  }

  ::DDS::Security::DomainId_t domain_to_find = ac_iter->second.domain_id;

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(domain_to_find)) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (pattern_match(topic_name, tr_iter->topic_expression.c_str())) {
          if (!tr_iter->topic_attrs.is_read_protected || !tr_iter->topic_attrs.is_write_protected) {
            return true;
          }
        }
      }
    }
  }

  const Permissions::Grant_rch grant = ac_iter->second.perm->find_grant(ac_iter->second.subject);
  if (!grant) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_topic: grant not found");
  }

  if (!validate_date_time(grant->validity, ex)) {
    return false;
  }

  Permissions::PublishSubscribe_t denied_type;
  bool found_deny = false;
  // Iterate over allow / deny rules
  for (perm_topic_rules_iter ptr_iter = grant->rules.begin(); ptr_iter != grant->rules.end(); ++ptr_iter) {

    if (ptr_iter->domains.has(domain_to_find)) {

      perm_topic_actions_iter tpsr_iter;
      for (tpsr_iter = ptr_iter->actions.begin(); tpsr_iter != ptr_iter->actions.end(); ++tpsr_iter) {

        std::vector<std::string>::iterator tl_iter;
        for (tl_iter = tpsr_iter->topics.begin(); tl_iter != tpsr_iter->topics.end(); ++tl_iter) {

          if (pattern_match(topic_name, tl_iter->c_str())) {
            if (ptr_iter->ad_type == Permissions::ALLOW) {
              return true;
            }
            if (found_deny && denied_type != tpsr_iter->ps_type) {
              return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_topic: Both publish and subscribe are denied for this topic.");
            } else if (!found_deny) {
              found_deny = true;
              denied_type = tpsr_iter->ps_type;
            }
          }
        }
      }
    }
  }

  // There is no matching rule for topic_name so use the value in default_permission
  if (grant->default_permission == Permissions::ALLOW) {
    return true;
  } else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_topic: No matching rule for topic, default permission is DENY.");
  }
}

::CORBA::Boolean AccessControlBuiltInImpl::check_local_datawriter_register_instance(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::DataWriter_ptr writer,
  ::DDS::DynamicData_ptr key,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datawriter_register_instance: Invalid permissions handle");
  }
  if (0 == writer) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datawriter_register_instance: Invalid Writer");
  }
  if (0 == key) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datawriter_register_instance: Invalid Topic Key");
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_local_datawriter_dispose_instance(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::DataWriter_ptr writer,
  ::DDS::DynamicData_ptr key,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datawriter_dispose_instance: Invalid permissions handle");
  }
  if (0 == writer) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datawriter_dispose_instance: Invalid Writer");
  }
  if (0 == key) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datawriter_dispose_instance: Invalid Topic Key");
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_participant(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::Security::ParticipantBuiltinTopicDataSecure & participant_data,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Invalid permissions handle");
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    return CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::check_remote_participant: No matching permissions handle present");
  }

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {
    if (giter->domains.has(domain_id) && !giter->domain_attrs.is_access_protected) {
      return true;
    }
  }

  // Check the PluginClassName and MajorVersion of the local permissions vs. remote  See Table 63 of spec
  const std::string remote_class_id = participant_data.base.permissions_token.class_id.in();

  std::string local_plugin_class_name,
              remote_plugin_class_name;
  int local_major_ver = 0,
      local_minor_ver,
      remote_major_ver,
      remote_minor_ver;

  if (remote_class_id.length() > 0) {
    parse_class_id(remote_class_id, remote_plugin_class_name, remote_major_ver, remote_minor_ver);
  } else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Invalid remote class ID");
  }

  for (ACPermsMap::iterator local_iter = local_ac_perms_.begin(); local_iter != local_ac_perms_.end(); ++local_iter) {
    if (local_iter->second.domain_id == domain_id && local_iter->first != permissions_handle) {
      const std::string local_class_id = local_iter->second.perm->perm_token_.class_id.in();

      if (local_class_id.length() > 0) {
        parse_class_id(local_class_id, local_plugin_class_name, local_major_ver, local_minor_ver);
        break;
      } else {
        return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Invalid local class ID");
      }
    }
  }

  if (strcmp(local_plugin_class_name.c_str(), remote_plugin_class_name.c_str())) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Class ID plugin class name do not match");
  }

  if (local_major_ver != remote_major_ver) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Class ID major versions do not match");
  }

  const Permissions::Grant_rch grant = ac_iter->second.perm->find_grant(ac_iter->second.subject);
  if (!grant) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Permissions grant not found");
  }

  for (perm_topic_rules_iter ptr_iter = grant->rules.begin(); ptr_iter != grant->rules.end(); ++ptr_iter) {
    if (ptr_iter->domains.has(domain_id) && ptr_iter->ad_type == Permissions::ALLOW) {
      return true;
    }
  }

  if (grant->default_permission == Permissions::ALLOW) {
    return true; // DDSSEC12-85
  }

  return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Not authorized for domain");
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_datawriter(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::Security::PublicationBuiltinTopicDataSecure & publication_data,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter: Invalid permissions handle");
  }

  if (publication_data.base.base.topic_name[0] == 0) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter: Invalid topic name");
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter: No matching permissions handle present");
  }

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(domain_id)) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (pattern_match(publication_data.base.base.topic_name, tr_iter->topic_expression.c_str())) {
          if (!tr_iter->topic_attrs.is_write_protected) {
            return true;
          }
        }
      }
    }
  }

  const Permissions::Grant_rch grant = ac_iter->second.perm->find_grant(ac_iter->second.subject);
  if (!grant) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter: Permissions grant not found");
  }

  if (!validate_date_time(grant->validity, ex)) {
    return false;
  }

  if (!search_permissions(publication_data.base.base.topic_name, domain_id,
                          publication_data.base.base.partition, Permissions::PUBLISH,
                          *grant, ex)) {
    return false;
  }

  make_task(remote_rp_task_)->insert(permissions_handle, grant->validity.not_after);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_datareader(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::Security::SubscriptionBuiltinTopicDataSecure & subscription_data,
  ::CORBA::Boolean & relay_only,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datareader: Invalid permissions handle");
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datareader: No matching permissions handle present");
  }

  // Default this to false for now
  relay_only = false;

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(domain_id)) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (pattern_match(subscription_data.base.base.topic_name, tr_iter->topic_expression.c_str())) {
          if (!tr_iter->topic_attrs.is_read_protected) {
            return true;
          }
        }
      }
    }
  }

  const Permissions::Grant_rch grant = ac_iter->second.perm->find_grant(ac_iter->second.subject);
  if (!grant) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datareader: Permissions grant not found");
  }

  if (!validate_date_time(grant->validity, ex)) {
    return false;
  }

  if (!search_permissions(subscription_data.base.base.topic_name, domain_id,
                          subscription_data.base.base.partition, Permissions::SUBSCRIBE,
                          *grant, ex)) {
    return false;
  }

  make_task(remote_rp_task_)->insert(permissions_handle, grant->validity.not_after);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_topic(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::TopicBuiltinTopicData & topic_data,
  ::DDS::Security::SecurityException & ex)
{
  // NOTE: permissions_handle is for the remote DomainParticipant.
  if (DDS::HANDLE_NIL == permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Invalid permissions handle");
  }

  if (topic_data.name[0] == 0) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Invalid topic data");
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    return CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::check_remote_topic: No matching permissions handle present");
  }

  // Compare the PluginClassName and MajorVersion of the local permissions_token
  // with those in the remote_permissions_token.
  const std::string remote_class_id = ac_iter->second.perm->perm_token_.class_id.in();

  std::string local_plugin_class_name,
              remote_plugin_class_name;
  int local_major_ver = 0,
      local_minor_ver,
      remote_major_ver,
      remote_minor_ver;

  if (remote_class_id.length() > 0) {
    parse_class_id(remote_class_id, remote_plugin_class_name, remote_major_ver, remote_minor_ver);
  } else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Invalid remote class ID");
  }

  for (ACPermsMap::iterator local_iter = local_ac_perms_.begin(); local_iter != local_ac_perms_.end(); ++local_iter) {
    if (local_iter->second.domain_id == domain_id && local_iter->first != permissions_handle) {
      const std::string local_class_id = local_iter->second.perm->perm_token_.class_id.in();

      if (local_class_id.length() > 0) {
        parse_class_id(local_class_id, local_plugin_class_name, local_major_ver, local_minor_ver);
        break;
      } else {
        return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Invalid local class ID");
      }
    }
  }

  if (strcmp(local_plugin_class_name.c_str(), remote_plugin_class_name.c_str())) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Class ID plugin class name do not match");
  }

  if (local_major_ver != remote_major_ver) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Class ID major versions do not match");
  }

  // Check the Governance file for allowable topic attributes

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(domain_id)) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (pattern_match(topic_data.name, tr_iter->topic_expression.c_str())) {
          if (!tr_iter->topic_attrs.is_read_protected || !tr_iter->topic_attrs.is_write_protected) {
            return true;
          }
        }
      }
    }
  }

  const Permissions::Grant_rch grant = ac_iter->second.perm->find_grant(ac_iter->second.subject);
  if (!grant) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: grant not found");
  }

  if (!validate_date_time(grant->validity, ex)) {
    return false;
  }

  Permissions::PublishSubscribe_t denied_type;
  bool found_deny = false;
  for (perm_topic_rules_iter ptr_iter = grant->rules.begin(); ptr_iter != grant->rules.end(); ++ptr_iter) {

    if (ptr_iter->domains.has(domain_id)) {

      // Iterate over pub / sub rules
      perm_topic_actions_iter tpsr_iter;
      for (tpsr_iter = ptr_iter->actions.begin(); tpsr_iter != ptr_iter->actions.end(); ++tpsr_iter) {

        // Check to make sure they can publish or subscribe to the topic
        // TODO Add support for relay permissions once relay only key exchange is supported
        if (tpsr_iter->ps_type == Permissions::PUBLISH || tpsr_iter->ps_type == Permissions::SUBSCRIBE) {

          std::vector<std::string>::iterator tl_iter;
          for (tl_iter = tpsr_iter->topics.begin(); tl_iter != tpsr_iter->topics.end(); ++tl_iter) {

            if (pattern_match(topic_data.name, tl_iter->c_str())) {
              if (ptr_iter->ad_type == Permissions::ALLOW) {
                return true;
              }
              if (found_deny && denied_type != tpsr_iter->ps_type) {
                return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Both publish and subscribe are denied for this topic.");
              } else if (!found_deny) {
                found_deny = true;
                denied_type = tpsr_iter->ps_type;
              }
            }
          }
        }
      }
    }
  }

  // There is no matching rule for topic_name so use the value in default_permission
  if (grant->default_permission == Permissions::ALLOW) {
    return true;
  } else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: No matching rule for topic, default permission is DENY.");
  }
}

::CORBA::Boolean AccessControlBuiltInImpl::check_local_datawriter_match(
  ::DDS::Security::PermissionsHandle writer_permissions_handle,
  ::DDS::Security::PermissionsHandle reader_permissions_handle,
  const ::DDS::Security::PublicationBuiltinTopicDataSecure & /*publication_data*/,
  const ::DDS::Security::SubscriptionBuiltinTopicDataSecure & /*subscription_data*/,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == writer_permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datawriter_match: Invalid writer permissions handle");
  }
  if (DDS::HANDLE_NIL == reader_permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datawriter_match: Invalid reader permissions handle");
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_local_datareader_match(
  ::DDS::Security::PermissionsHandle reader_permissions_handle,
  ::DDS::Security::PermissionsHandle writer_permissions_handle,
  const ::DDS::Security::SubscriptionBuiltinTopicDataSecure & /*subscription_data*/,
  const ::DDS::Security::PublicationBuiltinTopicDataSecure & /*publication_data*/,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == writer_permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datareader_match: Invalid writer permissions handle");
  }
  if (DDS::HANDLE_NIL == reader_permissions_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_local_datareader_match: Invalid reader permissions handle");
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_datawriter_register_instance(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::DataReader_ptr reader,
  ::DDS::InstanceHandle_t publication_handle,
  ::DDS::DynamicData_ptr key,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle ||
      DDS::HANDLE_NIL == publication_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter_register_instance: Invalid handle");
  }
  if (0 == reader) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter_register_instance: Invalid Reader pointer");
  }
  if (0 == key) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter_register_instance: Invalid Topic Key");
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_datawriter_dispose_instance(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::DataReader_ptr reader,
  ::DDS::InstanceHandle_t publication_handle,
  ::DDS::DynamicData_ptr key,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle ||
      DDS::HANDLE_NIL == publication_handle) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter_dispose_instance: Invalid handle");
  }
  if (0 == reader) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter_dispose_instance: Invalid Reader pointer");
  }
  if (0 == key) {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter_dispose_instance: Invalid Topic Key");
  }
  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_permissions_token(
  ::DDS::Security::PermissionsToken & permissions_token,
  ::DDS::Security::PermissionsHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_permissions_token: Invalid permissions handle");
    return false;
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator iter = local_ac_perms_.find(handle);

  if (iter != local_ac_perms_.end()) {
    permissions_token = iter->second.perm->perm_token_;
    return true;
  } else {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_permissions_token: No PermissionToken found");
    return false;
  }
}

::CORBA::Boolean AccessControlBuiltInImpl::get_permissions_credential_token(
  ::DDS::Security::PermissionsCredentialToken & permissions_credential_token,
  ::DDS::Security::PermissionsHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_permissions_credential_token: Invalid permissions handle");
    return false;
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator iter = local_ac_perms_.find(handle);

  if (iter != local_ac_perms_.end()) {
    permissions_credential_token = iter->second.perm->perm_cred_token_;
    return true;
  } else {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_permissions_credential_token: No PermissionToken found");
    return false;
  }
}

::CORBA::Boolean AccessControlBuiltInImpl::set_listener(
  ::DDS::Security::AccessControlListener_ptr listener,
  ::DDS::Security::SecurityException & ex)
{
  if (0 == listener) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::set_listener: Invalid Listener pointer");
    return false;
  }

  ACE_Guard<ACE_Thread_Mutex> guard(handle_mutex_);

  listener_ptr_ = listener;
  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::return_permissions_handle(
    ::DDS::Security::PermissionsHandle handle,
    ::DDS::Security::SecurityException& ex)
{
  if (DDS::HANDLE_NIL == handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::return_permissiosn_handle: Invalid permissions handle");
    return false;
  }

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(handle);

  if (ac_iter == local_ac_perms_.end()) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::return_permissions_handle: No matching permissions handle present");
    return false;
  }

  local_ac_perms_.erase(ac_iter);
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::return_permissions_handle local_ac_perms_ (total %B)\n"),
               local_ac_perms_.size()));
  }
  make_task(local_rp_task_)->erase(handle);
  make_task(remote_rp_task_)->erase(handle);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::return_permissions_token(
  const ::DDS::Security::PermissionsToken & token,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(token);
  ACE_UNUSED_ARG(ex);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::return_permissions_credential_token(
  const ::DDS::Security::PermissionsCredentialToken & permissions_credential_token,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(permissions_credential_token);
  ACE_UNUSED_ARG(ex);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_participant_sec_attributes(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::ParticipantSecurityAttributes & attributes,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_participant_sec_attributes: Invalid permissions handle");
    return false;
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_participant_sec_attributes: No matching permissions handle present");
    return false;
  }

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(ac_iter->second.domain_id)) {
      attributes = giter->domain_attrs;
      return true;
    }
  }

  CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_participant_sec_attributes: No matching domain in governance");
  return false;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_topic_sec_attributes(
  ::DDS::Security::PermissionsHandle permissions_handle,
  const char * topic_name,
  ::DDS::Security::TopicSecurityAttributes & attributes,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_topic_sec_attributes: Invalid permissions handle");
    return false;
  }
  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_topic_sec_attributes: Invalid topic name");
    return false;
  }

  // Extract Governance and the permissions data for the requested handle

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  ACPermsMap::iterator piter = local_ac_perms_.find(permissions_handle);

  if (piter == local_ac_perms_.end()) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_topic_sec_attributes: No matching permissions handle present");
    return false;
  }

  gov_iter begin = piter->second.gov->access_rules().begin();
  gov_iter end = piter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {

    if (giter->domains.has(piter->second.domain_id)) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (pattern_match(topic_name, tr_iter->topic_expression.c_str())) {
          attributes = tr_iter->topic_attrs;
          return true;
        }
      }
    }
  }

  CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::get_topic_sec_attributes: No matching domain/topic in governance");
  return false;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_datawriter_sec_attributes(
  ::DDS::Security::PermissionsHandle permissions_handle,
  const char * topic_name,
  const ::DDS::PartitionQosPolicy & partition,
  const ::DDS::Security::DataTagQosPolicy & data_tag,
  ::DDS::Security::EndpointSecurityAttributes & attributes,
  ::DDS::Security::SecurityException & ex)
{
  // The spec claims there is supposed to be a topic name parameter
  // to this function which is not in the IDL at this time

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_datawriter_sec_attributes: Invalid permissions handle");
    return false;
  }

  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_datawriter_sec_attributes: Invalid topic name");
    return false;
  }

  if (!get_sec_attributes(permissions_handle, topic_name, partition, data_tag, attributes, ex)) {
    return false;
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_datareader_sec_attributes(
  ::DDS::Security::PermissionsHandle permissions_handle,
  const char * topic_name,
  const ::DDS::PartitionQosPolicy & partition,
  const ::DDS::Security::DataTagQosPolicy & data_tag,
  ::DDS::Security::EndpointSecurityAttributes & attributes,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }

  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid topic name");
    return false;
  }

  if (!get_sec_attributes(permissions_handle, topic_name, partition, data_tag, attributes, ex)) {
    return false;
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::return_participant_sec_attributes(
  const ::DDS::Security::ParticipantSecurityAttributes & attributes,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(attributes);
  ACE_UNUSED_ARG(ex);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::return_datawriter_sec_attributes(
  const ::DDS::Security::EndpointSecurityAttributes & attributes,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(attributes);
  ACE_UNUSED_ARG(ex);

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::return_datareader_sec_attributes(
  const ::DDS::Security::EndpointSecurityAttributes & attributes,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(attributes);
  ACE_UNUSED_ARG(ex);

  return true;
}

::CORBA::Long AccessControlBuiltInImpl::generate_handle()
{
  ACE_Guard<ACE_Thread_Mutex> guard(gen_handle_mutex_);
  return CommonUtilities::increment_handle(next_handle_);
}

AccessControlBuiltInImpl::RevokePermissionsTask_rch&
AccessControlBuiltInImpl::make_task(RevokePermissionsTask_rch& task)
{
  if (!task) {
    task = DCPS::make_rch<RevokePermissionsTask>(TheServiceParticipant->time_source(), TheServiceParticipant->interceptor(), DCPS::ref(*this));
  }
  return task;
}

bool AccessControlBuiltInImpl::validate_date_time(
  const Permissions::Validity_t& validity,
  DDS::Security::SecurityException& ex)
{
  if (validity.not_before == 0) {
    CommonUtilities::set_security_error(ex, -1, 0,
      "AccessControlBuiltInImpl::validate_date_time: Permissions not_before time is invalid.");
    return false;
  }

  if (validity.not_after == 0) {
    CommonUtilities::set_security_error(ex, -1, 0,
      "AccessControlBuiltInImpl::validate_date_time: Permissions not_after time is invalid.");
    return false;
  }

  // Get the current time as UTC
  const time_t now = std::time(0);
  std::tm* const now_utc_tm = std::gmtime(&now);
  const time_t now_utc = std::mktime(now_utc_tm);

  if (now_utc < validity.not_before) {
    CommonUtilities::set_security_error(ex, -1, 0,
      "AccessControlBuiltInImpl::validate_date_time: Permissions grant hasn't started yet.");
    return false;
  }

  if (now_utc > validity.not_after) {
    CommonUtilities::set_security_error(ex, -1, 0,
      "AccessControlBuiltInImpl::validate_date_time: Permissions grant has expired.");
    return false;
  }

  return true;
}

CORBA::Boolean AccessControlBuiltInImpl::get_sec_attributes(::DDS::Security::PermissionsHandle permissions_handle,
                                                            const char * topic_name,
                                                            const::DDS::PartitionQosPolicy & /*partition*/,
                                                            const::DDS::Security::DataTagQosPolicy & /*data_tag*/,
                                                            ::DDS::Security::EndpointSecurityAttributes & attributes,
                                                            ::DDS::Security::SecurityException & ex)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, false);

  const ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);
  if (ac_iter == local_ac_perms_.end()) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_datawriter_sec_attributes: No matching permissions handle present");
    return false;
  }

  const gov_iter begin = ac_iter->second.gov->access_rules().begin();
  const gov_iter end = ac_iter->second.gov->access_rules().end();
  for (gov_iter giter = begin; giter != end; ++giter) {
    if (giter->domains.has(ac_iter->second.domain_id)) {
      if (std::strcmp(topic_name, "DCPSParticipantVolatileMessageSecure") == 0) {
        attributes.base.is_write_protected = false;
        attributes.base.is_read_protected = false;
        attributes.base.is_liveliness_protected = false;
        attributes.base.is_discovery_protected = false;
        attributes.is_submessage_protected = true;
        attributes.is_payload_protected = false;
        attributes.is_key_protected = false;
        return true;
      }

      if (std::strcmp(topic_name, "DCPSParticipantStatelessMessage") == 0) {
        attributes.base.is_write_protected = false;
        attributes.base.is_read_protected = false;
        attributes.base.is_liveliness_protected = false;
        attributes.base.is_discovery_protected = false;
        attributes.is_submessage_protected = false;
        attributes.is_payload_protected = false;
        attributes.is_key_protected = false;
        return true;
      }

      if (std::strcmp(topic_name, "DCPSParticipantMessageSecure") == 0) {
        attributes.base.is_write_protected = false;
        attributes.base.is_read_protected = false;
        attributes.base.is_liveliness_protected = false;
        attributes.base.is_discovery_protected = false;
        attributes.is_submessage_protected = giter->domain_attrs.is_liveliness_protected;
        attributes.is_payload_protected = false;
        attributes.is_key_protected = false;

        if (giter->domain_attrs.plugin_participant_attributes & ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED) {
          attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
        }

        if (giter->domain_attrs.plugin_participant_attributes & ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED) {
          attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED;
        }

        return true;
      }

      if (std::strcmp(topic_name, "DCPSParticipantSecure") == 0 ||
          std::strcmp(topic_name, "DCPSPublicationsSecure") == 0 ||
          std::strcmp(topic_name, "DCPSSubscriptionsSecure") == 0 ||
          std::strcmp(topic_name, "TypeLookupServiceRequestSecure") == 0 ||
          std::strcmp(topic_name, "TypeLookupServiceReplySecure") == 0) {
        attributes.base.is_write_protected = false;
        attributes.base.is_read_protected = false;
        attributes.base.is_liveliness_protected = false;
        attributes.base.is_discovery_protected = false;
        attributes.is_submessage_protected = giter->domain_attrs.is_discovery_protected;
        attributes.is_payload_protected = false;
        attributes.is_key_protected = false;

        if (giter->domain_attrs.plugin_participant_attributes & ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED) {
          attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
        }

        if (giter->domain_attrs.plugin_participant_attributes & ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED) {
          attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED;
        }

        return true;
      }

      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (pattern_match(topic_name, tr_iter->topic_expression.c_str())) {

          // Process the TopicSecurityAttributes base
          attributes.base.is_write_protected = tr_iter->topic_attrs.is_write_protected;
          attributes.base.is_read_protected = tr_iter->topic_attrs.is_read_protected;
          attributes.base.is_liveliness_protected = tr_iter->topic_attrs.is_liveliness_protected;
          attributes.base.is_discovery_protected = tr_iter->topic_attrs.is_discovery_protected;

          // Process metadata protection attributes
          if (tr_iter->metadata_protection_kind == "NONE") {
            attributes.is_submessage_protected = false;
          }
          else {
            attributes.is_submessage_protected = true;

            if (tr_iter->metadata_protection_kind == "ENCRYPT" ||
              tr_iter->metadata_protection_kind == "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") {
              attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
            }

            if (tr_iter->metadata_protection_kind == "SIGN_WITH_ORIGIN_AUTHENTICATION" ||
              tr_iter->metadata_protection_kind == "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") {
              attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED;
            }
          }

          // Process data protection attributes

          if (tr_iter->data_protection_kind == "NONE") {
            attributes.is_payload_protected = false;
            attributes.is_key_protected = false;
          }
          else if (tr_iter->data_protection_kind == "SIGN") {
            attributes.is_payload_protected = true;
            attributes.is_key_protected = false;
          }
          else if (tr_iter->data_protection_kind == "ENCRYPT") {
            attributes.is_payload_protected = true;
            attributes.is_key_protected = true;
            attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED;
          }

          return true;
        }
      }
    }
  }

  CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_sec_attributes: Invalid topic name");
  return false;
}

bool AccessControlBuiltInImpl::search_permissions(
  const char* topic_name,
  const DDS::Security::DomainId_t domain_id,
  const DDS::PartitionQosPolicy& partition,
  const Permissions::PublishSubscribe_t pub_or_sub,
  const Permissions::Grant& grant,
  DDS::Security::SecurityException& ex)
{
  for (Permissions::Rules::const_iterator rit = grant.rules.begin(); rit != grant.rules.end(); ++rit) {
    if (rit->domains.has(domain_id)) {
      for (Permissions::Actions::const_iterator ait = rit->actions.begin(); ait != rit->actions.end(); ++ait) {
        if (ait->ps_type == pub_or_sub &&
            ait->topic_matches(topic_name) &&
            ait->partitions_match(partition.name, rit->ad_type)) {
          if (rit->ad_type == Permissions::ALLOW) {
            return true;
          } else {
            return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl: DENY rule matched");
          }
        }
      }
    }
  }

  if (grant.default_permission == Permissions::ALLOW) {
    return true;
  } else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl: No matching rule for topic, default permission is DENY.");
  }
}

void AccessControlBuiltInImpl::parse_class_id(
  const std::string& class_id,
  std::string & plugin_class_name,
  int & major_version,
  int & minor_version)
{
  const std::string delimiter = ":";

  major_version = 1;
  minor_version = 0;

  size_t pos = class_id.find_last_of(delimiter);

  if ((pos > 0UL) && (pos != class_id.length() - 1)) {
    plugin_class_name = class_id.substr(0, (pos - 1));

    const std::string period = ".";

    size_t period_pos = class_id.find_last_of(period);

    if (period_pos > 0UL) {
      std::string mv_string = class_id.substr((pos + 1), (period_pos - 1));

      major_version = atoi(mv_string.c_str());

      if (period_pos != class_id.length() - 1) {
        mv_string = class_id.substr((period_pos + 1), (class_id.length() - 1));
        minor_version = atoi(mv_string.c_str());
      }
    }
  }
  else {
    plugin_class_name.clear();
  }

}

AccessControlBuiltInImpl::RevokePermissionsTask::RevokePermissionsTask(const DCPS::TimeSource& time_source,
                                                                       DCPS::ReactorInterceptor_rch interceptor,
                                                                       AccessControlBuiltInImpl& impl)
  : SporadicTask(time_source, interceptor)
  , impl_(impl)
{ }

AccessControlBuiltInImpl::RevokePermissionsTask::~RevokePermissionsTask()
{
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::RevokePermissionsTask::~RevokePermissionsTask %@ handle_to_expiration_ %B expiration_to_handle_ %B\n"),
               this,
               handle_to_expiration_.size(),
               expiration_to_handle_.size()));
  }
}

namespace {
  // Some platforms cannot schedule timers far enough into the future
  // to accommodate expiration times so the scheduling interval is
  // capped at an hour.
  const TimeDuration MAX_DURATION = TimeDuration(3600, 0);
}

void
AccessControlBuiltInImpl::RevokePermissionsTask::insert(::DDS::Security::PermissionsHandle pm_handle,
                                                        const time_t& expiration)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  if (handle_to_expiration_.count(pm_handle)) {
    return;
  }

  const time_t current_date_time = time(0);
  tm* current_time_tm = gmtime(&current_date_time);
  const time_t cur_utc_time = mktime(current_time_tm);
  const TimeDuration duration = std::min(TimeDuration(expiration - cur_utc_time), MAX_DURATION);

  const bool empty_before = expiration_to_handle_.empty();

  handle_to_expiration_[pm_handle] = expiration;

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::RevokePermissionsTask::insert handle_to_expiration_ (total %B)\n"),
               handle_to_expiration_.size()));
  }

  ExpirationToHandle::const_iterator pos =
    expiration_to_handle_.insert(ExpirationToHandle::value_type(expiration, pm_handle));

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::RevokePermissionsTask::insert expiration_to_handle_ (total %B)\n"),
               expiration_to_handle_.size()));
  }

  if (!empty_before && pos == expiration_to_handle_.begin()) {
    cancel();
  }

  if (pos == expiration_to_handle_.begin()) {
    schedule(duration);
  }
}

void
AccessControlBuiltInImpl::RevokePermissionsTask::erase(::DDS::Security::PermissionsHandle pm_handle)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  HandleToExpiration::iterator iter = handle_to_expiration_.find(pm_handle);
  if (iter == handle_to_expiration_.end()) {
    return;
  }

  const time_t expiration = iter->second;

  std::pair<ExpirationToHandle::iterator, ExpirationToHandle::iterator> er =
    expiration_to_handle_.equal_range(expiration);
  while (er.first != er.second) {
    if (er.first->second == pm_handle) {
      expiration_to_handle_.erase(er.first++);
      if (DCPS::security_debug.bookkeeping) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                   ACE_TEXT("AccessControlBuiltInImpl::RevokePermissionsTask::erase expiration_to_handle_ (total %B)\n"),
                   expiration_to_handle_.size()));
      }
    } else {
      ++er.first;
    }
  }

  handle_to_expiration_.erase(pm_handle);
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AccessControlBuiltInImpl::RevokePermissionsTask::execute handle_to_expiration_ (total %B)\n"),
               handle_to_expiration_.size()));
  }
}

void
AccessControlBuiltInImpl::RevokePermissionsTask::execute(const DCPS::MonotonicTimePoint& /*now*/)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  const time_t current_date_time = time(0);
  tm* current_time_tm = gmtime(&current_date_time);
  const time_t cur_utc_time = mktime(current_time_tm);

  for (ExpirationToHandle::iterator pos = expiration_to_handle_.begin(), limit = expiration_to_handle_.end();
         pos != limit && pos->first < cur_utc_time;) {
    const ::DDS::Security::PermissionsHandle pm_handle = pos->second;
    ACPermsMap::iterator iter = impl_.local_ac_perms_.find(pm_handle);
    if (iter == impl_.local_ac_perms_.end()) {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) AccessControlBuiltInImpl::Revoke_Permissions_Timer::execute: ")
                 ACE_TEXT("pm_handle %d not found!\n"), pm_handle));
    }
    impl_.local_ac_perms_.erase(iter);
    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AccessControlBuiltInImpl::RevokePermissionsTask::execute local_ac_perms_ (total %B)\n"),
                 impl_.local_ac_perms_.size()));
    }

    // If a listener exists, call on_revoke_permissions
    if (impl_.listener_ptr_ && !impl_.listener_ptr_->on_revoke_permissions(&impl_, pm_handle)) {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) AccessControlBuiltInImpl::Revoke_Permissions_Timer::execute: ")
                 ACE_TEXT("on_revoke_permissions failed for pm_handle %d!\n"), pm_handle));
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) AccessControlBuiltInImpl::Revoke_Permissions_Timer::execute: Completed...\n")));
    }

    handle_to_expiration_.erase(pm_handle);
    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AccessControlBuiltInImpl::RevokePermissionsTask::execute handle_to_expiration_ (total %B)\n"),
                 handle_to_expiration_.size()));
    }

    expiration_to_handle_.erase(pos++);
    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AccessControlBuiltInImpl::RevokePermissionsTask::execute expiration_to_handle_ (total %B)\n"),
                 expiration_to_handle_.size()));
    }
  }

  if (!expiration_to_handle_.empty()) {
    const TimeDuration duration = std::min(TimeDuration(expiration_to_handle_.begin()->first - cur_utc_time), MAX_DURATION);
    schedule(duration);
  }
}

SSL::SubjectName
AccessControlBuiltInImpl::get_subject_name(DDS::Security::PermissionsHandle permissions_handle) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, SSL::SubjectName());

  ACPermsMap::const_iterator pos = local_ac_perms_.find(permissions_handle);
  if (pos != local_ac_perms_.end()) {
    return pos->second.subject;
  }

  return SSL::SubjectName();
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
