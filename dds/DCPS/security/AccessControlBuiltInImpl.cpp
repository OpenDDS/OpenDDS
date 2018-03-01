/*
*
*
* Distributed under the DDS License.
* See: http://www.opendds.org/license.html
*/

#include "dds/DCPS/security/AccessControlBuiltInImpl.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DdsDcpsInfrastructureC.h"
//#include "dds/DCPS/security/TokenReader.h"
#include "ace/config-macros.h"

#include "dds/DCPS/security/TokenWriter.h"

//#include "ace/Guard_T.h"
//#include <sstream>
//#include <vector>
#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/util/XMLString.hpp"
#include "AccessControlBuiltInImpl.h"
#include <xercesc/util/PlatformUtils.hpp>

#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>


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
  if (DDS::HANDLE_NIL == identity) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid identity");
    return DDS::HANDLE_NIL;
  }

  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(participant_qos);

    // Pull file attribute info from qos
    // Place holder for not having qos populated
  std::string ac_files_path = "/home/neeleym/dev/ddsinterop/bg/Security_Demo_12_2017_Burlingame/configuration_files/";
  std::string gov_file = ac_files_path.append("governance/Governance_SC0_SecurityDisabled.xml");
  std::string perm_file = ac_files_path.append("permissions/Permissions_JoinDomain_OCI.xml");

  ::DDS::Security::PermissionsHandle perm_handle = load_governance_file(gov_file);
  if(-1 == perm_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid governance file");
    return DDS::HANDLE_NIL;
  };


//Test get_participant_sec_attributes

   ::DDS::Security::ParticipantSecurityAttributes attrs;
   get_participant_sec_attributes(perm_handle,attrs,ex);


  std::cout <<"created handle:"
            << perm_handle
            << std::endl
            << " allow_unauthenticated_participants:" << attrs.allow_unauthenticated_participants
            << std::endl
            << " is_rtps_protected:" << attrs.is_rtps_protected
            << std::endl;

  // Test check_create_participant

  ::DDS::Security::DomainId_t d_id;
  ::CORBA::Boolean z = check_create_participant(perm_handle, 0,participant_qos,ex) ;
  std::cout << "check_create_participant: "  << z << std::endl;
  /*
  if(0 != load_permissions_file(perm_file)) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permission file");
    return DDS::HANDLE_NIL;
  };
   */


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
  ACE_UNUSED_ARG(remote_permissions_token);
  ACE_UNUSED_ARG(remote_credential_token);

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

  return generate_handle();
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

  // This is dummy data
/*
  ::DDS::Security::DomainId_t  dummy_domain = 0;
  ::DDS::Security::ParticipantSecurityAttributes psa;
  ::DDS::Security::SecurityException exc;
*/

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


  ParticipantGovMapType::iterator iter = pgov_map.begin();
  iter = pgov_map.find(permissions_handle);
  if(iter == pgov_map.end()) {
    CommonUtilities::set_security_error(ex,-1, 0, "No matching permissions handle present");
    return false;
  }

  // 1. Domain element

  if ( iter->second.domain_list.find(domain_id) == iter->second.domain_list.end()){
      std::cout << "Domain ID of " << domain_id << "not found" << std::endl;
    return false;
      //TODO: this checks the governance file, but do we also need to look at the permissions file?
  }

  // Check topic rules for the given domain id.

  for(int r = 0; r < iter->second.topic_rules.size(); r++) {
    if(iter->second.topic_rules[r].topic_attrs.is_read_protected == false ||
       iter->second.topic_rules[r].topic_attrs.is_write_protected == false)
      return true;
  }

    // Check is_access_protected
  if( iter->second.domain_attrs.is_access_protected == false)
    return true;

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
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(qos);
  ACE_UNUSED_ARG(partition);
  ACE_UNUSED_ARG(data_tag);
  ACE_UNUSED_ARG(ex);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }
  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Name");
    return false;
  }

  return true;
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
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(qos);
  ACE_UNUSED_ARG(partition);
  ACE_UNUSED_ARG(data_tag);
  ACE_UNUSED_ARG(ex);

  if (DDS::HANDLE_NIL == permissions_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid permissions handle");
    return false;
  }
  if (0 == topic_name) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Topic Name");
    return false;
  }

  return true;
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

  return true;
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


  return true;
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

  return true;
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

  // This will just return a fixed token
  TokenWriter writer(permissions_token, PermissionsTokenClassId, 2, 0);
  writer.set_property(0, "dds.perm_ca.sn", "MyCA Name", true);
  writer.set_property(1, "dds.perm_ca.algo", "RSA-2048", true);

  return true;
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

  // This will just return a fixed token
  TokenWriter writer(permissions_credential_token, PermissionsCredentialTokenClassId, 1, 0);
  writer.set_property(0, "dds.perm.cert", "TheContentofThePermDocument", true);

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

   ParticipantGovMapType::iterator iter = pgov_map.begin();
   iter = pgov_map.find(permissions_handle);
   if(iter != pgov_map.end()) {
     std::cout << "aup:" << iter->second.domain_attrs.allow_unauthenticated_participants << std::endl;
     attributes.allow_unauthenticated_participants = iter->second.domain_attrs.allow_unauthenticated_participants;
     attributes.is_access_protected = iter->second.domain_attrs.is_access_protected;
     attributes.is_rtps_protected = iter->second.domain_attrs.is_rtps_protected;
     attributes.is_discovery_protected = iter->second.domain_attrs.is_discovery_protected;
     attributes.is_liveliness_protected = iter->second.domain_attrs.is_liveliness_protected;
     attributes.plugin_participant_attributes = iter->second.domain_attrs.plugin_participant_attributes;
     return true;
   }


  // Set all permissions to true in the stub
   attributes.allow_unauthenticated_participants = true;
   attributes.is_access_protected = true;
   attributes.is_rtps_protected = true;
   attributes.is_discovery_protected = true;
   attributes.is_liveliness_protected = true;
   attributes.plugin_participant_attributes = 0xFFFFFFFF;

  return true;
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

  // Set all permissions to true in the stub
  attributes.is_read_protected = true;
  attributes.is_write_protected = true;
  attributes.is_discovery_protected = true;
  attributes.is_liveliness_protected = true;

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_datawriter_sec_attributes(
  ::DDS::Security::PermissionsHandle permissions_handle,
  const char * /*topic_name*/,
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
  //if (0 == topic_name) {
  //  CommonUtilities::set_security_error(ex, -1, 0, "Invalid topic name");
  //  return false;
  //}

  // Set all permissions to true in the stub
  attributes.is_submessage_protected = true;
  attributes.is_payload_protected = true;
  attributes.is_key_protected = true;
  attributes.plugin_endpoint_attributes = 0xFFFFFFFF;

  return true;
}

