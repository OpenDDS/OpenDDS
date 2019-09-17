/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */



#ifndef DDS_ACCESS_CONTROL_BUILTIN_IMPL_H
#define DDS_ACCESS_CONTROL_BUILTIN_IMPL_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/TimeTypes.h"

#include "ace/Thread_Mutex.h"
#include "ace/Reactor.h"
#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <memory>

#include "AccessControl/LocalAccessCredentialData.h"
#include "AccessControl/Governance.h"
#include "AccessControl/Permissions.h"
#include "SSL/SubjectName.h"

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
* @brief Implements the DDS built-in version of the Access Control
* plugin for the DDS Security Specification
*
* See the DDS security specification, OMG formal/17-09-20, for a description of
* the interface this class is implementing.
*
*/
class DdsSecurity_Export AccessControlBuiltInImpl
  : public virtual DDS::Security::AccessControl {
public:
  AccessControlBuiltInImpl();
  virtual ~AccessControlBuiltInImpl();

  virtual DDS::Security::PermissionsHandle validate_local_permissions(
    DDS::Security::Authentication_ptr auth_plugin,
    DDS::Security::IdentityHandle identity,
    DDS::Security::DomainId_t domain_id,
    const DDS::DomainParticipantQos& participant_qos,
    DDS::Security::SecurityException& ex);

  virtual DDS::Security::PermissionsHandle validate_remote_permissions(
    DDS::Security::Authentication_ptr auth_plugin,
    DDS::Security::IdentityHandle local_identity_handle,
    DDS::Security::IdentityHandle remote_identity_handle,
    const DDS::Security::PermissionsToken& remote_permissions_token,
    const DDS::Security::AuthenticatedPeerCredentialToken& remote_credential_token,
    DDS::Security::SecurityException& ex);

  virtual bool check_create_participant(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::DomainId_t domain_id,
    const DDS::DomainParticipantQos& qos,
    DDS::Security::SecurityException& ex);

  virtual bool check_create_datawriter(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::DomainId_t domain_id,
    const char* topic_name,
    const DDS::DataWriterQos& qos,
    const DDS::PartitionQosPolicy& partition,
    const DDS::Security::DataTags& data_tag,
    DDS::Security::SecurityException& ex);

  virtual bool check_create_datareader(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::DomainId_t domain_id,
    const char* topic_name,
    const DDS::DataReaderQos& qos,
    const DDS::PartitionQosPolicy& partition,
    const DDS::Security::DataTags& data_tag,
    DDS::Security::SecurityException& ex);

  virtual bool check_create_topic(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::DomainId_t domain_id,
    const char* topic_name,
    const DDS::TopicQos& qos,
    DDS::Security::SecurityException& ex);

  virtual bool check_local_datawriter_register_instance(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::DataWriter_ptr writer,
    DDS::Security::DynamicData_ptr key,
    DDS::Security::SecurityException& ex);

  virtual bool check_local_datawriter_dispose_instance(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::DataWriter_ptr writer,
    DDS::Security::DynamicData_ptr key,
    DDS::Security::SecurityException& ex);

  virtual bool check_remote_participant(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::DomainId_t domain_id,
    const DDS::Security::ParticipantBuiltinTopicDataSecure& participant_data,
    DDS::Security::SecurityException& ex);

  virtual bool check_remote_datawriter(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::DomainId_t domain_id,
    const DDS::Security::PublicationBuiltinTopicDataSecure& publication_data,
    DDS::Security::SecurityException& ex);

  virtual bool check_remote_datareader(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::DomainId_t domain_id,
    const DDS::Security::SubscriptionBuiltinTopicDataSecure& subscription_data,
    bool& relay_only,
    DDS::Security::SecurityException& ex);

  virtual bool check_remote_topic(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::DomainId_t domain_id,
    const DDS::TopicBuiltinTopicData& topic_data,
    DDS::Security::SecurityException& ex);

  virtual bool check_local_datawriter_match(
    DDS::Security::PermissionsHandle writer_permissions_handle,
    DDS::Security::PermissionsHandle reader_permissions_handle,
    const DDS::Security::PublicationBuiltinTopicDataSecure& publication_data,
    const DDS::Security::SubscriptionBuiltinTopicDataSecure& subscription_data,
    DDS::Security::SecurityException& ex);

  virtual bool check_local_datareader_match(
    DDS::Security::PermissionsHandle reader_permissions_handle,
    DDS::Security::PermissionsHandle writer_permissions_handle,
    const DDS::Security::SubscriptionBuiltinTopicDataSecure& subscription_data,
    const DDS::Security::PublicationBuiltinTopicDataSecure& publication_data,
    DDS::Security::SecurityException& ex);

  virtual bool check_remote_datawriter_register_instance(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::DataReader_ptr reader,
    DDS::InstanceHandle_t publication_handle,
    DDS::Security::DynamicData_ptr key,
    DDS::InstanceHandle_t instance_handle,
    DDS::Security::SecurityException& ex);

  virtual bool check_remote_datawriter_dispose_instance(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::DataReader_ptr reader,
    DDS::InstanceHandle_t publication_handle,
    DDS::Security::DynamicData_ptr key,
    DDS::Security::SecurityException& ex);

  virtual bool get_permissions_token(
    DDS::Security::PermissionsToken& permissions_token,
    DDS::Security::PermissionsHandle handle,
    DDS::Security::SecurityException& ex);

  virtual bool get_permissions_credential_token(
    DDS::Security::PermissionsCredentialToken& permissions_credential_token,
    DDS::Security::PermissionsHandle handle,
    DDS::Security::SecurityException& ex);

  virtual bool set_listener(
    DDS::Security::AccessControlListener_ptr listener,
    DDS::Security::SecurityException& ex);

  virtual bool return_permissions_token(
    const DDS::Security::PermissionsToken& token,
    DDS::Security::SecurityException& ex);

  virtual bool return_permissions_credential_token(
    const DDS::Security::PermissionsCredentialToken& permissions_credential_token,
    DDS::Security::SecurityException& ex);

  virtual bool get_participant_sec_attributes(
    DDS::Security::PermissionsHandle permissions_handle,
    DDS::Security::ParticipantSecurityAttributes& attributes,
    DDS::Security::SecurityException& ex);

  virtual bool get_topic_sec_attributes(
    DDS::Security::PermissionsHandle permissions_handle,
    const char* topic_name,
    DDS::Security::TopicSecurityAttributes& attributes,
    DDS::Security::SecurityException& ex);

  virtual bool get_datawriter_sec_attributes(
    DDS::Security::PermissionsHandle permissions_handle,
    const char* topic_name,
    const DDS::PartitionQosPolicy& partition,
    const DDS::Security::DataTagQosPolicy& data_tag,
    DDS::Security::EndpointSecurityAttributes& attributes,
    DDS::Security::SecurityException& ex);

  virtual bool get_datareader_sec_attributes(
    DDS::Security::PermissionsHandle permissions_handle,
    const char* topic_name,
    const DDS::PartitionQosPolicy& partition,
    const DDS::Security::DataTagQosPolicy& data_tag,
    DDS::Security::EndpointSecurityAttributes& attributes,
    DDS::Security::SecurityException& ex);

  virtual bool return_participant_sec_attributes(
    const DDS::Security::ParticipantSecurityAttributes& attributes,
    DDS::Security::SecurityException& ex);

  virtual bool return_datawriter_sec_attributes(
    const DDS::Security::EndpointSecurityAttributes& attributes,
    DDS::Security::SecurityException& ex);

  virtual bool return_datareader_sec_attributes(
    const DDS::Security::EndpointSecurityAttributes& attributes,
    DDS::Security::SecurityException& ex);


private:

  AccessControlBuiltInImpl(const AccessControlBuiltInImpl&);
  AccessControlBuiltInImpl& operator=(const AccessControlBuiltInImpl&);

  struct AccessData {
    DDS::Security::IdentityHandle identity;
    DDS::Security::DomainId_t domain_id;
    SSL::SubjectName subject;
    Permissions::shared_ptr perm;
    Governance::shared_ptr gov;
    LocalAccessCredentialData::shared_ptr local_access_credential_data;
  };

  typedef std::map<DDS::Security::PermissionsHandle, AccessData> ACPermsMap;
  ACPermsMap local_ac_perms_;

  typedef std::map<DDS::Security::IdentityHandle, DDS::Security::PermissionsHandle> ACIdentityMap;
  ACIdentityMap local_identity_map_;

  class RevokePermissionsTimer : public ACE_Event_Handler {
  public:
    RevokePermissionsTimer(AccessControlBuiltInImpl& impl);
    virtual ~RevokePermissionsTimer();
    bool start_timer(const DCPS::TimeDuration& length, DDS::Security::PermissionsHandle pm_handle);
    virtual int handle_timeout(const ACE_Time_Value& tv, const void* arg);
    bool is_scheduled() { return scheduled_; }

  protected:
    AccessControlBuiltInImpl& impl_;

    const DCPS::TimeDuration& interval() const { return interval_; }

  private:
    DCPS::TimeDuration interval_;
    bool scheduled_;
    long timer_id_;
    ACE_Thread_Mutex lock_;
  };

  RevokePermissionsTimer local_rp_timer_;
  RevokePermissionsTimer remote_rp_timer_;

  int generate_handle();

  ACE_Thread_Mutex handle_mutex_;
  ACE_Thread_Mutex gen_handle_mutex_;

  int next_handle_;

  DDS::Security::AccessControlListener_ptr listener_ptr_;

  time_t convert_permissions_time(const std::string& timeString);

  bool validate_date_time(ACPermsMap::iterator ac_iter,
                          DCPS::TimeDuration& delta_time,
                          DDS::Security::SecurityException& ex);

  bool get_sec_attributes(DDS::Security::PermissionsHandle permissions_handle,
                          const char* topic_name,
                          const DDS::PartitionQosPolicy& partition,
                          const DDS::Security::DataTagQosPolicy& data_tag,
                          DDS::Security::EndpointSecurityAttributes& attributes,
                          DDS::Security::SecurityException& ex);

  bool search_local_permissions(const char* topic_name,
                                DDS::Security::DomainId_t domain_id,
                                const DDS::PartitionQosPolicy& partition,
                                Permissions::PublishSubscribe_t pub_or_sub,
                                ACPermsMap::iterator ac_iter,
                                DDS::Security::SecurityException& ex);

  bool search_remote_permissions(const char* topic_name,
                                 DDS::Security::DomainId_t domain_id,
                                 ACPermsMap::iterator ac_iter,
                                 Permissions::PublishSubscribe_t pub_or_sub,
                                 DDS::Security::SecurityException& ex);

  void parse_class_id(const std::string& class_id,
                      std::string& plugin_class_name,
                      int& major_version,
                      int& minor_version);

};

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
