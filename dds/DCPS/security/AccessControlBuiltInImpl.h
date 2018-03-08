/*
 *
 *
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */



#ifndef DDS_ACCESS_CONTROL_BUILTIN_IMPL_H
#define DDS_ACCESS_CONTROL_BUILTIN_IMPL_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"

#include "ace/Thread_Mutex.h"
#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <memory>

#include "AccessControl/LocalCredentialData.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {


/**
* @class AccessControlBuiltInImpl
*
* @brief Implements the DDS built-in version of the Access Contro
* plugin for the DDS Security Specification
*
* See the DDS security specification, OMG formal/17-09-20, for a description of
* the interface this class is implementing.
*
*/
class DdsSecurity_Export  AccessControlBuiltInImpl
	: public virtual DDS::Security::AccessControl
{
public:
  AccessControlBuiltInImpl();
  virtual ~AccessControlBuiltInImpl();

  virtual ::DDS::Security::PermissionsHandle validate_local_permissions (
    ::DDS::Security::Authentication_ptr auth_plugin,
    ::DDS::Security::IdentityHandle identity,
    ::DDS::Security::DomainId_t domain_id,
    const ::DDS::DomainParticipantQos & participant_qos,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::PermissionsHandle validate_remote_permissions (
    ::DDS::Security::Authentication_ptr auth_plugin,
    ::DDS::Security::IdentityHandle local_identity_handle,
    ::DDS::Security::IdentityHandle remote_identity_handle,
    const ::DDS::Security::PermissionsToken & remote_permissions_token,
    const ::DDS::Security::AuthenticatedPeerCredentialToken & remote_credential_token,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_create_participant (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::DomainId_t domain_id,
    const ::DDS::DomainParticipantQos & qos,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_create_datawriter (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::DomainId_t domain_id,
    const char * topic_name,
    const ::DDS::DataWriterQos & qos,
    const ::DDS::PartitionQosPolicy & partition,
    const ::DDS::Security::DataTags & data_tag,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_create_datareader (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::DomainId_t domain_id,
    const char * topic_name,
    const ::DDS::DataReaderQos & qos,
    const ::DDS::PartitionQosPolicy & partition,
    const ::DDS::Security::DataTags & data_tag,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_create_topic (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::DomainId_t domain_id,
    const char * topic_name,
    const ::DDS::TopicQos & qos,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_local_datawriter_register_instance (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::DataWriter_ptr writer,
    ::DDS::Security::DynamicData_ptr key,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_local_datawriter_dispose_instance (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::DataWriter_ptr writer,
    ::DDS::Security::DynamicData_ptr key,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_remote_participant (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::DomainId_t domain_id,
    const ::DDS::Security::ParticipantBuiltinTopicDataSecure & participant_data,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_remote_datawriter (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::DomainId_t domain_id,
    const ::DDS::Security::PublicationBuiltinTopicDataSecure & publication_data,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_remote_datareader (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::DomainId_t domain_id,
    const ::DDS::Security::SubscriptionBuiltinTopicDataSecure & subscription_data,
    ::CORBA::Boolean & relay_only,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_remote_topic (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::DomainId_t domain_id,
    const ::DDS::TopicBuiltinTopicData & topic_data,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_local_datawriter_match (
    ::DDS::Security::PermissionsHandle writer_permissions_handle,
    ::DDS::Security::PermissionsHandle reader_permissions_handle,
    const ::DDS::Security::PublicationBuiltinTopicDataSecure & publication_data,
    const ::DDS::Security::SubscriptionBuiltinTopicDataSecure & subscription_data,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_local_datareader_match (
    ::DDS::Security::PermissionsHandle reader_permissions_handle,
    ::DDS::Security::PermissionsHandle writer_permissions_handle,
    const ::DDS::Security::SubscriptionBuiltinTopicDataSecure & subscription_data,
    const ::DDS::Security::PublicationBuiltinTopicDataSecure & publication_data,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_remote_datawriter_register_instance (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::DataReader_ptr reader,
    ::DDS::InstanceHandle_t publication_handle,
    ::DDS::Security::DynamicData_ptr key,
    ::DDS::InstanceHandle_t instance_handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean check_remote_datawriter_dispose_instance (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::DataReader_ptr reader,
    ::DDS::InstanceHandle_t publication_handle,
    ::DDS::Security::DynamicData_ptr key,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_permissions_token (
    ::DDS::Security::PermissionsToken & permissions_token,
    ::DDS::Security::PermissionsHandle handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_permissions_credential_token (
    ::DDS::Security::PermissionsCredentialToken & permissions_credential_token,
    ::DDS::Security::PermissionsHandle handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean set_listener (
    ::DDS::Security::AccessControlListener_ptr listener,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_permissions_token (
    const ::DDS::Security::PermissionsToken & token,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_permissions_credential_token (
    const ::DDS::Security::PermissionsCredentialToken & permissions_credential_token,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_participant_sec_attributes (
    ::DDS::Security::PermissionsHandle permissions_handle,
    ::DDS::Security::ParticipantSecurityAttributes & attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_topic_sec_attributes (
    ::DDS::Security::PermissionsHandle permissions_handle,
    const char * topic_name,
    ::DDS::Security::TopicSecurityAttributes & attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_datawriter_sec_attributes (
    ::DDS::Security::PermissionsHandle permissions_handle,
    const char * topic_name,
    const ::DDS::PartitionQosPolicy & partition,
    const ::DDS::Security::DataTagQosPolicy & data_tag,
    ::DDS::Security::EndpointSecurityAttributes & attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_datareader_sec_attributes (
    ::DDS::Security::PermissionsHandle permissions_handle,
    const char * topic_name,
    const ::DDS::PartitionQosPolicy & partition,
    const ::DDS::Security::DataTagQosPolicy & data_tag,
    ::DDS::Security::EndpointSecurityAttributes & attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_participant_sec_attributes (
    const ::DDS::Security::ParticipantSecurityAttributes & attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_datawriter_sec_attributes (
    const ::DDS::Security::EndpointSecurityAttributes & attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_datareader_sec_attributes (
    const ::DDS::Security::EndpointSecurityAttributes & attributes,
    ::DDS::Security::SecurityException & ex);


private:

  AccessControlBuiltInImpl(const AccessControlBuiltInImpl& right);
  AccessControlBuiltInImpl& operator-=(const AccessControlBuiltInImpl& right);

    // Governance Rule definitions

    typedef struct {
        const char * topic_expression;
        ::DDS::Security::TopicSecurityAttributes topic_attrs;
    } TopicAccessRule;

    typedef std::vector<TopicAccessRule> TopicAccessRules;

    typedef struct {
        std::set< ::DDS::Security::DomainId_t > domain_list;
        ::DDS::Security::ParticipantSecurityAttributes domain_attrs;
        TopicAccessRules topic_rules;
    } domain_rule;


    typedef std::vector<domain_rule> GovernanceAccessRules;


    // TODO: the ParticipantGovMapType needs to support multiple domain_rule(s). See domain_access_rules above



    // Permission Rule definitions

    typedef struct {
        std::string not_before;
        std::string not_after;
    } Validity_t;


    typedef struct {
        std::string grant_name;
        std::string subject;
        Validity_t validity;

    } permission_grant_rule;

    typedef std::vector<permission_grant_rule> PermissionGrantRules;



    typedef struct {
        GovernanceAccessRules gov_rules;
        PermissionGrantRules perm_rules;
        ::DDS::Security::PermissionsToken perm_token;
        ::DDS::Security::PermissionsCredentialToken perm_cred_token;
    } ac_perms;

    typedef std::map< ::DDS::Security::PermissionsHandle , ac_perms > ACPermsMap;

    ACPermsMap local_ac_perms;


  ::CORBA::Long generate_handle();
  ::CORBA::Long load_governance_file(ac_perms *, std::string);
  ::CORBA::Long load_permissions_file(ac_perms *, std::string);
  ::CORBA::Boolean file_exists(const std::string&);
  std::string extract_file_name(const std::string&);
  std::string get_file_contents(const char *);
  ::CORBA::Boolean clean_smime_content(std::string&);

  ACE_Thread_Mutex handle_mutex_;
  ::CORBA::Long next_handle_;

  LocalAccessCredentialData local_access_control_data_;

};

} // namespace Security
} // namespace OpenDDS

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

#endif