::CORBA::Boolean AccessControlBuiltInImpl::get_datareader_sec_attributes(
  ::DDS::Security::PermissionsHandle permissions_handle,
  const char * /*topic_name*/,
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
  //if (0 == topic_name) {
  //  CommonUtilities::set_security_error(ex, -1, 0, "Invalid topic name");
  //  return false;
  //}

  // Set all permissions to true in the stub
  attributes.is_submessage_protected = true;
  attributes.is_payload_protected = true;
  attributes.is_key_protected = true;
  attributes.plugin_endpoint_attributes = 0xFFFFFFFF;

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
  ACE_Guard<ACE_Thread_Mutex> guard(handle_mutex_);
  ::CORBA::Long new_handle = next_handle_++;

  if (new_handle == DDS::HANDLE_NIL) {
    new_handle = next_handle_++;
  }
  return new_handle;
}


::CORBA::Long AccessControlBuiltInImpl::load_governance_file(std::string g_file)
{


  /* Set all permissions to true in the stub
      domain_rule gov_config_data;
      gov_config_data.domain_list.insert(0);
      gov_config_data.domain_attrs.allow_unauthenticated_participants = true;
      gov_config_data.domain_attrs.is_access_protected = false;
      gov_config_data.domain_attrs.is_discovery_protected = false;
      gov_config_data.domain_attrs.is_liveliness_protected = false;
      gov_config_data.domain_attrs.is_rtps_protected = false;
     gov_config_data.domain_attrs.plugin_participant_attributes = 0xFFFFFFFF;

     TopicAccessRule t_rules;

     t_rules.topic_expression = "*";
     t_rules.topic_attrs.is_liveliness_protected = 0;
     t_rules.topic_attrs.is_discovery_protected = 0;
     t_rules.topic_attrs.is_read_protected = 1;
     t_rules.topic_attrs.is_write_protected = 1;

     gov_config_data.topic_rules.push_back(t_rules);

     domain_access_rules gov_domain_access_rules;
     gov_domain_access_rules.push_back(gov_config_data);
     */

     ::DDS::Security::PermissionsHandle ph = generate_handle();
     // pgov_map.insert(std::make_pair(ph , gov_config_data ));

     ParticipantGovMapType::iterator iter = pgov_map.begin();
     iter = pgov_map.find(ph);
     if(iter != pgov_map.end()) {
        std::cout << "handle:" << iter->first << std::endl;
     }

  //std::cout << "Gov size:" << pgov_map.size() <<  "Gov entry:" << pgov_map.count(1) << std::endl;


  try
  {
    xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
    std::cout  << "Xerces initialized..." << std::endl;
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

    xercesc::ErrorHandler* errHandler = (xercesc::ErrorHandler*) new xercesc::HandlerBase();
    parser->setErrorHandler(errHandler);

    try {
        parser->parse(g_file.c_str());
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
        std::cout << "Unexpected Exception \n" ;
        return -1;
    }


  // Successfully parsed the governance file

   xercesc::DOMDocument* xmlDoc = parser->getDocument();

   xercesc::DOMElement* elementRoot = xmlDoc->getDocumentElement();
  if( !elementRoot ) throw(std::runtime_error( "empty XML document" ));

   // Find the domain rules
   xercesc::DOMNodeList * domainRules = xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("domain_rule"));


   for(XMLSize_t r=0;r<domainRules->getLength();r++) {
       domain_rule rule_holder_;

       // Pull out domain ids used in the rule. We are NOT supporting ranges at this time
       xercesc::DOMNodeList * ruleNodes = domainRules->item(r)->getChildNodes();
       for( XMLSize_t rn = 0; rn<ruleNodes->getLength();rn++) {
           char * dn_tag = xercesc::XMLString::transcode(ruleNodes->item(rn)->getNodeName());
           if ( strcmp("domains", dn_tag) == 0 ) {
             std::cout << xercesc::XMLString::transcode(ruleNodes->item(rn)->getNodeName()) << std::endl;
             xercesc::DOMNodeList * domainIdNodes = ruleNodes->item(rn)->getChildNodes();
               for(XMLSize_t did = 0; did<domainIdNodes->getLength();did++) {
                 if(strcmp("id" , xercesc::XMLString::transcode(domainIdNodes->item(did)->getNodeName())) == 0) {
                   rule_holder_.domain_list.insert(atoi(xercesc::XMLString::transcode(domainIdNodes->item(did)->getTextContent())));
                 }
               }

           }
       }

     // Process allow_unauthenticated_participants
     xercesc::DOMNodeList * allow_unauthenticated_participants_ =
              xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("allow_unauthenticated_participants"));
     char * attr_aup = xercesc::XMLString::transcode(allow_unauthenticated_participants_->item(0)->getTextContent());
     std::cout << "attr_aup:" << attr_aup << std::endl;
     rule_holder_.domain_attrs.allow_unauthenticated_participants = (strcmp(attr_aup,"false") == 0 ? false : true);


     // Process enable_join_access_control
     xercesc::DOMNodeList * enable_join_access_control_ =
             xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("enable_join_access_control"));
     char * attr_ejac = xercesc::XMLString::transcode(enable_join_access_control_->item(0)->getTextContent());
     rule_holder_.domain_attrs.is_access_protected = (strcmp(attr_ejac, "false") == 0 ? false : true);


     // Process discovery_protection_kind
     xercesc::DOMNodeList * discovery_protection_kind_ =
             xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("discovery_protection_kind"));
     char * attr_dpk = xercesc::XMLString::transcode(discovery_protection_kind_->item(0)->getTextContent());
     rule_holder_.domain_attrs.is_discovery_protected= (strcmp(attr_dpk, "NONE") == 0 ? false : true);

     // Process liveliness_protection_kind
     xercesc::DOMNodeList * liveliness_protection_kind_ =
             xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("liveliness_protection_kind"));
     char * attr_lpk = xercesc::XMLString::transcode(liveliness_protection_kind_->item(0)->getTextContent());
     rule_holder_.domain_attrs.is_liveliness_protected = (strcmp(attr_lpk, "NONE") == 0 ? false : true);

     // Process rtps_protection_kind
     xercesc::DOMNodeList * rtps_protection_kind_ =
             xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("rtps_protection_kind"));
     char * attr_rpk = xercesc::XMLString::transcode(rtps_protection_kind_->item(0)->getTextContent());
     rule_holder_.domain_attrs.is_rtps_protected = (strcmp(attr_rpk, "NONE") == 0 ? false : true);



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
               t_rules.topic_attrs.is_discovery_protected = strcmp(tr_val,"false") == 0 ? false : true;
             } else if(strcmp(tr_tag, "enable_liveliness_protection") == 0) {
               t_rules.topic_attrs.is_liveliness_protected = strcmp(tr_val,"false") == 0 ? false : true;
             } else if(strcmp(tr_tag, "enable_read_access_control") == 0) {
               t_rules.topic_attrs.is_read_protected = strcmp(tr_val,"false") == 0 ? false : true;
             } else if(strcmp(tr_tag, "enable_write_access_control") == 0) {
               t_rules.topic_attrs.is_write_protected = strcmp(tr_val,"false") == 0 ? false : true;
             }
           /* The following two Topic Rules are not supported at this time
                        <metadata_protection_kind>NONE</metadata_protection_kind>
                        <data_protection_kind>NONE</data_protection_kind>
           */

         }
         rule_holder_.topic_rules.push_back(t_rules);
       }


       std::cout << "Inserting:" << ph << std::endl;
       std::cout << "allow_unauthenticated_participants: " << rule_holder_.domain_attrs.allow_unauthenticated_participants << std::endl;
       pgov_map.insert(std::make_pair(ph , rule_holder_));

   } // domain_rule

  std::cout << "domain rules count:" << domainRules->getLength() << std::endl;



    delete parser;
    delete errHandler;


  return ph;
}


::CORBA::Long AccessControlBuiltInImpl::load_permissions_file(std::string p_file)
{
  try
  {
    xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
    std::cout  << "Xerces initialized..." << std::endl;
  }
  catch( xercesc::XMLException& e )
  {
    char* message = xercesc::XMLString::transcode( e.getMessage() );
    std::cerr << "XML toolkit initialization error: " << message << std::endl;
    xercesc::XMLString::release( &message );
    // throw exception here to return ERROR_XERCES_INIT
  }
  return 0;
}


} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL


