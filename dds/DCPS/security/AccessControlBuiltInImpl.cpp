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
#include "dds/DCPS/security/TokenWriter.h"
#include "ace/config-macros.h"
//#include "ace/Guard_T.h"
//#include <sstream>
//#include <vector>

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

  return generate_handle();
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

  return true;
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


} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL


