/*
*
*
* Distributed under the DDS License.
* See: http://www.opendds.org/license.html
*/

#include "dds/DCPS/security/AccessControlBuiltInImpl.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "ace/config-macros.h"
#include "ace/OS_NS_strings.h"
#include "dds/DCPS/security/TokenWriter.h"
#include "SSL/SubjectName.h"

#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/framework/MemBufInputSource.hpp"
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

static const std::string PermissionsTokenClassId("DDS:Access:Permissions:1.0");
static const std::string AccessControl_Plugin_Name("DDS:Access:Permissions");
static const std::string AccessControl_Major_Version("1");
static const std::string AccessControl_Minor_Version("0");

static const std::string PermissionsCredentialTokenClassId("DDS:Access:PermissionsCredential");



AccessControlBuiltInImpl::AccessControlBuiltInImpl()
  : handle_mutex_()
  , next_handle_(1)
  , local_access_control_data_()
{
}

AccessControlBuiltInImpl::~AccessControlBuiltInImpl()
{
  // - Clean up resources used by this implementation
}

::DDS::Security::PermissionsHandle AccessControlBuiltInImpl::validate_local_permissions(
  ::DDS::Security::Authentication_ptr auth_plugin,
  ::DDS::Security::IdentityHandle identity,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::DomainParticipantQos & participant_qos,
  ::DDS::Security::SecurityException & ex)
{
  if (0 == auth_plugin) {
    CommonUtilities::set_security_error(ex, -1, 0, "Null Authentication plugin");
    return DDS::HANDLE_NIL;
  }

  DDS::Security::IdentityToken id_token;
  if (DDS::HANDLE_NIL == identity || auth_plugin->get_identity_token(id_token, identity, ex) == false) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid identity");
    return DDS::HANDLE_NIL;
  }

  ACE_UNUSED_ARG(domain_id);

  const ::DDS::Security::PropertySeq& props = participant_qos.property.value;
  std::string name, value, permca_file, gov_file, perm_file;


  //TODO: These comparisons are only supporting the file: protocol type in the property value
  for (size_t i = 0; i < props.length(); ++i) {
    name = props[i].name;
    value = props[i].value;
    if (name == "dds.sec.access.permissions_ca") {
        std::string fn = extract_file_name(value);
        if (!fn.empty()) {
          if (file_exists(fn)) {
            permca_file = fn;
          } else {
            CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions_ca file property." );
          }
        }

    } else if (name == "dds.sec.access.governance") {
      std::string fn = extract_file_name(value);
        if (!fn.empty()) {
          if (file_exists(fn)) {
            gov_file = fn;
          } else {
            CommonUtilities::set_security_error(ex, -1, 0, "Invalid governance file property." );
          }
        }
    } else if (name == "dds.sec.access.permissions") {
      std::string fn = extract_file_name(value);
      if (!fn.empty()) {
        if (file_exists(fn)) {
          perm_file = fn;
        } else {
          CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions file property." );
        }
      }
    }
  }

  local_access_control_data_.load(participant_qos.property.value);


  std::string gov_content, perm_content, perm_sn;

  // Read in permissions_ca

  SSL::Certificate& local_ca = local_access_control_data_.get_ca_cert();

  // Read in governance file

  SSL::SignedDocument& local_gov = local_access_control_data_.get_governance_doc();

  // return of 0 = verified  1 = not verified
  int gov_verified = local_gov.verify_signature(local_ca);
  if (0 == gov_verified) {
    std::cout << "Governance document verified:" << gov_verified << std::endl;
  } else {
    CommonUtilities::set_security_error(ex, -1, 0, "Governance signature not verified");
    return DDS::HANDLE_NIL;
  }

  local_gov.get_content(gov_content);
  clean_smime_content(gov_content);

  ac_perms perm_set;

  // permissions file

  SSL::SignedDocument& local_perm = local_access_control_data_.get_permissions_doc();
  local_perm.get_content(perm_content);
  clean_smime_content(perm_content);

  //Extract and compare the subject name for validation

  perm_sn.assign(perm_content);
  if (extract_subject_name(perm_sn)) {
    std::cout << "permission subject name:" << perm_sn << std::endl;
  };

  TokenReader tr(id_token);
  const char* id_sn = tr.get_property_value("dds.cert.sn");

  OpenDDS::Security::SSL::SubjectName sn_id;
  OpenDDS::Security::SSL::SubjectName sn_perm;

  if (id_sn == NULL || perm_sn.empty() ||
      sn_id.parse(id_sn) != 0 ||
      sn_perm.parse(perm_sn, true) != 0 ||
      sn_id != sn_perm) {
    CommonUtilities::set_security_error(ex, -1, 0, "Permissions subject name does not match identity subject name");
    return DDS::HANDLE_NIL;
  }

  // Verify signature of permissions file

  // return of 0 = verified  1 = not verified
  int perm_verified = local_perm.verify_signature(local_ca);
  if (0 == perm_verified) {
    std::cout << "Permissions document verified:" << perm_verified << std::endl;
  } else {
    CommonUtilities::set_security_error(ex, -1, 0, "Permissions signature not verified");
    return DDS::HANDLE_NIL;
  }

  // If all checks are successful load the content into cache

  if (0 != load_governance_file(&perm_set, gov_content)) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid governance file");
    return DDS::HANDLE_NIL;
  };

  // Set and store the permissions credential token while we have the raw content

  //TODO: the SignedDocument class does not allow the retrieval of the raw file.
  // The file will have to be pulled from the file system until that is fixed.

  ::DDS::Security::PermissionsCredentialToken permissions_cred_token;
  TokenWriter pctWriter(permissions_cred_token, PermissionsCredentialTokenClassId);
  pctWriter.add_property("dds.perm.cert", get_file_contents(perm_file.c_str()).c_str());

  // Set and store the permissions token
  ::DDS::Security::PermissionsToken permissions_token;
  TokenWriter writer(permissions_token, PermissionsTokenClassId);
  writer.add_property("dds.perm_ca.sn", "MyCA Name");
  writer.add_property("dds.perm_ca.algo", "RSA-2048");

  perm_set.perm_token = permissions_token;
  perm_set.perm_cred_token = permissions_cred_token;

  if (0 != load_permissions_file(&perm_set, perm_content)) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permission file");
    return DDS::HANDLE_NIL;
  };

  // Set the domain of the participant
  perm_set.domain_id = domain_id;

  ::CORBA::Long perm_handle = generate_handle();
  local_ac_perms.insert(std::make_pair(perm_handle, perm_set));

  local_identity_map.insert(std::make_pair(identity,perm_handle));


  return perm_handle ;
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
    CommonUtilities::set_security_error(ex, -1, 0, "Null Authentication plugin");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == local_identity_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Local Identity");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == remote_identity_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Remote Identity");
    return DDS::HANDLE_NIL;
  }

  ACIdentityMap::iterator iter = local_identity_map.begin();
  iter = local_identity_map.find(local_identity_handle);

  if (iter == local_identity_map.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching local identity handle present");
    return DDS::HANDLE_NIL;
  }

  // Extract Governance and domain id data for new permissions entry
  ::DDS::Security::PermissionsHandle rph = iter->second;

  ACPermsMap::iterator piter = local_ac_perms.begin();
  piter = local_ac_perms.find(rph);

  if (piter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return DDS::HANDLE_NIL;
  }

  ac_perms perm_set;

  perm_set.domain_id = piter->second.domain_id;
  perm_set.gov_rules = piter->second.gov_rules;

  //local_access_control_data_.load(remote_credential_token.properties);


  // Read in permissions_ca
  SSL::Certificate& local_ca = local_access_control_data_.get_ca_cert();
  std::string ca_subject;

  local_ca.subject_name_to_str(ca_subject);

  // permissions file

  // SSL::SignedDocument& local_perm = local_access_control_data_.get_permissions_doc();
  // local_perm.get_content(perm_content);


  // Instead of pulling the content from a SignedDocument yet, we can strip it from the
  // c.perm parm of the AuthenticatedCredentialToken properties until SignedDocument is implemented

  std::string remote_perm_content;
  OpenDDS::Security::TokenReader remote_perm_wrapper(remote_credential_token);

  if (remote_credential_token.binary_properties.length() > 0) {
    const DDS::OctetSeq& raw = remote_perm_wrapper.get_bin_property_value("c.perm");
    remote_perm_content.assign(reinterpret_cast<const char*>(raw.get_buffer()), raw.length());
  } else {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permission file");
    return DDS::HANDLE_NIL;
  }

  // Validate the signature of the remote permissions
  std::string remote_uri_content;
  
  remote_uri_content.assign("data:");
  remote_uri_content.append(remote_perm_content);

  SSL::SignedDocument remote_signed;

  remote_signed.load(remote_uri_content);

  int sign_verified = remote_signed.verify_signature(local_ca);

  if (sign_verified == 0) {
      // The remote permissions signature is verified
      std::cout << "Remote permissions document verified:" << sign_verified << std::endl;
  }
  else {
      CommonUtilities::set_security_error(ex, -1, 0, "Remote permissions signature not verified");
      return DDS::HANDLE_NIL;
  }

  // Try to locate the payload
  if (!clean_smime_content(remote_perm_content)){
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote permission content");
    return DDS::HANDLE_NIL;
  }

  //Extract and compare the remote subject name for validation

  std::string remote_perm_sn(remote_perm_content);
  if (extract_subject_name(remote_perm_sn)) {
    std::cout << "Remote permission subject name:" << remote_perm_sn << std::endl;
  };

  TokenReader remote_credential_tr(remote_credential_token);
  const DDS::OctetSeq& cid = remote_credential_tr.get_bin_property_value("c.id");

  if (cid.length() == 0) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote credential identity");
    return DDS::HANDLE_NIL;
  }

  SSL::Certificate::unique_ptr remote_cert(new SSL::Certificate);
  remote_cert->deserialize(cid);

  std::string remote_identity_sn;
  remote_cert->subject_name_to_str(remote_identity_sn);

  OpenDDS::Security::SSL::SubjectName sn_id_remote;
  OpenDDS::Security::SSL::SubjectName sn_perm_remote;

  if (remote_identity_sn.empty() || remote_perm_sn.empty() ||
      sn_id_remote.parse(remote_identity_sn) != 0 ||
      sn_perm_remote.parse(remote_perm_sn, true) != 0 ||
      sn_id_remote != sn_perm_remote) {
    CommonUtilities::set_security_error(ex, -1, 0, "Remote permissions subject name does not match remote identity subject name");
    return DDS::HANDLE_NIL;
  }

  // Set and store the permissions credential token while we have the raw content

  //TODO: the SignedDocument class does not allow the retrieval of the raw file.
  // The file will have to be pulled from the file system until that is fixed.

  perm_set.perm_token = remote_permissions_token;
  perm_set.perm_cred_token = remote_credential_token;

  if (0 != load_permissions_file(&perm_set, remote_perm_content)) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permission file");
    return DDS::HANDLE_NIL;
  };

  ::CORBA::Long perm_handle = generate_handle();
  local_ac_perms.insert(std::make_pair(perm_handle, perm_set));

  return perm_handle;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_create_participant(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::DomainParticipantQos & qos,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(qos);
  ACE_UNUSED_ARG(ex);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
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

  // TODO: Optional checking of QoS is not completed ( see  8.4.2.9.3 )


  ACPermsMap::iterator piter = local_ac_perms.begin();
  piter = local_ac_perms.find(permissions_handle);
  if(piter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return false;
  }

  ::DDS::Security::DomainId_t domain_to_find = piter->second.domain_id;

  GovernanceAccessRules::iterator giter;

  for(giter = piter->second.gov_rules.begin(); giter != piter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if(d > 0){
      TopicAccessRules::iterator tr_iter;

      for(tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if(tr_iter->topic_attrs.is_read_protected == false ||
           tr_iter->topic_attrs.is_write_protected == false ) {
          return true;
        }
      }

      if(giter->domain_attrs.is_access_protected == false) return true;
    }
  }

  return false;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_create_datawriter(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const char * topic_name,
  const ::DDS::DataWriterQos & qos,
  const ::DDS::PartitionQosPolicy & partition,
  const ::DDS::Security::DataTags & data_tag,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(qos);
  ACE_UNUSED_ARG(data_tag);


  //TODO: Options of DataTag, QoS, and Partitions checks are not implemented (See description of Figure 23 )
  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }
  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Name");
    return false;
  }
  if (partition.name.length() == 0) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Partition");
    return false;
  }

  ACPermsMap::iterator ac_iter = local_ac_perms.begin();
  ac_iter = local_ac_perms.find(permissions_handle);

  if (ac_iter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex, -1, 0, "No matching permissions handle present");
    return false;
  }


  GovernanceAccessRules::iterator giter;

  for(giter = ac_iter->second.gov_rules.begin(); giter != ac_iter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_id);

    if(d > 0){
      TopicAccessRules::iterator tr_iter;

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

  PermissionGrantRules::iterator pm_iter;
  std::string default_value;

  for (pm_iter = ac_iter->second.perm_rules.begin(); pm_iter != ac_iter->second.perm_rules.end(); ++pm_iter) {
    std::cout<<"Checking Permissions ..." << std::endl;

    default_value = pm_iter->default_permission;

    // Check the date/time range for validity 
    time_t after_time,
           before_time;

    before_time = convert_permissions_time(pm_iter->validity.not_before);

    time_t current_date_time = time(0);

    if (current_date_time < before_time) {
      CommonUtilities::set_security_error(ex, -1, 0, "Permissions grant hasn't started yet.");
      return false;
    }

    after_time = convert_permissions_time(pm_iter->validity.not_after);

    if (current_date_time > after_time) {
      CommonUtilities::set_security_error(ex, -1, 0, "Permissions grant has expired.");
      return false;
    }

    std::list<permissions_topic_rule>::iterator ptr_iter; // allow/deny rules
    std::list<permissions_partition>::iterator pp_iter;
    int matched_allow_partitions = 0;
    int matched_deny_partitions = 0;
    CORBA::ULong num_param_partitions = 0;

    for (ptr_iter = pm_iter->PermissionTopicRules.begin(); ptr_iter != pm_iter->PermissionTopicRules.end(); ++ptr_iter) {
      size_t  d = ptr_iter->domain_list.count(domain_id);

      if ((d > 0) && (ptr_iter->ad_type == ALLOW)) {
        std::list<permission_topic_ps_rule>::iterator tpsr_iter;

        for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
          if (tpsr_iter->ps_type == PUBLISH) {
            std::vector<std::string>::iterator tl_iter; // topic list

            for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
                if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) {
                    // Topic matches now check that the partitions match

                    // First look for the ad_type & ps_type in the partitions
                    for (pp_iter = pm_iter->PermissionPartitions.begin(); pp_iter != pm_iter->PermissionPartitions.end(); pp_iter++) {
                        size_t pd = pp_iter->domain_list.count(domain_id);

                        if ((pd > 0) && (pp_iter->ad_type == ALLOW)) {
                            std::list<permission_partition_ps>::iterator pps_iter;

                            for (pps_iter = pp_iter->partition_ps.begin(); pps_iter != pp_iter->partition_ps.end(); ++pps_iter) {
                                if (pps_iter->ps_type == PUBLISH) {
                                    std::vector<std::string>::iterator pl_iter; // partition list
                                    num_param_partitions = pps_iter->partition_list.size();

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
            }
          }
        }
      }
      else if ((d > 0) && (ptr_iter->ad_type == DENY)) {
          std::list<permission_topic_ps_rule>::iterator tpsr_iter;

          for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
              if (tpsr_iter->ps_type == PUBLISH) {
                  std::vector<std::string>::iterator tl_iter; // topic list

                  for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
                      if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) {
                          // Topic matches now check that the partitions match

                          // First look for the ad_type & ps_type in the partitions
                          for (pp_iter = pm_iter->PermissionPartitions.begin(); pp_iter != pm_iter->PermissionPartitions.end(); pp_iter++) {
                              size_t pd = pp_iter->domain_list.count(domain_id);

                              if ((pd > 0) && (pp_iter->ad_type == DENY)) {
                                  std::list<permission_partition_ps>::iterator pps_iter;

                                  for (pps_iter = pp_iter->partition_ps.begin(); pps_iter != pp_iter->partition_ps.end(); ++pps_iter) {
                                      if (pps_iter->ps_type == PUBLISH) {
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
                  }
              }
          }

      } // end of DENY
    }

    // If a topic and partition match were found, return the appropriate value.
    if ((matched_allow_partitions > 0) && (matched_deny_partitions > 0)) {
        CommonUtilities::set_security_error(ex, -1, 0, "Topic is in both allow and deny.");
        return false;
    }
    else {
        if (matched_allow_partitions > 0) {
            if (num_param_partitions > partition.name.length()) {
                CommonUtilities::set_security_error(ex, -1, 0, "Requested more partitions than available in permissions file.");
                return false;
            }
            else {
                CommonUtilities::set_security_error(ex, -1, 0, "Matching rule for topic in deny_rule.");
                return true;
            }
        }
        else if (matched_deny_partitions > 0) {
            return false;
        }

    }

  }

  // If this point in the code is reached it means that either there are no PermissionTopicRules 
  // or the topic_name does not exist in the topic_list so return the value of default_permission
  if (strcmp(default_value.c_str(), "ALLOW") == 0) {
      return true;
  }
  else {
      CommonUtilities::set_security_error(ex, -1, 0, "No matching rule for topic, default permission is DENY.");
      return false;
  }
}

::CORBA::Boolean AccessControlBuiltInImpl::check_create_datareader(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const char * topic_name,
  const ::DDS::DataReaderQos & qos,
  const ::DDS::PartitionQosPolicy & partition,
  const ::DDS::Security::DataTags & data_tag,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(qos);
  ACE_UNUSED_ARG(data_tag);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }

  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Name");
    return false;
  }

  if (partition.name.length() == 0) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Partition");
      return false;
  }

  ACPermsMap::iterator ac_iter = local_ac_perms.begin();
  ac_iter = local_ac_perms.find(permissions_handle);
  if(ac_iter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex, -1, 0, "No matching permissions handle present");
    return false;
  }


  GovernanceAccessRules::iterator giter;

  for(giter = ac_iter->second.gov_rules.begin(); giter != ac_iter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_id);

    if(d > 0){
      TopicAccessRules::iterator tr_iter;

      for(tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if( ::ACE::wild_match(topic_name, tr_iter->topic_expression.c_str(), true,false)) {
          std::cout << "Found topic '"<< tr_iter->topic_expression << "'" << std::endl;
          if(tr_iter->topic_attrs.is_read_protected == false ) {
            return true;
          }
        }

      }

    }
  }

  // Check the Permissions file

  PermissionGrantRules::iterator pm_iter;
  std::string default_value;

  for (pm_iter = ac_iter->second.perm_rules.begin(); pm_iter != ac_iter->second.perm_rules.end(); ++pm_iter) {
    std::cout<<"Checking Permissions ..." << std::endl;

    default_value = pm_iter->default_permission;

    // Check the date/time range for validity 
    time_t after_time,
           before_time;

    before_time = convert_permissions_time(pm_iter->validity.not_before);

    time_t current_date_time = time(0);

    if (current_date_time < before_time) {
        CommonUtilities::set_security_error(ex, -1, 0, "Permissions grant hasn't started yet.");
        return false;
    }

    after_time = convert_permissions_time(pm_iter->validity.not_after);

    if (current_date_time > after_time) {
        CommonUtilities::set_security_error(ex, -1, 0, "Permissions grant has expired.");
        return false;
    }

    std::list<permissions_topic_rule>::iterator ptr_iter; // allow/deny rules
    std::list<permissions_partition>::iterator pp_iter;
    int matched_allow_partitions = 0;
    int matched_deny_partitions = 0;
    CORBA::ULong num_param_partitions = 0;

    for (ptr_iter = pm_iter->PermissionTopicRules.begin(); ptr_iter != pm_iter->PermissionTopicRules.end(); ++ptr_iter) {
        size_t  d = ptr_iter->domain_list.count(domain_id);

        if ((d > 0) && (ptr_iter->ad_type == ALLOW)) {
            std::list<permission_topic_ps_rule>::iterator tpsr_iter;

            for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
                if (tpsr_iter->ps_type == SUBSCRIBE) {
                    std::vector<std::string>::iterator tl_iter; // topic list

                    for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
                        if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) {

                            // Topic matches now check that the partitions match

                            // First look for the ad_type & ps_type in the partitions
                            for (pp_iter = pm_iter->PermissionPartitions.begin(); pp_iter != pm_iter->PermissionPartitions.end(); pp_iter++) {
                                size_t pd = pp_iter->domain_list.count(domain_id);

                                if ((pd > 0) && (pp_iter->ad_type == ALLOW)) {
                                    std::list<permission_partition_ps>::iterator pps_iter;

                                    for (pps_iter = pp_iter->partition_ps.begin(); pps_iter != pp_iter->partition_ps.end(); ++pps_iter) {
                                        if (pps_iter->ps_type == SUBSCRIBE) {
                                            std::vector<std::string>::iterator pl_iter; // partition list
                                            num_param_partitions = pps_iter->partition_list.size();

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
                    }
                }
            }
        }
        else if ((d > 0) && (ptr_iter->ad_type == DENY)) {
            std::list<permission_topic_ps_rule>::iterator tpsr_iter;

            for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
                if (tpsr_iter->ps_type == SUBSCRIBE) {
                    std::vector<std::string>::iterator tl_iter; // topic list

                    for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
                        if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) {
                            // Topic matches now check that the partitions match
                            // First look for the ad_type & ps_type in the partitions
                            for (pp_iter = pm_iter->PermissionPartitions.begin(); pp_iter != pm_iter->PermissionPartitions.end(); pp_iter++) {
                                size_t pd = pp_iter->domain_list.count(domain_id);

                                if ((pd > 0) && (pp_iter->ad_type == DENY)) {
                                    std::list<permission_partition_ps>::iterator pps_iter;

                                    for (pps_iter = pp_iter->partition_ps.begin(); pps_iter != pp_iter->partition_ps.end(); ++pps_iter) {
                                        if (pps_iter->ps_type == SUBSCRIBE) {
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
                    }
                }
            }

        } // end of DENY
    }

    // If a topic and partition match were found, return the appropriate value.
    if ((matched_allow_partitions > 0) && (matched_deny_partitions > 0)) {
        CommonUtilities::set_security_error(ex, -1, 0, "Topic is in both allow and deny.");
        return false;
    }
    else {
        if (matched_allow_partitions > 0) {
            if (num_param_partitions > partition.name.length()) {
                CommonUtilities::set_security_error(ex, -1, 0, "Requested more partitions than available in permissions file.");
                return false;
            }
            else {
                CommonUtilities::set_security_error(ex, -1, 0, "Matching rule for topic in deny_rule.");
                return true;
            }
        }
        else if (matched_deny_partitions > 0) {
            return false;
        }

    }
  }

  // No matching topic rule was found of topic_name so return the value in default_permission
  if (strcmp(default_value.c_str(), "ALLOW") == 0) {
      return true;
  }
  else {
      CommonUtilities::set_security_error(ex, -1, 0, "No matching rule for topic, default permission is DENY.");
      return false;
  }
}

::CORBA::Boolean AccessControlBuiltInImpl::check_create_topic(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const char * topic_name,
  const ::DDS::TopicQos & qos,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(qos);
  ACE_UNUSED_ARG(ex);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }
  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Name");
    return false;
  }

  ACPermsMap::iterator ac_iter = local_ac_perms.begin();
  ac_iter = local_ac_perms.find(permissions_handle);
  if(ac_iter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex, -1, 0, "No matching permissions handle present");
    return false;
  }

  // Check the Governance file for allowable topic attributes

  ::DDS::Security::DomainId_t domain_to_find = ac_iter->second.domain_id;

  GovernanceAccessRules::iterator giter;

  for(giter = ac_iter->second.gov_rules.begin(); giter != ac_iter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if (d) {
      TopicAccessRules::iterator tr_iter;

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

  PermissionGrantRules::iterator pm_iter;

  for (pm_iter = ac_iter->second.perm_rules.begin(); pm_iter != ac_iter->second.perm_rules.end(); ++pm_iter) {
    // Check the date/time range for validity 
    time_t after_time,
            before_time;

    before_time = convert_permissions_time(pm_iter->validity.not_before);

    time_t current_date_time = time(0);

    if (current_date_time < before_time) {
        CommonUtilities::set_security_error(ex, -1, 0, "Permissions grant hasn't started yet.");
        return false;
    }

    after_time = convert_permissions_time(pm_iter->validity.not_after);

    if (current_date_time > after_time) {
        CommonUtilities::set_security_error(ex, -1, 0, "Permissions grant has expired.");
        return false;
    }

    std::list<permissions_topic_rule>::iterator ptr_iter; // allow/deny rules

    for (ptr_iter = pm_iter->PermissionTopicRules.begin(); ptr_iter != pm_iter->PermissionTopicRules.end(); ++ptr_iter) {
      size_t  d = ptr_iter->domain_list.count(domain_to_find);

      if ((d > 0) && (ptr_iter->ad_type == ALLOW)) {
        std::list<permission_topic_ps_rule>::iterator tpsr_iter;

        for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
          std::vector<std::string>::iterator tl_iter; // topic list

          for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
            if (::ACE::wild_match(topic_name, (*tl_iter).c_str(), true, false)) 
                return true;
          }
        }
      }
    }

    // There is no matching rule for topic_name so use the value in default_permission
    if (strcmp(pm_iter->default_permission.c_str(), "ALLOW") == 0) {
      return true;
    }
    else {
      CommonUtilities::set_security_error(ex, -1, 0, "No matching rule for topic, default permission is DENY.");
      return false;
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
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }
  if (0 == writer) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Writer");
    return false;
  }
  if (0 == key) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Key");
    return false;
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
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }
  if (0 == writer) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Writer");
    return false;
  }
  if (0 == key) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Key");
    return false;
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_participant(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::Security::ParticipantBuiltinTopicDataSecure & participant_data,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(participant_data);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }


  ACPermsMap::iterator ac_iter = local_ac_perms.begin();
  ac_iter = local_ac_perms.find(permissions_handle);
  if(ac_iter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return false;
  }

  GovernanceAccessRules::iterator giter;

  for(giter = ac_iter->second.gov_rules.begin(); giter != ac_iter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_id);

    if (d > 0) {
      if (giter->domain_attrs.is_access_protected == false) return true;
    }
  }
  //TODO: Need to check the PluginClassName and MajorVersion of the local permmissions vs. remote  See Table 63 of spec

  // Check permissions topic grants

  PermissionGrantRules::iterator pgr_iter;

  // Check topic rules for the given domain id.

  for(pgr_iter = ac_iter->second.perm_rules.begin(); pgr_iter != ac_iter->second.perm_rules.end(); ++pgr_iter) {
    // Cycle through topic rules to find an allow
    std::list<permissions_topic_rule>::iterator ptr_iter;
    for (ptr_iter = pgr_iter->PermissionTopicRules.begin(); ptr_iter != pgr_iter->PermissionTopicRules.end(); ++ptr_iter) {
      size_t z = (ptr_iter->domain_list.count(domain_id));
      if ((z > 0) && (ptr_iter->ad_type == ALLOW)) {
        return true;
      }
    }
  }

  return false;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_datawriter(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::Security::PublicationBuiltinTopicDataSecure & publication_data,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(publication_data);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
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
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(subscription_data);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }

  // Default this to false for now
  relay_only = false;
  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_remote_topic(
  ::DDS::Security::PermissionsHandle permissions_handle,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::TopicBuiltinTopicData & topic_data,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(topic_data);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }

  ACPermsMap::iterator ac_iter = local_ac_perms.begin();
  ac_iter = local_ac_perms.find(permissions_handle);
  if(ac_iter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return false;
  }

  // Check the Governance file for allowable topic attributes


  GovernanceAccessRules::iterator giter;

  for(giter = ac_iter->second.gov_rules.begin(); giter != ac_iter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_id);

    if (d) {
      TopicAccessRules::iterator tr_iter;

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

  PermissionGrantRules::iterator pm_iter;


  for (pm_iter = ac_iter->second.perm_rules.begin(); pm_iter != ac_iter->second.perm_rules.end(); ++pm_iter) {
    std::cout<<"Checking Permissions ..." << std::endl;

    // Check the date/time range for validity 
    time_t after_time,
           before_time;

    before_time = convert_permissions_time(pm_iter->validity.not_before);

    time_t current_date_time = time(0);

    if (current_date_time < before_time) {
        CommonUtilities::set_security_error(ex, -1, 0, "Permissions grant hasn't started yet.");
        return false;
    }

    after_time = convert_permissions_time(pm_iter->validity.not_after);

    if (current_date_time > after_time) {
        CommonUtilities::set_security_error(ex, -1, 0, "Permissions grant has expired.");
        return false;
    }

    std::list<permissions_topic_rule>::iterator ptr_iter; // allow/deny rules

    for (ptr_iter = pm_iter->PermissionTopicRules.begin(); ptr_iter != pm_iter->PermissionTopicRules.end(); ++ptr_iter) {
      size_t  d = ptr_iter->domain_list.count(domain_id);

      if ((d > 0) && (ptr_iter->ad_type == ALLOW)) {
        std::list<permission_topic_ps_rule>::iterator tpsr_iter;

        for (tpsr_iter = ptr_iter->topic_ps_rules.begin(); tpsr_iter != ptr_iter->topic_ps_rules.end(); ++tpsr_iter) {
          if (tpsr_iter->ps_type == PUBLISH) {
            std::vector<std::string>::iterator tl_iter; // topic list

            for (tl_iter = tpsr_iter->topic_list.begin(); tl_iter != tpsr_iter->topic_list.end(); ++tl_iter) {
              if (::ACE::wild_match(topic_data.name, (*tl_iter).c_str(), true, false)) 
                  return true;
            }
          }
        }
      }
    }

    // There is no matching rule for topic_name so use the value in default_permission
    if (strcmp(pm_iter->default_permission.c_str(), "ALLOW") == 0) {
      return true;
    }
    else {
      CommonUtilities::set_security_error(ex, -1, 0, "No matching rule for topic, default permission is DENY.");
      return false;
    }
  }

  //TODO: QoS rules are not implemented


  return false;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_local_datawriter_match(
  ::DDS::Security::PermissionsHandle writer_permissions_handle,
  ::DDS::Security::PermissionsHandle reader_permissions_handle,
  const ::DDS::Security::PublicationBuiltinTopicDataSecure & publication_data,
  const ::DDS::Security::SubscriptionBuiltinTopicDataSecure & subscription_data,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(publication_data);
  ACE_UNUSED_ARG(subscription_data);

  if (DDS::HANDLE_NIL == writer_permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid writer permissions handle");
    return false;
  }
  if (DDS::HANDLE_NIL == reader_permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid reader permissions handle");
    return false;
  }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::check_local_datareader_match(
  ::DDS::Security::PermissionsHandle reader_permissions_handle,
  ::DDS::Security::PermissionsHandle writer_permissions_handle,
  const ::DDS::Security::SubscriptionBuiltinTopicDataSecure & subscription_data,
  const ::DDS::Security::PublicationBuiltinTopicDataSecure & publication_data,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(subscription_data);
  ACE_UNUSED_ARG(publication_data);

  if (DDS::HANDLE_NIL == writer_permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid writer permissions handle");
    return false;
  }
  if (DDS::HANDLE_NIL == reader_permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid reader permissions handle");
    return false;
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
  if (DDS::HANDLE_NIL == permissions_handle
    || DDS::HANDLE_NIL == publication_handle
    || DDS::HANDLE_NIL == instance_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid handle");
    return false;
  }
  if (0 == reader) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Reader pointer");
    return false;
  }
  if (0 == key) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Key");
    return false;
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
  if (DDS::HANDLE_NIL == permissions_handle
    || DDS::HANDLE_NIL == publication_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid handle");
    return false;
  }
  if (0 == reader) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Reader pointer");
    return false;
  }
  if (0 == key) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Key");
    return false;
  }
  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_permissions_token(
  ::DDS::Security::PermissionsToken & permissions_token,
  ::DDS::Security::PermissionsHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(permissions_token);

  if (DDS::HANDLE_NIL == handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }

    ACPermsMap::iterator iter = local_ac_perms.begin();
    iter = local_ac_perms.find(handle);
    if(iter != local_ac_perms.end()) {
        permissions_token = iter->second.perm_token;
        return true;
    } else {
        CommonUtilities::set_security_error(ex, -1, 0, "No PermissionToken found");
        return false;
    }
}

::CORBA::Boolean AccessControlBuiltInImpl::get_permissions_credential_token(
  ::DDS::Security::PermissionsCredentialToken & permissions_credential_token,
  ::DDS::Security::PermissionsHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(permissions_credential_token);

  if (DDS::HANDLE_NIL == handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }


    ACPermsMap::iterator iter = local_ac_perms.begin();
    iter = local_ac_perms.find(handle);

    if (iter != local_ac_perms.end()) {
        permissions_credential_token = iter->second.perm_cred_token;
        return true;
    } else {
        CommonUtilities::set_security_error(ex, -1, 0, "No PermissionToken found");
        return false;
    }

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::set_listener(
  ::DDS::Security::AccessControlListener_ptr listener,
  ::DDS::Security::SecurityException & ex)
{
  if (0 == listener) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Listener pointer");
    return false;
  }

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
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }
    // TODO: Need to add the ac_endpoints properties to the returned attributes

  ACPermsMap::iterator ac_iter = local_ac_perms.begin();
  ac_iter = local_ac_perms.find(permissions_handle);
  if(ac_iter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return false;
  }

  // Check the Governance file for allowable topic attributes
  ::DDS::Security::DomainId_t domain_to_find = ac_iter->second.domain_id;

  GovernanceAccessRules::iterator giter;

  for(giter = ac_iter->second.gov_rules.begin(); giter != ac_iter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if (d > 0) {
      attributes.allow_unauthenticated_participants = giter->domain_attrs.allow_unauthenticated_participants;
      attributes.is_access_protected = giter->domain_attrs.is_access_protected;
      attributes.is_rtps_protected = giter->domain_attrs.is_rtps_protected;
      attributes.is_discovery_protected = giter->domain_attrs.is_discovery_protected;
      attributes.is_liveliness_protected = giter->domain_attrs.is_liveliness_protected;
      attributes.plugin_participant_attributes = giter->domain_attrs.plugin_participant_attributes;
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
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }
  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid topic name");
    return false;
  }

  // Extract Governance and the permissions data for the requested handle

  ACPermsMap::iterator piter = local_ac_perms.begin();
  piter = local_ac_perms.find(permissions_handle);
  if(piter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return false;
  }

  ::DDS::Security::DomainId_t domain_to_find = piter->second.domain_id;

  GovernanceAccessRules::iterator giter;

  for(giter = piter->second.gov_rules.begin(); giter != piter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if(d > 0){
      TopicAccessRules::iterator tr_iter;

      for(tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if(::ACE::wild_match(topic_name,tr_iter->topic_expression.c_str(),true,false)) {
//          std::cout << "Topic passed in:" << topic_name << " topic found: " << tr_iter->topic_expression << std::endl;
          attributes.is_read_protected = tr_iter->topic_attrs.is_read_protected;
          attributes.is_write_protected = tr_iter->topic_attrs.is_write_protected;
          attributes.is_discovery_protected = tr_iter->topic_attrs.is_discovery_protected;
          attributes.is_liveliness_protected = tr_iter->topic_attrs.is_liveliness_protected;

//          std::cout << "Attributes:" << std::endl;
//          std::cout << "is_read_protected: " << attributes.is_read_protected << "  " << tr_iter->topic_attrs.is_read_protected << std::endl;
//          std::cout << "is_write_protected: " << attributes.is_write_protected << "  " << tr_iter->topic_attrs.is_write_protected << std::endl;
//          std::cout << "is_discover_protected: " << attributes.is_discovery_protected << "  " << tr_iter->topic_attrs.is_discovery_protected << std::endl;
//          std::cout << "is_liveliness_protected: " << attributes.is_liveliness_protected<< "  " << tr_iter->topic_attrs.is_liveliness_protected << std::endl;

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
  ACE_UNUSED_ARG(partition);
  ACE_UNUSED_ARG(data_tag);

  // The spec claims there is supposed to be a topic name parameter
  // to this function which is not in the IDL at this time

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }

  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid topic name");
    return false;
  }

  ACPermsMap::iterator ac_iter = local_ac_perms.begin();
  ac_iter = local_ac_perms.find(permissions_handle);
  if(ac_iter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return false;
  }


  ::DDS::Security::DomainId_t domain_to_find = ac_iter->second.domain_id;

  GovernanceAccessRules::iterator giter;

  for(giter = ac_iter->second.gov_rules.begin(); giter != ac_iter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if(d > 0){
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
        attributes.base.is_liveliness_protected = giter->domain_attrs.is_liveliness_protected;
        attributes.base.is_discovery_protected = false;
        attributes.is_submessage_protected = true;
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
      if (std::strcmp(topic_name, "DCPSParticipantSecure") == 0 || std::strcmp(topic_name, "DCPSPublicationsSecure") == 0 || std::strcmp(topic_name, "DCPSSubscriptionsSecure") == 0) {
        attributes.base.is_write_protected = false;
        attributes.base.is_read_protected = false;
        attributes.base.is_liveliness_protected = giter->domain_attrs.is_discovery_protected;
        attributes.base.is_discovery_protected = false;
        attributes.is_submessage_protected = true;
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
      TopicAccessRules::iterator tr_iter;

      for(tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if( ::ACE::wild_match(topic_name, tr_iter->topic_expression.c_str(), true,false)) {

          // Process the TopicSecurityAttributes base
          attributes.base.is_write_protected = tr_iter->topic_attrs.is_write_protected;
          attributes.base.is_read_protected = tr_iter->topic_attrs.is_read_protected;
          attributes.base.is_liveliness_protected = tr_iter->topic_attrs.is_liveliness_protected;
          attributes.base.is_discovery_protected = tr_iter->topic_attrs.is_discovery_protected;

          // Process metadata protection attributes
          if (tr_iter->metadata_protection_kind == "NONE") {
            attributes.is_submessage_protected = false;
          } else {
            attributes.is_submessage_protected = true;
            if ( tr_iter->metadata_protection_kind == "ENCRYPT" ||
                    tr_iter->metadata_protection_kind == "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") {
              attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
            }

            if ( tr_iter->metadata_protection_kind == "SIGN_WITH_ORIGIN_AUTHENTICATION" ||
                 tr_iter->metadata_protection_kind == "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") {
              attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED;
            }
          }

          // Process data protection attributes

          if (tr_iter->data_protection_kind == "NONE") {
            attributes.is_payload_protected = false;
            attributes.is_key_protected = false;

          } else if (tr_iter->data_protection_kind == "SIGN") {
              attributes.is_payload_protected = true;
              attributes.is_key_protected = false;
            } else if ( tr_iter->data_protection_kind == "ENCRYPT") {
                attributes.is_payload_protected = true;
                attributes.is_key_protected = true;
                attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED;
              }

            return true;
        }

      }

    }
  }

  CommonUtilities::set_security_error(ex, -1, 0, "Invalid topic name");
  return false;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_datareader_sec_attributes(
  ::DDS::Security::PermissionsHandle permissions_handle,
  const char * topic_name,
  const ::DDS::PartitionQosPolicy & partition,
  const ::DDS::Security::DataTagQosPolicy & data_tag,
  ::DDS::Security::EndpointSecurityAttributes & attributes,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(partition);
  ACE_UNUSED_ARG(data_tag);

  ACPermsMap::iterator ac_iter = local_ac_perms.begin();
  ac_iter = local_ac_perms.find(permissions_handle);
  if(ac_iter == local_ac_perms.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return false;
  }


  ::DDS::Security::DomainId_t domain_to_find = ac_iter->second.domain_id;

  GovernanceAccessRules::iterator giter;

  for(giter = ac_iter->second.gov_rules.begin(); giter != ac_iter->second.gov_rules.end(); ++giter) {
    size_t d = giter->domain_list.count(domain_to_find);

    if(d > 0){
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
        attributes.base.is_liveliness_protected = giter->domain_attrs.is_liveliness_protected;
        attributes.base.is_discovery_protected = false;
        attributes.is_submessage_protected = true;
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
      if (std::strcmp(topic_name, "DCPSParticipantSecure") == 0 || std::strcmp(topic_name, "DCPSPublicationsSecure") == 0 || std::strcmp(topic_name, "DCPSSubscriptionsSecure") == 0) {
        attributes.base.is_write_protected = false;
        attributes.base.is_read_protected = false;
        attributes.base.is_liveliness_protected = giter->domain_attrs.is_discovery_protected;
        attributes.base.is_discovery_protected = false;
        attributes.is_submessage_protected = true;
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
      TopicAccessRules::iterator tr_iter;

      for(tr_iter = giter->topic_rules.begin(); tr_iter != giter->topic_rules.end(); ++tr_iter) {
        if( ::ACE::wild_match(topic_name, tr_iter->topic_expression.c_str(), true,false)) {

          // Process the TopicSecurityAttributes base
          attributes.base.is_write_protected = tr_iter->topic_attrs.is_write_protected;
          attributes.base.is_read_protected = tr_iter->topic_attrs.is_read_protected;
          attributes.base.is_liveliness_protected = tr_iter->topic_attrs.is_liveliness_protected;
          attributes.base.is_discovery_protected = tr_iter->topic_attrs.is_discovery_protected;

          // Process metadata protection attributes
          if (tr_iter->metadata_protection_kind == "NONE") {
            attributes.is_submessage_protected = false;
          } else {
            attributes.is_submessage_protected = true;
            if ( tr_iter->metadata_protection_kind == "ENCRYPT" ||
                 tr_iter->metadata_protection_kind == "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") {
              attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
            }

            if ( tr_iter->metadata_protection_kind == "SIGN_WITH_ORIGIN_AUTHENTICATION" ||
                 tr_iter->metadata_protection_kind == "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") {
              attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED;
            }
          }

          // Process data protection attributes

          if (tr_iter->data_protection_kind == "NONE") {
            attributes.is_payload_protected = false;
            attributes.is_key_protected = false;

          } else if (tr_iter->data_protection_kind == "SIGN") {
            attributes.is_payload_protected = true;
            attributes.is_key_protected = false;
          } else if ( tr_iter->data_protection_kind == "ENCRYPT") {
            attributes.is_payload_protected = true;
            attributes.is_key_protected = true;
            attributes.plugin_endpoint_attributes |= ::DDS::Security::PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED;
          }

          return true;
        }

      }

    }
  }

  CommonUtilities::set_security_error(ex, -1, 0, "Invalid topic name");
  return false;
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
  ACE_Guard<ACE_Thread_Mutex> guard(handle_mutex_);
  ::CORBA::Long new_handle = next_handle_++;

  if (new_handle == DDS::HANDLE_NIL) {
    new_handle = next_handle_++;
  }
  return new_handle;
}


::CORBA::Boolean AccessControlBuiltInImpl::extract_subject_name(std::string& sn_) {

  std::string start_str("<subject_name>") , end_str("</subject_name>");

  size_t found_begin = sn_.find(start_str);

  if (found_begin != std::string::npos) {
    sn_.erase(0, found_begin + start_str.length());
    const char* t = " \t\n\r\f\v";
    sn_.erase(0, sn_.find_first_not_of(t));
  } else {
    return false;
  }

  size_t found_end = sn_.find(end_str);

  if (found_end != std::string::npos) {
    sn_.erase(found_end);
  } else {
    return false;
  }

  return true;
}

time_t AccessControlBuiltInImpl::convert_permissions_time(std::string timeString)
{
    tm permission_tm;
    std::string temp_str;

    memset(&permission_tm, 0, sizeof(tm));
    temp_str.clear();

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

    return mktime(&permission_tm);
}

//char * AccessControlBuiltInImpl::strptime(const char * s, const char * f, tm * tm)
//{
//    // Isn't the C++ standard lib nice? std::get_time is defined such that its
//    // format parameters are the exact same as strptime. Of course, we have to
//    // create a string stream first, and imbue it with the current C locale, and
//    // we also have to make sure we return the right things if it fails, or
//    // if it succeeds, but this is still far simpler an implementation than any
//    // of the versions in any of the C standard libraries.
//    std::istringstream input(s);
//    input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
//    input >> std::get_time(tm, f);
//    if (input.fail()) {
//        return nullptr;
//    }
//    return (char*)(s + input.tellg());
//}

::CORBA::Long AccessControlBuiltInImpl::load_governance_file(ac_perms * ac_perms_holder , std::string g_content)
{

  static const char* gMemBufId = "gov buffer id";

  try
  {
    xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
  }
  catch( xercesc::XMLException& e )
  {
    char* message = xercesc::XMLString::transcode( e.getMessage() );
    std::cerr << "XML toolkit initialization error: " << message << std::endl;
    xercesc::XMLString::release( &message );
    // throw exception here to return ERROR_XERCES_INIT
  }

    xercesc::XercesDOMParser* parser = new xercesc::XercesDOMParser();
    parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
    parser->setDoNamespaces(true);    // optional
    parser->setCreateCommentNodes(false);

    xercesc::ErrorHandler* errHandler = (xercesc::ErrorHandler*) new xercesc::HandlerBase();
    parser->setErrorHandler(errHandler);


    // buffer for parsing

    xercesc::MemBufInputSource contentbuf((const XMLByte*)g_content.c_str(),
                                          g_content.size(),
                                          gMemBufId);

    try {
        parser->parse(contentbuf);
    }
    catch (const xercesc::XMLException& toCatch) {
        char* message = xercesc::XMLString::transcode(toCatch.getMessage());
        std::cout << "Exception message is: \n"
             << message << "\n";
      xercesc::XMLString::release(&message);
        return -1;
    }
    catch (const xercesc::DOMException& toCatch) {
        char* message = xercesc::XMLString::transcode(toCatch.msg);
        std::cout << "Exception message is: \n"
             << message << "\n";
        xercesc::XMLString::release(&message);
        return -1;
    }
    catch (...) {
        std::cout << "Unexpected Governance XML Parser Exception \n" ;
        return -1;
    }


  // Successfully parsed the governance file

   xercesc::DOMDocument* xmlDoc = parser->getDocument();

   xercesc::DOMElement* elementRoot = xmlDoc->getDocumentElement();
  if( !elementRoot ) throw(std::runtime_error( "empty XML document" ));

   // Find the domain rules
   xercesc::DOMNodeList * domainRules = xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("domain_rule"));


   for (XMLSize_t r=0; r < domainRules->getLength(); r++) {
     domain_rule rule_holder_;
     rule_holder_.domain_attrs.plugin_participant_attributes = 0;

       // Pull out domain ids used in the rule. We are NOT supporting ranges at this time
       xercesc::DOMNodeList * ruleNodes = domainRules->item(r)->getChildNodes();

       for (XMLSize_t rn = 0; rn < ruleNodes->getLength(); rn++) {
           char * dn_tag = xercesc::XMLString::transcode(ruleNodes->item(rn)->getNodeName());

           if ( strcmp("domains", dn_tag) == 0 ) {
               xercesc::DOMNodeList * domainIdNodes = ruleNodes->item(rn)->getChildNodes();

               for (XMLSize_t did = 0; did < domainIdNodes->getLength(); did++) {
                 if (strcmp("id" , xercesc::XMLString::transcode(domainIdNodes->item(did)->getNodeName())) == 0) {
                   rule_holder_.domain_list.insert(atoi(xercesc::XMLString::transcode(domainIdNodes->item(did)->getTextContent())));
                 }
                 else if (strcmp("id_range", xercesc::XMLString::transcode(domainIdNodes->item(did)->getNodeName())) == 0) {
                     int min_value = 0;
                     int max_value = 0;
                     xercesc::DOMNodeList * domRangeIdNodes = domainIdNodes->item(did)->getChildNodes();

                     for (XMLSize_t drid = 0; drid < domRangeIdNodes->getLength(); drid++) {
                         if (strcmp("min", xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getNodeName())) == 0) {
                             min_value = atoi(xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getTextContent()));
                         }
                         else if (strcmp("max", xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getNodeName())) == 0) {
                             max_value = atoi(xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getTextContent()));

                             if ((min_value == 0) || (min_value > max_value)) {
                                 std::cout << "Governance XML Domain Range invalid! \n";
                                 return -1;
                             }

                             for (int i = min_value; i <= max_value; i++) {
                                 rule_holder_.domain_list.insert(i);
                             }
                         }
                     }
                 }
               }

           }
       }

     // Process allow_unauthenticated_participants
     xercesc::DOMNodeList * allow_unauthenticated_participants_ =
              xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("allow_unauthenticated_participants"));
     char * attr_aup = xercesc::XMLString::transcode(allow_unauthenticated_participants_->item(0)->getTextContent());
     rule_holder_.domain_attrs.allow_unauthenticated_participants = (ACE_OS::strcasecmp(attr_aup,"false") == 0 ? false : true);


     // Process enable_join_access_control
     xercesc::DOMNodeList * enable_join_access_control_ =
             xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("enable_join_access_control"));
     char * attr_ejac = xercesc::XMLString::transcode(enable_join_access_control_->item(0)->getTextContent());
     rule_holder_.domain_attrs.is_access_protected = ACE_OS::strcasecmp(attr_ejac, "false") == 0 ? false : true;


     // Process discovery_protection_kind
     xercesc::DOMNodeList * discovery_protection_kind_ =
             xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("discovery_protection_kind"));
     char * attr_dpk = xercesc::XMLString::transcode(discovery_protection_kind_->item(0)->getTextContent());
     if (ACE_OS::strcasecmp(attr_dpk, "NONE") == 0) {
       rule_holder_.domain_attrs.is_discovery_protected = false;
     } else if (ACE_OS::strcasecmp(attr_dpk, "SIGN") == 0) {
       rule_holder_.domain_attrs.is_discovery_protected = true;
     } else if (ACE_OS::strcasecmp(attr_dpk, "ENCRYPT") == 0) {
       rule_holder_.domain_attrs.is_discovery_protected = true;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED;
     } else if (ACE_OS::strcasecmp(attr_dpk, "SIGN_WITH_ORIGIN_AUTHENTICATION") == 0) {
       rule_holder_.domain_attrs.is_discovery_protected = true;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED;
     } else if (ACE_OS::strcasecmp(attr_dpk, "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") == 0) {
       rule_holder_.domain_attrs.is_discovery_protected = true;
         rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED;
         rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED;
     }

     // Process liveliness_protection_kind
     xercesc::DOMNodeList * liveliness_protection_kind_ =
             xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("liveliness_protection_kind"));
     char * attr_lpk = xercesc::XMLString::transcode(liveliness_protection_kind_->item(0)->getTextContent());
     if (ACE_OS::strcasecmp(attr_lpk, "NONE") == 0) {
       rule_holder_.domain_attrs.is_liveliness_protected = false;
     } else if (ACE_OS::strcasecmp(attr_dpk, "SIGN") == 0) {
       rule_holder_.domain_attrs.is_liveliness_protected = true;
     } else if (ACE_OS::strcasecmp(attr_dpk, "ENCRYPT") == 0) {
       rule_holder_.domain_attrs.is_liveliness_protected = true;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED;
     } else if (ACE_OS::strcasecmp(attr_dpk, "SIGN_WITH_ORIGIN_AUTHENTICATION") == 0) {
       rule_holder_.domain_attrs.is_liveliness_protected = true;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED;
     } else if (ACE_OS::strcasecmp(attr_dpk, "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") == 0) {
       rule_holder_.domain_attrs.is_liveliness_protected = true;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED;
     }

     // Process rtps_protection_kind
     xercesc::DOMNodeList * rtps_protection_kind_ =
             xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("rtps_protection_kind"));
     char * attr_rpk = xercesc::XMLString::transcode(rtps_protection_kind_->item(0)->getTextContent());
     if (ACE_OS::strcasecmp(attr_rpk, "NONE") == 0) {
       rule_holder_.domain_attrs.is_rtps_protected = false;
     } else if (ACE_OS::strcasecmp(attr_dpk, "SIGN") == 0) {
       rule_holder_.domain_attrs.is_rtps_protected = true;
     } else if (ACE_OS::strcasecmp(attr_dpk, "ENCRYPT") == 0) {
       rule_holder_.domain_attrs.is_rtps_protected = true;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED;
     } else if (ACE_OS::strcasecmp(attr_dpk, "SIGN_WITH_ORIGIN_AUTHENTICATION") == 0) {
       rule_holder_.domain_attrs.is_rtps_protected = true;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED;
     } else if (ACE_OS::strcasecmp(attr_dpk, "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") == 0) {
       rule_holder_.domain_attrs.is_rtps_protected = true;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED;
       rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED;
     }

     rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;


     // Process topic rules

       xercesc::DOMNodeList * topic_rules = xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("topic_rule"));

       for( XMLSize_t tr = 0; tr<topic_rules->getLength(); tr++) {
         xercesc::DOMNodeList * topic_rule_nodes = topic_rules->item(tr)->getChildNodes();
         TopicAccessRule t_rules;
         for (XMLSize_t trn = 0; trn<topic_rule_nodes->getLength(); trn++) {
           char * tr_tag = xercesc::XMLString::transcode(topic_rule_nodes->item(trn)->getNodeName());
           char * tr_val = xercesc::XMLString::transcode(topic_rule_nodes->item(trn)->getTextContent());
             if (strcmp(tr_tag,"topic_expression") == 0){
               t_rules.topic_expression = tr_val;
             } else if(strcmp(tr_tag, "enable_discovery_protection") == 0) {
               t_rules.topic_attrs.is_discovery_protected = ACE_OS::strcasecmp(tr_val,"false") == 0 ? false : true;
             } else if(strcmp(tr_tag, "enable_liveliness_protection") == 0) {
               t_rules.topic_attrs.is_liveliness_protected = ACE_OS::strcasecmp(tr_val,"false") == 0 ? false : true;
             } else if(strcmp(tr_tag, "enable_read_access_control") == 0) {
               t_rules.topic_attrs.is_read_protected = ACE_OS::strcasecmp(tr_val,"false") == 0 ? false : true;
             } else if(strcmp(tr_tag, "enable_write_access_control") == 0) {
               t_rules.topic_attrs.is_write_protected = ACE_OS::strcasecmp(tr_val,"false")== 0 ? false : true;
             } else if(strcmp(tr_tag, "metadata_protection_kind") == 0) {
               t_rules.metadata_protection_kind.assign(tr_val);
             } else if(strcmp(tr_tag, "data_protection_kind") == 0) {
               t_rules.data_protection_kind.assign(tr_val);
             }
         }
         rule_holder_.topic_rules.push_back(t_rules);
       }

       ac_perms_holder->gov_rules.push_back( rule_holder_);

   } // domain_rule


    delete parser;
    delete errHandler;


  return 0;
}


::CORBA::Long AccessControlBuiltInImpl::load_permissions_file(ac_perms * ac_perms_holder, std::string p_content) {

  static const char* gMemBufId = "gov buffer id";

  try
  {
    xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
  }
  catch( xercesc::XMLException& e )
  {
    char* message = xercesc::XMLString::transcode( e.getMessage() );
    std::cerr << "XML toolkit initialization error: " << message << std::endl;
    xercesc::XMLString::release( &message );
    // throw exception here to return ERROR_XERCES_INIT
  }

  xercesc::XercesDOMParser* parser = new xercesc::XercesDOMParser();
  parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);    // optional
  parser->setCreateCommentNodes(false);

  xercesc::ErrorHandler* errHandler = (xercesc::ErrorHandler*) new xercesc::HandlerBase();
  parser->setErrorHandler(errHandler);

    xercesc::MemBufInputSource contentbuf((const XMLByte*)p_content.c_str(),
                                          p_content.size(),
                                          gMemBufId);

  try {
    parser->parse(contentbuf);
  }
  catch (const xercesc::XMLException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.getMessage());
    std::cout << "Exception message is: \n"
              << message << "\n";
    xercesc::XMLString::release(&message);
    return -1;
  }
  catch (const xercesc::DOMException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.msg);
    std::cout << "Exception message is: \n"
              << message << "\n";
    xercesc::XMLString::release(&message);
    return -1;
  }
  catch (...) {
    std::cout << "Unexpected Permissions XML Parser Exception \n" ;
    return -1;
  }


  // Successfully parsed the permissions file

  xercesc::DOMDocument* xmlDoc = parser->getDocument();

  xercesc::DOMElement* elementRoot = xmlDoc->getDocumentElement();
  if( !elementRoot ) throw(std::runtime_error( "empty XML document" ));


  //TODO:  WARNING - this implementation only supports 1 permissions/grant set
 // Different from governance from here forward
  // Find the validity rules
  xercesc::DOMNodeList * grantRules = xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("grant"));

    PermissionGrantRules grant_rules_list_holder_;

  for (XMLSize_t r = 0; r < grantRules->getLength(); r++) {
    permission_grant_rule rule_holder_;

    // Pull out the grant name for this grant
    xercesc::DOMNamedNodeMap * rattrs = grantRules->item(r)->getAttributes();
    rule_holder_.grant_name = xercesc::XMLString::transcode(rattrs->item(0)->getTextContent());

    // Pull out subject name, validity, and default
    xercesc::DOMNodeList * grantNodes = grantRules->item(r)->getChildNodes();

    for ( XMLSize_t gn = 0; gn < grantNodes->getLength(); gn++) {
        char *g_tag = xercesc::XMLString::transcode(grantNodes->item(gn)->getNodeName());

        if (strcmp(g_tag, "subject_name") == 0) {
            rule_holder_.subject = xercesc::XMLString::transcode(grantNodes->item(gn)->getTextContent());
        } else if (strcmp(g_tag, "validity") == 0) {
            //Validity_t gn_validity;
            xercesc::DOMNodeList *validityNodes = grantNodes->item(gn)->getChildNodes();

            for (XMLSize_t vn = 0; vn < validityNodes->getLength(); vn++) {
                char *v_tag = xercesc::XMLString::transcode((validityNodes->item(vn)->getNodeName()));

                if (strcmp(v_tag, "not_before") == 0) {
                    rule_holder_.validity.not_before = xercesc::XMLString::transcode(
                            (validityNodes->item(vn)->getTextContent()));
                } else if (strcmp(v_tag, "not_after") == 0) {
                    rule_holder_.validity.not_after = xercesc::XMLString::transcode(
                            (validityNodes->item(vn)->getTextContent()));
                }
            }
        } else if (strcmp(g_tag, "default") == 0) {
          rule_holder_.default_permission = xercesc::XMLString::transcode(grantNodes->item(gn)->getTextContent());
        }
    }
    // Pull out allow/deny rules
    xercesc::DOMNodeList * adGrantNodes = grantRules->item(r)->getChildNodes();

    for (XMLSize_t gn = 0; gn < adGrantNodes->getLength(); gn++) {
      char *g_tag = xercesc::XMLString::transcode(adGrantNodes->item(gn)->getNodeName());

      if (strcmp(g_tag, "allow_rule") == 0 || strcmp(g_tag, "deny_rule") == 0) {
        permissions_topic_rule ptr_holder_;
        permissions_partition pp_holder_;

        ptr_holder_.ad_type = (strcmp(g_tag,"allow_rule") ==  0 ? ALLOW : DENY);
        pp_holder_.ad_type = (strcmp(g_tag, "allow_rule") == 0 ? ALLOW : DENY);

        xercesc::DOMNodeList * adNodeChildren = adGrantNodes->item(gn)->getChildNodes();

        for (XMLSize_t anc = 0; anc < adNodeChildren->getLength(); anc++) {
          char *anc_tag = xercesc::XMLString::transcode(adNodeChildren->item(anc)->getNodeName());

          if (strcmp(anc_tag, "domains") == 0) {   //domain list
            xercesc::DOMNodeList * domainIdNodes = adNodeChildren->item(anc)->getChildNodes();

            for (XMLSize_t did = 0; did < domainIdNodes->getLength(); did++) {
              if (strcmp("id" , xercesc::XMLString::transcode(domainIdNodes->item(did)->getNodeName())) == 0) {
                ptr_holder_.domain_list.insert(atoi(xercesc::XMLString::transcode(domainIdNodes->item(did)->getTextContent())));
                pp_holder_.domain_list.insert(atoi(xercesc::XMLString::transcode(domainIdNodes->item(did)->getTextContent())));
              }
              else if (strcmp("id_range", xercesc::XMLString::transcode(domainIdNodes->item(did)->getNodeName())) == 0) {
                  int min_value = 0;
                  int max_value = 0;
                  xercesc::DOMNodeList * domRangeIdNodes = domainIdNodes->item(did)->getChildNodes();

                  for (XMLSize_t drid = 0; drid < domRangeIdNodes->getLength(); drid++) {
                      if (strcmp("min", xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getNodeName())) == 0) {
                           min_value = atoi(xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getTextContent()));
                      }
                      else if (strcmp("max", xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getNodeName())) == 0) {
                          max_value = atoi(xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getTextContent()));

                          if ((min_value == 0) || (min_value > max_value)) {
                              std::cout << "Permission XML Domain Range invalid! \n";
                              return -1;
                          }

                          for (int i = min_value; i <= max_value; i++) {
                              ptr_holder_.domain_list.insert(i);
                              pp_holder_.domain_list.insert(i);
                          }
                      }
                  }
              }
            }

          } else if (ACE_OS::strcasecmp(anc_tag, "publish") == 0 || ACE_OS::strcasecmp(anc_tag, "subscribe") == 0) {   // pub sub nodes
            permission_topic_ps_rule anc_ps_rule_holder_;
            permission_partition_ps anc_ps_partition_holder_;

            anc_ps_rule_holder_.ps_type = (ACE_OS::strcasecmp(anc_tag,"publish") ==  0 ? PUBLISH : SUBSCRIBE);
            anc_ps_partition_holder_.ps_type = (ACE_OS::strcasecmp(anc_tag, "publish") == 0 ? PUBLISH : SUBSCRIBE);
            xercesc::DOMNodeList * topicListNodes = adNodeChildren->item(anc)->getChildNodes();

            for (XMLSize_t tln = 0; tln < topicListNodes->getLength(); tln++) {
              if (strcmp("topics" , xercesc::XMLString::transcode(topicListNodes->item(tln)->getNodeName())) == 0) {
                xercesc::DOMNodeList * topicNodes = topicListNodes->item(tln)->getChildNodes();

                for (XMLSize_t tn = 0; tn < topicNodes->getLength(); tn++) {
                  if (strcmp("topic", xercesc::XMLString::transcode(topicNodes->item(tn)->getNodeName())) == 0) {
                    anc_ps_rule_holder_.topic_list.push_back(xercesc::XMLString::transcode(topicNodes->item(tn)->getTextContent()));
                  }
                }

              }
              else if (strcmp("partitions", xercesc::XMLString::transcode(topicListNodes->item(tln)->getNodeName())) == 0) {
                  xercesc::DOMNodeList * partitionNodes = topicListNodes->item(tln)->getChildNodes();

                  for (XMLSize_t pn = 0; pn < partitionNodes->getLength(); pn++) {
                      if (strcmp("partition", xercesc::XMLString::transcode(partitionNodes->item(pn)->getNodeName())) == 0) {
                          anc_ps_partition_holder_.partition_list.push_back(xercesc::XMLString::transcode(partitionNodes->item(pn)->getTextContent()));
                      }
                  }
              }
            }

            ptr_holder_.topic_ps_rules.push_back(anc_ps_rule_holder_);
            pp_holder_.partition_ps.push_back(anc_ps_partition_holder_);
          }
        }

        rule_holder_.PermissionTopicRules.push_back(ptr_holder_);
        rule_holder_.PermissionPartitions.push_back(pp_holder_);
      }
    }

    ac_perms_holder->perm_rules.push_back(rule_holder_);
  } // grant_rules

  delete parser;
  delete errHandler;
  return 0;
}

::CORBA::Boolean AccessControlBuiltInImpl::file_exists(const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

std::string AccessControlBuiltInImpl::extract_file_name(const std::string& file_parm) {
  std::string del = ":";
  u_long pos = file_parm.find_last_of(del);
    if ((pos > 0UL) && (pos != file_parm.length() - 1)) {
        return file_parm.substr(pos + 1);
    } else {
        return std::string("");
    }
}

std::string AccessControlBuiltInImpl::get_file_contents(const char *filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
    {
      std::ostringstream contents;
      contents << in.rdbuf();
      in.close();
          return(contents.str());
    }
        throw(errno);
}

::CORBA::Boolean AccessControlBuiltInImpl::clean_smime_content(std::string& content_) {

  std::string start_str("Content-Type: text/plain") , end_str("dds>");

    size_t found_begin = content_.find(start_str);

    if (found_begin != std::string::npos) {
      content_.erase(0, found_begin + start_str.length());
      const char* t = " \t\n\r\f\v";
      content_.erase(0, content_.find_first_not_of(t));
    } else {
      return false;
    }

    size_t found_end = content_.find(end_str);

    if (found_end != std::string::npos) {
      content_.erase(found_end + end_str.length());
    } else {
      return false;
    }

    return true;
}


} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
