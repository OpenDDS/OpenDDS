/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include "dds/DCPS/security/AccessControlBuiltInImpl.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "ace/config-macros.h"
#include "ace/OS_NS_strings.h"
#include "dds/DCPS/security/TokenWriter.h"
#include "SSL/SubjectName.h"
#include "dds/DCPS/Service_Participant.h"
#include "ace/Reactor.h"
#include "tao/debug.h"

#include "AccessControlBuiltInImpl.h"

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
typedef Permissions::PermissionGrantRules::iterator perm_grant_iter;
typedef Permissions::TopicRules::iterator perm_topic_rules_iter;
typedef Permissions::Partitions::iterator perm_partitions_iter;
typedef Permissions::TopicPsRules::iterator perm_topic_ps_rules_iter;
typedef Permissions::PartitionPsList::iterator perm_partition_ps_iter;

static const std::string PermissionsTokenClassId("DDS:Access:Permissions:1.0");
static const std::string AccessControl_Plugin_Name("DDS:Access:Permissions");
static const std::string AccessControl_Major_Version("1");
static const std::string AccessControl_Minor_Version("0");

static const std::string PermissionsCredentialTokenClassId("DDS:Access:PermissionsCredential");



AccessControlBuiltInImpl::AccessControlBuiltInImpl()
  : local_rp_timer_(*this)
  , remote_rp_timer_(*this)
  , handle_mutex_()
  , gen_handle_mutex_()
  , next_handle_(1)
  , listener_ptr_(0)
{  }

AccessControlBuiltInImpl::~AccessControlBuiltInImpl()
{
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

  if (auth_plugin->get_identity_token(id_token, identity, ex) == false) {
    return DDS::HANDLE_NIL;
  }

  LocalAccessCredentialData::shared_ptr local_access_credential_data = DCPS::make_rch<LocalAccessCredentialData>();

  if (! local_access_credential_data->load(participant_qos.property.value, ex)) {
    return DDS::HANDLE_NIL;
  }

  const SSL::Certificate& local_ca = local_access_credential_data->get_ca_cert();
  const SSL::SignedDocument& local_gov = local_access_credential_data->get_governance_doc();

  int err = local_gov.verify_signature(local_ca);
  if (err) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Governance signature not verified");
    return DDS::HANDLE_NIL;
  }

  Governance::shared_ptr governance = DCPS::make_rch<Governance>();

  err = governance->load(local_gov);
  if (err) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Invalid governance file");
    return DDS::HANDLE_NIL;
  }

  const SSL::SignedDocument& local_perm = local_access_credential_data->get_permissions_doc();
  Permissions::shared_ptr permissions = DCPS::make_rch<Permissions>();

  err = permissions->load(local_perm);
  if (err) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Invalid permission file");
    return DDS::HANDLE_NIL;
  }

  // Compare the subject name for validation

  TokenReader tr(id_token);
  const char* id_sn = tr.get_property_value("dds.cert.sn");

  OpenDDS::Security::SSL::SubjectName sn_id;

  if (!id_sn || sn_id.parse(id_sn) != 0 || !permissions->contains_subject_name(sn_id)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: No permissions subject name matches identity subject name");
    return DDS::HANDLE_NIL;
  }

  // Verify signature of permissions file
  err = local_perm.verify_signature(local_ca);
  if (err) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Permissions signature not verified");
    return DDS::HANDLE_NIL;
  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::validate_local_permissions: Permissions document verified.\n")));
    }
  }

  // Set and store the permissions credential token while we have the raw content
  DDS::Security::PermissionsCredentialToken permissions_cred_token;
  TokenWriter pctWriter(permissions_cred_token, PermissionsCredentialTokenClassId);

  pctWriter.add_property("dds.perm.cert", local_perm.get_original());

  // Set and store the permissions token
  DDS::Security::PermissionsToken permissions_token;
  TokenWriter writer(permissions_token, PermissionsTokenClassId);

  // If all checks are successful load the content into cache
  Permissions::AcPerms& perm_data = permissions->data();
  perm_data.perm_token = permissions_token;
  perm_data.perm_cred_token = permissions_cred_token;

  ::CORBA::Long perm_handle = generate_handle();

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, 0);

  AccessData cache_this;
  cache_this.identity = identity;
  cache_this.subject = sn_id;
  cache_this.domain_id = domain_id;
  cache_this.perm = permissions;
  cache_this.gov = governance;
  cache_this.local_access_credential_data = local_access_credential_data;

  local_ac_perms_.insert(std::make_pair(perm_handle, cache_this));
  local_identity_map_.insert(std::make_pair(identity, perm_handle));

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

  ACIdentityMap::iterator iter = local_identity_map_.find(local_identity_handle);

  if (iter == local_identity_map_.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: No matching local identity handle present");
    return DDS::HANDLE_NIL;
  }

  // Extract Governance and domain id data for new permissions entry
  ::DDS::Security::PermissionsHandle local_ph = iter->second;

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, 0);

  ACPermsMap::iterator piter = local_ac_perms_.find(local_ph);

  if (piter == local_ac_perms_.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: No matching local permissions handle present");
    return DDS::HANDLE_NIL;
  }

  // permissions file
  OpenDDS::Security::TokenReader remote_perm_wrapper(remote_credential_token);
  SSL::SignedDocument remote_perm_doc;

  int err = remote_perm_doc.deserialize(remote_perm_wrapper.get_bin_property_value("c.perm"));
  if (err)
  {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Failed to deserialize c.perm into signed-document");
    return DDS::HANDLE_NIL;
  }

  Permissions::shared_ptr remote_permissions = DCPS::make_rch<Permissions>();

  err = remote_permissions->load(remote_perm_doc);
  if (err)
  {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Invalid permission file");
    return DDS::HANDLE_NIL;
  }

  const LocalAccessCredentialData::shared_ptr& local_access_credential_data = piter->second.local_access_credential_data;

  // Validate the signature of the remote permissions
  const SSL::Certificate& local_ca = local_access_credential_data->get_ca_cert();
  std::string ca_subject;

  local_ca.subject_name_to_str(ca_subject);

  err = remote_perm_doc.verify_signature(local_ca);
  if (err) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: Remote permissions signature not verified");
    return DDS::HANDLE_NIL;
  }

  // The remote permissions signature is verified
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
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

  OpenDDS::Security::SSL::SubjectName sn_id_remote;

  if (remote_identity_sn.empty() || sn_id_remote.parse(remote_identity_sn) != 0 || !remote_permissions->contains_subject_name(sn_id_remote)) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_remote_permissions: "
                                        "Remote identity subject name does not match any subject name in remote permissions grants");
    return DDS::HANDLE_NIL;
  }

  // Set and store the permissions credential token while we have the raw content

  Permissions::AcPerms& perm_data = remote_permissions->data();
  perm_data.perm_token = remote_permissions_token;
  perm_data.perm_cred_token = remote_credential_token;

  ::CORBA::Long perm_handle = generate_handle();

  AccessData cache_this;
  cache_this.identity = remote_identity_handle;
  cache_this.subject = sn_id_remote;
  cache_this.domain_id = piter->second.domain_id;
  cache_this.perm = remote_permissions;
  cache_this.gov = piter->second.gov;
  cache_this.local_access_credential_data = local_access_credential_data;

  local_ac_perms_.insert(std::make_pair(perm_handle, cache_this));
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

  ::DDS::Security::DomainId_t domain_to_find = domain_id;

  gov_iter begin = piter->second.gov->access_rules().begin();
  gov_iter end = piter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if (d > 0) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (tr_iter->topic_attrs.is_read_protected == false ||
            tr_iter->topic_attrs.is_write_protected == false ) {
          return true;
        }
      }

      if (giter->domain_attrs.is_access_protected == false) return true;
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
    size_t d = giter->domain_list.count(domain_id);

    if (d > 0) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if ( ::ACE::wild_match(topic_name, tr_iter->topic_expression.c_str(), true,false)) {
          if (tr_iter->topic_attrs.is_write_protected == false ) {
            return true;
          }
        }
      }
    }
  }

  // Check the Permissions file
  TimeDuration delta_time;
  if (!validate_date_time(ac_iter, delta_time, ex)) {
    return false;
  }

  if (!search_local_permissions(topic_name, domain_id, partition, Permissions::PUBLISH, ac_iter, ex)) {
    return false;
  }

  if (!local_rp_timer_.is_scheduled()) {
    // Start timer
    if (!local_rp_timer_.start_timer(delta_time, permissions_handle)) {
      return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: Permissions timer could not be created.");
    }
  }

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
    size_t d = giter->domain_list.count(domain_id);

    if (d > 0) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if ( ::ACE::wild_match(topic_name, tr_iter->topic_expression.c_str(), true, false)) {
          if (tr_iter->topic_attrs.is_read_protected == false ) {
            return true;
          }
        }

      }

    }
  }

  // Check the Permissions file
  TimeDuration delta_time;
  if (!validate_date_time(ac_iter, delta_time, ex)) {
    return false;
  }

  if (!search_local_permissions(topic_name, domain_id, partition, Permissions::SUBSCRIBE, ac_iter, ex)) {
    return false;
  }

  if (!local_rp_timer_.is_scheduled()) {
    if (!local_rp_timer_.start_timer(delta_time, permissions_handle)) {
      return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datareader: Permissions timer could not be created.");
    }
  }

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
    size_t d = giter->domain_list.count(domain_to_find);

    if (d) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (::ACE::wild_match(topic_name, tr_iter->topic_expression.c_str(), true, false)) {
          if (tr_iter->topic_attrs.is_read_protected == false ||
              tr_iter->topic_attrs.is_write_protected == false) {
            return true;
          }
        }
      }
    }
  }

  // Check the Permissions file for grants
  TimeDuration delta_time;
  if (!validate_date_time(ac_iter, delta_time, ex)) {
    return false;
  }

  // Iterate over grant rules
  Permissions::PermissionGrantRules::iterator pgr_iter;
  for (pgr_iter = ac_iter->second.perm->data().perm_rules.begin(); pgr_iter != ac_iter->second.perm->data().perm_rules.end(); ++pgr_iter) {

    // Check to make sure our participant subject name matches the grant we're looking at
    if (pgr_iter->subject == ac_iter->second.subject) {

      // Iterate over allow / deny rules
      perm_topic_rules_iter ptr_iter;
      for (ptr_iter = pgr_iter->PermissionTopicRules.begin(); ptr_iter != pgr_iter->PermissionTopicRules.end(); ++ptr_iter) {

        // Check that our domain is listed and the permissions type is ALLOW before checking further
        size_t d = ptr_iter->domain_list.count(domain_to_find);
        if ((d > 0) && (ptr_iter->ad_type == Permissions::ALLOW)) {

          // Iterate over pub / sub rules
          perm_topic_ps_rules_iter tpsr_iter;
          for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {

            // Iterate over topics
            std::vector<std::string>::iterator tl_iter;
            for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {

              // If we have a match, we're ok to allow topic creation
              if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false))
                return true;
            }
          }
        }
      }

      // There is no matching rule for topic_name so use the value in default_permission
      if (strcmp(pgr_iter->default_permission.c_str(), "ALLOW") == 0) {
        return true;
      }
      else {
        return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_topic: No matching rule for topic, default permission is DENY.");
      }
    }
  }

  //TODO: QoS rules are not implemented

  return false;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_local_datawriter_register_instance(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::DataWriter_ptr writer,
  ::DDS::Security::DynamicData_ptr key,
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
  ::DDS::Security::DynamicData_ptr key,
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
    size_t d = giter->domain_list.count(domain_id);

    if (d > 0) {
      if (giter->domain_attrs.is_access_protected == false) return true;
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
  }
  else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Invalid remote class ID");
  }

  for (ACPermsMap::iterator local_iter = local_ac_perms_.begin(); local_iter != local_ac_perms_.end(); ++local_iter) {
    if (local_iter->second.domain_id == domain_id) {
      if (local_iter->first != permissions_handle) {
        std::string local_class_id = local_iter->second.perm->data().perm_token.class_id.in();

        if (local_class_id.length() > 0) {
          parse_class_id(local_class_id, local_plugin_class_name, local_major_ver, local_minor_ver);
        }
        else {
          return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Invalid local class ID");
        }

        break;
      }
    }
  }

  if (strcmp(local_plugin_class_name.c_str(), remote_plugin_class_name.c_str()) == 0) {
    if (local_major_ver != remote_major_ver) {
      return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Class ID major versions do not match");
    }
  }
  else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_participant: Class ID plugin class name do not match");
  }

  // Check permissions topic grants

  Permissions::PermissionGrantRules::iterator pgr_iter;

  // Check topic rules for the given domain id.

  for (pgr_iter = ac_iter->second.perm->data().perm_rules.begin(); pgr_iter != ac_iter->second.perm->data().perm_rules.end(); ++pgr_iter) {
    // Cycle through topic rules to find an allow
    perm_topic_rules_iter ptr_iter;

    for (ptr_iter = pgr_iter->PermissionTopicRules.begin(); ptr_iter != pgr_iter->PermissionTopicRules.end(); ++ptr_iter) {
      size_t z = (ptr_iter->domain_list.count(domain_id));

      if ((z > 0) && (ptr_iter->ad_type == Permissions::ALLOW)) {
        return true;
      }
    }
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
    size_t d = giter->domain_list.count(domain_id);

    if (d > 0) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (::ACE::wild_match(publication_data.base.base.topic_name, tr_iter->topic_expression.c_str(), true, false)) {
          if (tr_iter->topic_attrs.is_write_protected == false) {
            return true;
          }
        }
      }
    }
  }

  TimeDuration delta_time;
  if (!validate_date_time(ac_iter, delta_time, ex)) {
    return false;
  }

  if (!search_remote_permissions(publication_data.base.base.topic_name, domain_id, ac_iter, Permissions::PUBLISH, ex)) {
    return false;
  }

  if (!remote_rp_timer_.is_scheduled()) {
    if (!remote_rp_timer_.start_timer(delta_time, permissions_handle)) {
      return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datareader: Permissions timer could not be created.");
    }
  }

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
    size_t d = giter->domain_list.count(domain_id);

    if (d > 0) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (::ACE::wild_match(subscription_data.base.base.topic_name, tr_iter->topic_expression.c_str(), true, false)) {
          if (tr_iter->topic_attrs.is_read_protected == false) {
            return true;
          }
        }
      }

    }
  }

  TimeDuration delta_time;
  if (!validate_date_time(ac_iter, delta_time, ex)) {
    return false;
  }

  if (!search_remote_permissions(subscription_data.base.base.topic_name, domain_id, ac_iter, Permissions::SUBSCRIBE, ex)) {
    return false;
  }

  if (!remote_rp_timer_.is_scheduled()) {
    if (!remote_rp_timer_.start_timer(delta_time, permissions_handle)) {
      return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datareader: Permissions timer could not be created.");
    }
  }

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
  const std::string remote_class_id = ac_iter->second.perm->data().perm_token.class_id.in();

  std::string local_plugin_class_name,
              remote_plugin_class_name;
  int local_major_ver = 0,
      local_minor_ver,
      remote_major_ver,
      remote_minor_ver;

  if (remote_class_id.length() > 0) {
    parse_class_id(remote_class_id, remote_plugin_class_name, remote_major_ver, remote_minor_ver);
  }
  else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Invalid remote class ID");
  }

  for (ACPermsMap::iterator local_iter = local_ac_perms_.begin(); local_iter != local_ac_perms_.end(); ++local_iter) {
    if (local_iter->second.domain_id == domain_id) {
      if (local_iter->first != permissions_handle) {
        std::string local_class_id = local_iter->second.perm->data().perm_token.class_id.in();

        if (local_class_id.length() > 0) {
          parse_class_id(local_class_id, local_plugin_class_name, local_major_ver, local_minor_ver);
        }
        else {
          return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Invalid local class ID");
        }

        break;
      }
    }
  }

  if (strcmp(local_plugin_class_name.c_str(), remote_plugin_class_name.c_str()) == 0) {
    if (local_major_ver != remote_major_ver) {
      return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Class ID major versions do not match");
    }
  }
  else {
    return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: Class ID plugin class name do not match");
  }

  // Check the Governance file for allowable topic attributes

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {
    size_t d = giter->domain_list.count(domain_id);

    if (d) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (::ACE::wild_match(topic_data.name, tr_iter->topic_expression.c_str(), true, false)) {
          if (tr_iter->topic_attrs.is_read_protected == false ||
              tr_iter->topic_attrs.is_write_protected == false) {
            return true;
          }
        }
      }
    }
  }

  // Check the Permissions file for grants
  TimeDuration delta_time;
  if (!validate_date_time(ac_iter, delta_time, ex)) {
    return false;
  }

  // Iterate over grant rules
  Permissions::PermissionGrantRules::iterator pgr_iter;
  for (pgr_iter = ac_iter->second.perm->data().perm_rules.begin(); pgr_iter != ac_iter->second.perm->data().perm_rules.end(); ++pgr_iter) {

    // Check to make sure our participant subject name matches the grant we're looking at
    if (pgr_iter->subject == ac_iter->second.subject) {

      // Iterate over allow / deny rules
      perm_topic_rules_iter ptr_iter;
      for (ptr_iter = pgr_iter->PermissionTopicRules.begin(); ptr_iter != pgr_iter->PermissionTopicRules.end(); ++ptr_iter) {

        // Check that our domain is listed and the permissions type is ALLOW before checking further
        size_t d = ptr_iter->domain_list.count(domain_id);
        if ((d > 0) && (ptr_iter->ad_type == Permissions::ALLOW)) {

          // Iterate over pub / sub rules
          perm_topic_ps_rules_iter tpsr_iter;
          for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {

            // Check to make sure they can publish or subscribe to the topic
            // TODO Add support for relay permissions once relay only key exchange is supported
            if (tpsr_iter->ps_type == Permissions::PUBLISH || tpsr_iter->ps_type == Permissions::SUBSCRIBE) {

              // Iterate over topics
              std::vector<std::string>::iterator tl_iter;
              for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {

                // If we have a match, we're ok to allow topic creation
                if (::ACE::wild_match(topic_data.name, (*tl_iter).c_str(), true, false))
                  return true;
              }
            }
          }
        }
      }

      // There is no matching rule for topic_name so use the value in default_permission
      if (strcmp(pgr_iter->default_permission.c_str(), "ALLOW") == 0) {
        return true;
      }
      else {
        return CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_topic: No matching rule for topic, default permission is DENY.");
      }
    }
  }

  //TODO: QoS rules are not implemented

  return false;
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
  ::DDS::Security::DynamicData_ptr key,
  ::DDS::InstanceHandle_t instance_handle,
  ::DDS::Security::SecurityException & ex)
{
  if (DDS::HANDLE_NIL == permissions_handle ||
      DDS::HANDLE_NIL == publication_handle ||
      DDS::HANDLE_NIL == instance_handle) {
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
  ::DDS::Security::DynamicData_ptr key,
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
    permissions_token = iter->second.perm->data().perm_token;
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
    permissions_credential_token = iter->second.perm->data().perm_cred_token;
    return true;
  } else {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_permissions_credential_token: No PermissionToken found");
    return false;
  }

  return true;
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
    CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::get_participant_sec_attributes: No matching permissions handle present");
    return false;
  }

  // Check the Governance file for allowable topic attributes
  ::DDS::Security::DomainId_t domain_to_find = ac_iter->second.domain_id;

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if (d > 0) {
      attributes = giter->domain_attrs;
      return true;
    }
  }

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
    CommonUtilities::set_security_error(ex,-1, 0, "AccessControlBuiltInImpl::get_topic_sec_attributes: No matching permissions handle present");
    return false;
  }

  ::DDS::Security::DomainId_t domain_to_find = piter->second.domain_id;

  gov_iter begin = piter->second.gov->access_rules().begin();
  gov_iter end = piter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if (d > 0) {
      Governance::TopicAccessRules::iterator tr_iter;

      for (tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if (::ACE::wild_match(topic_name,tr_iter->topic_expression.c_str(), true, false)) {
          attributes = tr_iter->topic_attrs;
          return true;
        }
      }
    }
  }

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

// NOTE: This function will return the time value as UTC
// Format from DDS Security spec 1.1 is:
//   CCYY-MM-DDThh:mm:ss[Z|(+|-)hh:mm]
time_t AccessControlBuiltInImpl::convert_permissions_time(const std::string& timeString)
{
  // Check for a valid length time string, which is 19 characters (up through seconds)
  if (timeString.length() < 19) {
    return 0;
  }

  //time_t permission_time_t;
  tm permission_tm;
  std::string temp_str;

  memset(&permission_tm, 0, sizeof(tm));
  // Year
  temp_str = timeString.substr(0, 4);
  permission_tm.tm_year = (atoi(temp_str.c_str()) - 1900);
  temp_str.clear();
  // Month
  temp_str = timeString.substr(5, 2);
  permission_tm.tm_mon = (atoi(temp_str.c_str()) - 1);
  temp_str.clear();
  // Day
  temp_str = timeString.substr(8, 2);
  permission_tm.tm_mday = atoi(temp_str.c_str());
  temp_str.clear();
  // Hour
  temp_str = timeString.substr(11, 2);
  permission_tm.tm_hour = atoi(temp_str.c_str());
  temp_str.clear();
  // Minutes
  temp_str = timeString.substr(14, 2);
  permission_tm.tm_min = atoi(temp_str.c_str());
  temp_str.clear();
  // Seconds
  temp_str = timeString.substr(17, 2);
  permission_tm.tm_sec = atoi(temp_str.c_str());

  // Check if there is time zone information in the string, Z is in the 20th character
  if (timeString.length() > 20) {
    temp_str.clear();
    temp_str = timeString.substr(19, 1);

    // The only adjustments that need to be made are if the character
    // is a '+' or '-'
    if (strcmp(temp_str.c_str(), "Z") == 0) {
      //int hours_adj = 0;
      //int mins_adj = 0;

      temp_str.clear();
      temp_str = timeString.substr(20, 1);

      if (strcmp(temp_str.c_str(), "+") == 0) {
        temp_str.clear();
        temp_str = timeString.substr(21, 2);
        //hours_adj = atoi(temp_str.c_str());
        permission_tm.tm_hour -= atoi(temp_str.c_str());
        temp_str.clear();
        temp_str = timeString.substr(24, 2);
        //mins_adj = atoi(temp_str.c_str());
        permission_tm.tm_min -= atoi(temp_str.c_str());
        //permission_time_t -= (hours_adj + mins_adj);
      }
      else if (strcmp(temp_str.c_str(), "-") == 0) {
        temp_str.clear();
        temp_str = timeString.substr(21, 2);
        //hours_adj = atoi(temp_str.c_str());
        permission_tm.tm_hour += atoi(temp_str.c_str());
        temp_str.clear();
        temp_str = timeString.substr(24, 2);
        //mins_adj = atoi(temp_str.c_str());
        permission_tm.tm_min += atoi(temp_str.c_str());
        //permission_time_t += (hours_adj + mins_adj);
      }
    }

  }

  permission_tm.tm_isdst = -1;

  //return permission_time_t;
  return mktime(&permission_tm);
}

bool AccessControlBuiltInImpl::validate_date_time(
  const ACPermsMap::iterator ac_iter,
  TimeDuration& delta_time,
  DDS::Security::SecurityException& ex)
{
  time_t after_time = 0, cur_utc_time = 0;
  time_t current_date_time = time(0);

  perm_grant_iter pbegin = ac_iter->second.perm->data().perm_rules.begin();
  perm_grant_iter pend = ac_iter->second.perm->data().perm_rules.end();

  for (perm_grant_iter pm_iter = pbegin; pm_iter != pend; ++pm_iter) {
    const time_t before_time = convert_permissions_time(pm_iter->validity.not_before);

    if (before_time == 0) {
      CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: Permissions not_before time is invalid.");
      return false;
    }

    // Adjust the current time to UTC/GMT
    tm *current_time_tm = gmtime(&current_date_time);
    cur_utc_time = mktime(current_time_tm);

    if (cur_utc_time < before_time) {
      CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: Permissions grant hasn't started yet.");
      return false;
    }

    after_time = convert_permissions_time(pm_iter->validity.not_after);

    if (after_time == 0) {
      CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: Permissions not_after time is invalid.");
      return false;
    }

    if (cur_utc_time > after_time) {
      CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_create_datawriter: Permissions grant has expired.");
      return false;
    }

  }

  delta_time = TimeDuration(after_time - cur_utc_time);
  return true;
}

CORBA::Boolean AccessControlBuiltInImpl::get_sec_attributes(::DDS::Security::PermissionsHandle permissions_handle,
                                                            const char * topic_name,
                                                            const::DDS::PartitionQosPolicy & /*partition*/,
                                                            const::DDS::Security::DataTagQosPolicy & /*data_tag*/,
                                                            ::DDS::Security::EndpointSecurityAttributes & attributes,
                                                            ::DDS::Security::SecurityException & ex)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_mutex_, 1);

  ACPermsMap::iterator ac_iter = local_ac_perms_.find(permissions_handle);

  if (ac_iter == local_ac_perms_.end()) {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_datawriter_sec_attributes: No matching permissions handle present");
    return false;
  }

  ::DDS::Security::DomainId_t domain_to_find = ac_iter->second.domain_id;

  gov_iter begin = ac_iter->second.gov->access_rules().begin();
  gov_iter end = ac_iter->second.gov->access_rules().end();

  for (gov_iter giter = begin; giter != end; ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if (d > 0) {
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
          std::strcmp(topic_name, "DCPSSubscriptionsSecure") == 0) {
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
        if (::ACE::wild_match(topic_name, tr_iter->topic_expression.c_str(), true, false)) {

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

  CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::get_datawriter_sec_attributes: Invalid topic name");
  return false;
}

CORBA::Boolean AccessControlBuiltInImpl::search_local_permissions(
  const char * topic_name,
  const ::DDS::Security::DomainId_t domain_id,
  const ::DDS::PartitionQosPolicy & partition,
  const Permissions::PublishSubscribe_t pub_or_sub,
  const ACPermsMap::iterator ac_iter,
  ::DDS::Security::SecurityException & ex)
{
  std::string default_value;

  perm_grant_iter pbegin = ac_iter->second.perm->data().perm_rules.begin();
  perm_grant_iter pend = ac_iter->second.perm->data().perm_rules.end();

  for (perm_grant_iter pm_iter = pbegin; pm_iter != pend; ++pm_iter) {
    perm_grant_iter pbegin = ac_iter->second.perm->data().perm_rules.begin();
    perm_grant_iter pend = ac_iter->second.perm->data().perm_rules.end();

    for (perm_grant_iter pm_iter = pbegin; pm_iter != pend; ++pm_iter) {
      default_value = pm_iter->default_permission;

      perm_topic_rules_iter ptr_iter; // allow/deny rules
      perm_partitions_iter pp_iter;
      int matched_allow_partitions = 0;
      int matched_deny_partitions = 0;
      CORBA::ULong num_param_partitions = 0;

      for (ptr_iter = pm_iter->PermissionTopicRules.begin(); ptr_iter != pm_iter->PermissionTopicRules.end(); ++ptr_iter) {
        size_t  d = ptr_iter->domain_list.count(domain_id);

        if ((d > 0) && (ptr_iter->ad_type == Permissions::ALLOW)) {
          perm_topic_ps_rules_iter tpsr_iter;

          for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
            if (tpsr_iter->ps_type == pub_or_sub) {
              std::vector<std::string>::iterator tl_iter; // topic list

              for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
                if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) {
                  // Topic matches now check that the partitions match
                  if (partition.name.length() > 0) {
                    // First look for the ad_type & ps_type in the partitions
                    for (pp_iter = pm_iter->PermissionPartitions.begin(); pp_iter != pm_iter->PermissionPartitions.end(); pp_iter++) {
                      size_t pd = pp_iter->domain_list.count(domain_id);

                      if ((pd > 0) && (pp_iter->ad_type == Permissions::ALLOW)) {
                        perm_partition_ps_iter pps_iter;

                        for (pps_iter = pp_iter->partition_ps.begin(); pps_iter != pp_iter->partition_ps.end(); ++pps_iter) {
                          if (pps_iter->ps_type == pub_or_sub) {
                            std::vector<std::string>::iterator pl_iter; // partition list
                            num_param_partitions = static_cast<unsigned int>(pps_iter->partition_list.size());

                            for (pl_iter = pps_iter->partition_list.begin(); pl_iter != pps_iter->partition_list.end(); ++pl_iter) {
                              // Check the pl_iter value against the list of partitions in the partition parameter
                              for (CORBA::ULong i = 0; i < partition.name.length(); ++i) {
                                if (::ACE::wild_match(partition.name[i], (*pl_iter).c_str(), true, false)) {
                                  matched_allow_partitions++;
                                  break;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                  else { // No partitions to match
                    return true;
                  }

                }
              }
            }
          }
        }
        else if ((d > 0) && (ptr_iter->ad_type == Permissions::DENY)) {
          perm_topic_ps_rules_iter tpsr_iter;

          for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
//            if (tpsr_iter->ps_type == Permissions::PUBLISH) {
            if (tpsr_iter->ps_type == pub_or_sub) {
              std::vector<std::string>::iterator tl_iter; // topic list

              for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
                if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) {
                  // Topic matches now check that the partitions match
                  if (partition.name.length() > 0) {
                    // First look for the ad_type & ps_type in the partitions
                    for (pp_iter = pm_iter->PermissionPartitions.begin(); pp_iter != pm_iter->PermissionPartitions.end(); pp_iter++) {
                      size_t pd = pp_iter->domain_list.count(domain_id);

                      if ((pd > 0) && (pp_iter->ad_type == Permissions::DENY)) {
                        perm_partition_ps_iter pps_iter;

                        for (pps_iter = pp_iter->partition_ps.begin(); pps_iter != pp_iter->partition_ps.end(); ++pps_iter) {
//                          if (pps_iter->ps_type == Permissions::PUBLISH) {
                          if (pps_iter->ps_type == pub_or_sub) {
                            std::vector<std::string>::iterator pl_iter; // partition list

                            for (pl_iter = pps_iter->partition_list.begin(); pl_iter != pps_iter->partition_list.end(); ++pl_iter) {
                              // Check the pl_iter value against the list of partitions in the partition parameter
                              for (CORBA::ULong i = 0; i < partition.name.length(); ++i) {
                                if (::ACE::wild_match(partition.name[i], (*pl_iter).c_str(), true, false)) {
                                  matched_deny_partitions++;
                                  break;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                  else {
                    return false;
                  }
                }
              }
            }
          }

        } // end of DENY
      }

      // If a topic and partition match were found, return the appropriate value.
      if ((matched_allow_partitions > 0) && (matched_deny_partitions > 0)) {
        CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl: Topic is in both allow and deny.");
        return false;
      }
      else {
        if (matched_allow_partitions > 0) {
          if (num_param_partitions > partition.name.length()) {
            CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl: Requested more partitions than available in permissions file.");
            return false;
          }
          else {
            return true;
          }
        }
        else if (matched_deny_partitions > 0) {
          return false;
        }

      }

    }
  }

  // If this point in the code is reached it means that either there are no PermissionTopicRules
  // or the topic_name does not exist in the topic_list so return the value of default_permission
  if (strcmp(default_value.c_str(), "ALLOW") == 0) {
    return true;
  }
  else {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl: No matching rule for topic, default permission is DENY.");
    return false;
  }
}

CORBA::Boolean AccessControlBuiltInImpl::search_remote_permissions(
  const char * topic_name,
  const ::DDS::Security::DomainId_t domain_id,
  const ACPermsMap::iterator ac_iter,
  const Permissions::PublishSubscribe_t pub_or_sub,
  ::DDS::Security::SecurityException & ex)
{
  perm_grant_iter pm_iter;
  std::string default_value;

  for (pm_iter = ac_iter->second.perm->data().perm_rules.begin(); pm_iter != ac_iter->second.perm->data().perm_rules.end(); ++pm_iter) {
    default_value = pm_iter->default_permission;

    perm_topic_rules_iter ptr_iter; // allow/deny rules

    for (ptr_iter = pm_iter->PermissionTopicRules.begin(); ptr_iter != pm_iter->PermissionTopicRules.end(); ++ptr_iter) {
      size_t  d = ptr_iter->domain_list.count(domain_id);

      if ((d > 0) && (ptr_iter->ad_type == Permissions::ALLOW)) {
        perm_topic_ps_rules_iter tpsr_iter;

        for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
          if (tpsr_iter->ps_type == pub_or_sub) {
            std::vector<std::string>::iterator tl_iter; // topic list

            for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
              if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) {
                return true;
              }
            }
          }  // end if (tpsr_iter->ps_type)
        } // end for
      }
      else if ((d > 0) && (ptr_iter->ad_type == Permissions::DENY)) {
        perm_topic_ps_rules_iter tpsr_iter;

        for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
          if (tpsr_iter->ps_type == pub_or_sub) {
            std::vector<std::string>::iterator tl_iter; // topic list

            for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
              if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) {
                CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter: Permissions is DENY");
                return false;
              }
            }
          }
        }

      } // end of DENY
    }

  }

  // If this point in the code is reached it means that either there are no PermissionTopicRules
  // or the topic_name does not exist in the topic_list so return the value of default_permission
  if (strcmp(default_value.c_str(), "ALLOW") == 0) {
    return true;
  }
  else {
    CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::check_remote_datawriter: Topic not in Permissions, default is DENY");
    return false;
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

AccessControlBuiltInImpl::RevokePermissionsTimer::RevokePermissionsTimer(AccessControlBuiltInImpl& impl)
    : impl_(impl)
    , scheduled_(false)
    , timer_id_(0)
{ }

AccessControlBuiltInImpl::RevokePermissionsTimer::~RevokePermissionsTimer()
{
  ACE_Reactor_Timer_Interface* reactor = TheServiceParticipant->timer();

  if (scheduled_ && reactor) {
    reactor->cancel_timer(this);
    scheduled_ = false;
  }
}

bool
AccessControlBuiltInImpl::RevokePermissionsTimer::start_timer(
  const TimeDuration& length, ::DDS::Security::PermissionsHandle pm_handle)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, false);

  ::DDS::Security::PermissionsHandle *eh_params_ptr =
      new ::DDS::Security::PermissionsHandle(pm_handle);

  ACE_Reactor_Timer_Interface* reactor = TheServiceParticipant->timer();

  if (reactor) {
    timer_id_ = reactor->schedule_timer(this, eh_params_ptr, length.value());

    if (timer_id_ != -1) {
      scheduled_ = true;
      delete eh_params_ptr;
      return true;
    }

  }

  delete eh_params_ptr;
  return false;
}

int
AccessControlBuiltInImpl::RevokePermissionsTimer::handle_timeout(
  const ACE_Time_Value& /*tv*/, const void* arg)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, -1);

  const DDS::Security::PermissionsHandle* pm_handle =
    static_cast<const DDS::Security::PermissionsHandle*>(arg);

  scheduled_ = false;

  ACPermsMap::iterator iter = impl_.local_ac_perms_.find(*pm_handle);

  if (iter == impl_.local_ac_perms_.end()) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) AccessControlBuiltInImpl::Revoke_Permissions_Timer::handle_timeout: ")
      ACE_TEXT("pm_handle %d not found!\n"), *pm_handle));
    return -1;
  }

  impl_.local_ac_perms_.erase(iter);

  // If a listener exists, call on_revoke_permissions
  if (impl_.listener_ptr_ && !impl_.listener_ptr_->on_revoke_permissions(&impl_, *pm_handle)) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) AccessControlBuiltInImpl::Revoke_Permissions_Timer::handle_timeout: ")
      ACE_TEXT("on_revoke_permissions failed for pm_handle %d!\n"), *pm_handle));
    return -1;
  }

  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT(
      "(%P|%t) AccessControlBuiltInImpl::Revoke_Permissions_Timer::handle_timeout: Completed...\n")));
  }

  return 0;
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
