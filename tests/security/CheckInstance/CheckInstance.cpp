#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/RTPS/BaseMessageTypes.h>
#include <dds/DCPS/RTPS/GuidGenerator.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/ParameterListConverter.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/security/AccessControlBuiltInImpl.h>
#include <dds/DCPS/security/AuthenticationBuiltInImpl.h>
#include <dds/DCPS/security/CommonUtilities.h>
#include <dds/DCPS/security/CryptoBuiltInImpl.h>
#include <dds/DCPS/security/UtilityImpl.h>
#include <dds/DCPS/security/framework/Properties.h>
#include <dds/DCPS/security/framework/SecurityPluginInst_rch.h>
#include <dds/DCPS/security/framework/SecurityRegistry.h>

#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/StatusMatching.h"

#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/security/BuiltInPlugins.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <stdexcept>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

const size_t CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_FALSE = (1<<0);
const size_t CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_TRUE = (1<<1);
const size_t CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_FALSE = (1<<2);
const size_t CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_TRUE = (1<<3);
const size_t CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_FALSE = (1<<4);
const size_t CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_TRUE = (1<<5);
const size_t CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_FALSE = (1<<6);
const size_t CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_TRUE = (1<<7);

class CustomAccessControl : public OpenDDS::Security::AccessControlBuiltInImpl {
public:

  CustomAccessControl()
    : register_value_(false)
    , dispose_value_(false)
    , condition_(mutex_)
  {
    reset_flags();
  }

  bool check_local_datawriter_register_instance(DDS::Security::PermissionsHandle permissions_handle,
                                                DDS::DataWriter_ptr writer,
                                                DDS::DynamicData_ptr key,
                                                DDS::Security::SecurityException& ex)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, false);

    if (!OpenDDS::Security::AccessControlBuiltInImpl::check_local_datawriter_register_instance(permissions_handle, writer, key, ex)) {
      flags_ |= CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return false;
    }

    if (0 == key->type()) {
      flags_ |= CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_local_datawriter_register_instance: No type support");
    }

    DDS::MemberId member_id = key->get_member_id_by_name("subject_id");
    if (member_id == OpenDDS::XTypes::MEMBER_ID_INVALID) {
      flags_ |= CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_local_datawriter_register_instance: No subject_id");
    }

    CORBA::Long subject_id;
    if (key->get_int32_value(subject_id, member_id) != DDS::RETCODE_OK) {
      flags_ |= CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_local_datawriter_register_instance: Could not get subject_id");
    }

    if (subject_id != 0) {
      flags_ |= CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_local_datawriter_register_instance: subject_id != 0");
    }

    flags_ |= register_value_ ? CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_TRUE : CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_FALSE;
    condition_.notify_one();
    return register_value_;
  }

  bool check_local_datawriter_dispose_instance(DDS::Security::PermissionsHandle permissions_handle,
                                               DDS::DataWriter_ptr writer,
                                               DDS::DynamicData_ptr key,
                                               DDS::Security::SecurityException& ex)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, false);

    if (!OpenDDS::Security::AccessControlBuiltInImpl::check_local_datawriter_dispose_instance(permissions_handle, writer, key, ex)) {
      flags_ |= CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return false;
    }

    if (0 == key->type()) {
      flags_ |= CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_local_datawriter_dispose_instance: No type support");
    }

    DDS::MemberId member_id = key->get_member_id_by_name("subject_id");
    if (member_id == OpenDDS::XTypes::MEMBER_ID_INVALID) {
      flags_ |= CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_local_datawriter_dispose_instance: No subject_id");
    }

    CORBA::Long subject_id;
    if (key->get_int32_value(subject_id, member_id) != DDS::RETCODE_OK) {
      flags_ |= CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_local_datawriter_dispose_instance: Could not get subject_id");
    }

    if (subject_id != 0) {
      flags_ |= CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_local_datawriter_dispose_instance: subject_id != 0");
    }

    flags_ |= dispose_value_ ? CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_TRUE : CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_FALSE;
    condition_.notify_one();
    return dispose_value_;
  }

  bool check_remote_datawriter_register_instance(DDS::Security::PermissionsHandle permissions_handle,
                                                 DDS::DataReader_ptr reader,
                                                 DDS::InstanceHandle_t publication_handle,
                                                 DDS::DynamicData_ptr key,
                                                 DDS::Security::SecurityException& ex)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, false);

    if (!OpenDDS::Security::AccessControlBuiltInImpl::check_remote_datawriter_register_instance(permissions_handle, reader, publication_handle, key, ex)) {
      flags_ |= CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return false;
    }

    if (0 == key->type()) {
      flags_ |= CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_remote_datawriter_register_instance: No type support");
    }

    DDS::MemberId member_id = key->get_member_id_by_name("subject_id");
    if (member_id == OpenDDS::XTypes::MEMBER_ID_INVALID) {
      flags_ |= CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_remote_datawriter_register_instance: No subject_id");
    }

    CORBA::Long subject_id;
    if (key->get_int32_value(subject_id, member_id) != DDS::RETCODE_OK) {
      flags_ |= CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_remote_datawriter_register_instance: Could not get subject_id");
    }

    if (subject_id != 0) {
      flags_ |= CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_remote_datawriter_register_instance: subject_id != 0");
    }

    flags_ |= register_value_ ? CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_TRUE : CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_FALSE;
    condition_.notify_one();
    return register_value_;
  }

  bool check_remote_datawriter_dispose_instance(DDS::Security::PermissionsHandle permissions_handle,
                                                DDS::DataReader_ptr reader,
                                                DDS::InstanceHandle_t publication_handle,
                                                DDS::DynamicData_ptr key,
                                                DDS::Security::SecurityException& ex)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, false);

    if (!OpenDDS::Security::AccessControlBuiltInImpl::check_remote_datawriter_dispose_instance(permissions_handle, reader, publication_handle, key, ex)) {
      flags_ |= CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return false;
    }

    if (0 == key->type()) {
      flags_ |= CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_remote_datawriter_dispose_instance: No type support");
    }

    DDS::MemberId member_id = key->get_member_id_by_name("subject_id");
    if (member_id == OpenDDS::XTypes::MEMBER_ID_INVALID) {
      flags_ |= CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_remote_datawriter_dispose_instance: No subject_id");
    }

    CORBA::Long subject_id;
    if (key->get_int32_value(subject_id, member_id) != DDS::RETCODE_OK) {
      flags_ |= CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_remote_datawriter_dispose_instance: Could not get subject_id");
    }

    if (subject_id != 0) {
      flags_ |= CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_FALSE;
      condition_.notify_one();
      return OpenDDS::Security::CommonUtilities::set_security_error(ex, -1, 0, "CustomAccessControl::check_remote_datawriter_dispose_instance: subject_id != 0");
    }

    flags_ |= dispose_value_ ? CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_TRUE : CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_FALSE;
    condition_.notify_one();
    return dispose_value_;
  }

  void reset_flags()
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    flags_ = 0;
  }

  void wait_for(size_t mask)
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    ACE_DEBUG((LM_DEBUG, "Waiting for %B\n", mask));
    while ((mask & flags_) != mask) {
      condition_.wait(TheServiceParticipant->get_thread_status_manager());
    }
    ACE_DEBUG((LM_DEBUG, "Done waiting for %B\n", mask));
  }

  void set_return(bool register_value, bool dispose_value)
  {
    register_value_ = register_value;
    dispose_value_ = dispose_value;
  }

private:
  bool register_value_;
  bool dispose_value_;
  mutable ACE_Thread_Mutex mutex_;
  OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> condition_;
  size_t flags_;
};

class CustomSecurityPluginInst : public OpenDDS::Security::SecurityPluginInst {
public:
  CustomSecurityPluginInst()
    : authentication_(new OpenDDS::Security::AuthenticationBuiltInImpl)
    , access_control_(new CustomAccessControl)
    , key_factory_(new OpenDDS::Security::CryptoBuiltInImpl)
    , key_exchange_(DDS::Security::CryptoKeyExchange::_narrow(key_factory_))
    , transform_(DDS::Security::CryptoTransform::_narrow(key_factory_))
    , utility_(OpenDDS::DCPS::make_rch<OpenDDS::Security::UtilityImpl>())
  {}

  void reset_flags()
  {
    CustomAccessControl* cac = dynamic_cast<CustomAccessControl*>(access_control_.in());
    if (!cac) {
      return;
    }

    cac->reset_flags();
  }

  void wait_for(size_t mask) const
  {
    CustomAccessControl* cac = dynamic_cast<CustomAccessControl*>(access_control_.in());
    if (!cac) {
      return;
    }

    cac->wait_for(mask);
  }

  void set_return(bool register_value, bool dispose_value)
  {
    CustomAccessControl* cac = dynamic_cast<CustomAccessControl*>(access_control_.in());
    if (!cac) {
      return;
    }

    cac->set_return(register_value, dispose_value);
  }

private:
  DDS::Security::Authentication_var create_authentication() { return authentication_; }
  DDS::Security::AccessControl_var create_access_control() { return access_control_; }
  DDS::Security::CryptoKeyExchange_var create_crypto_key_exchange() { return key_exchange_; }
  DDS::Security::CryptoKeyFactory_var create_crypto_key_factory() { return key_factory_; }
  DDS::Security::CryptoTransform_var create_crypto_transform() { return transform_; }
  OpenDDS::DCPS::RcHandle<OpenDDS::Security::Utility> create_utility() { return utility_; }
  void shutdown() {}

  DDS::Security::Authentication_var authentication_;
  DDS::Security::AccessControl_var access_control_;
  DDS::Security::CryptoKeyFactory_var key_factory_;
  DDS::Security::CryptoKeyExchange_var key_exchange_;
  DDS::Security::CryptoTransform_var transform_;
  OpenDDS::DCPS::RcHandle<OpenDDS::Security::Utility> utility_;
};

void append(DDS::PropertySeq& props, const char* name, const OpenDDS::DCPS::String& value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value.c_str(), propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  // Parse options.
  OpenDDS::DCPS::String DDS_ROOT;

  const char* value = ACE_OS::getenv("DDS_ROOT");
  if (value) {
    DDS_ROOT = value;
  }

  ACE_DEBUG((LM_DEBUG, "DDS_ROOT = %C\n", DDS_ROOT.c_str()));

  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  TheServiceParticipant->set_security(true);

  const DDS::DomainId_t domain = 0;
  const OpenDDS::DCPS::String prefix = "file:" + DDS_ROOT + "/tests/security/";

  const RcHandle<RtpsDiscovery> disc = make_rch<RtpsDiscovery>("RtpsDiscovery");
  TheServiceParticipant->add_discovery(disc);
  TheServiceParticipant->set_default_discovery(disc->key());

  // Create a participant without the custom plugin.
  DDS::DomainParticipantQos participant1_qos;
  dpf->get_default_participant_qos(participant1_qos);

  DDS::PropertySeq& props1 = participant1_qos.property.value;
  append(props1, DDS::Security::Properties::AuthIdentityCA, prefix + "certs/identity/identity_ca_cert.pem");
  append(props1, DDS::Security::Properties::AuthPrivateKey, prefix + "certs/identity/test_participant_01_private_key.pem");
  append(props1, DDS::Security::Properties::AuthIdentityCertificate, prefix + "certs/identity/test_participant_01_cert.pem");
  append(props1, DDS::Security::Properties::AccessPermissionsCA, prefix + "certs/permissions/permissions_ca_cert.pem");
  append(props1, DDS::Security::Properties::AccessGovernance, prefix + "governance/governance_SC1_ProtectedDomain1_signed.p7s");
  append(props1, DDS::Security::Properties::AccessPermissions, prefix + "permissions/permissions_test_participant_01_signed.p7s");

  DDS::DomainParticipant_var participant1 = dpf->create_participant(domain, participant1_qos, 0, 0);

  TransportConfig_rch config1 = TheTransportRegistry->create_config("config1");
  TransportInst_rch inst1 = TheTransportRegistry->create_inst("inst1", "rtps_udp");
  config1->instances_.push_back(inst1);
  TheTransportRegistry->bind_config(config1, participant1);

  Messenger::MessageTypeSupport_var type_support1 = new Messenger::MessageTypeSupportImpl();
  if (type_support1->register_type(participant1.in(), "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): register_type failed!\n"));
    return EXIT_FAILURE;
  }

  CORBA::String_var type_name1 = type_support1->get_type_name();
  DDS::Topic_var topic1 = participant1->create_topic("Movie Discussion List",
                                                     type_name1.in(),
                                                     TOPIC_QOS_DEFAULT,
                                                     DDS::TopicListener::_nil(),
                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::Publisher_var publisher1 = participant1->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                                 DDS::PublisherListener::_nil(),
                                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Install the custom plugin.
  RcHandle<CustomSecurityPluginInst> custom_plugin = OpenDDS::DCPS::make_rch<CustomSecurityPluginInst>();
  TheSecurityRegistry->register_plugin("Custom", custom_plugin);
  OpenDDS::Security::SecurityConfig_rch custom_config = TheSecurityRegistry->create_config("Custom", custom_plugin);
  TheSecurityRegistry->default_config(custom_config);

  // Create a participant without the custom plugin.
  DDS::DomainParticipantQos participant2_qos;
  dpf->get_default_participant_qos(participant2_qos);

  DDS::PropertySeq& props2 = participant2_qos.property.value;
  append(props2, DDS::Security::Properties::AuthIdentityCA, prefix + "certs/identity/identity_ca_cert.pem");
  append(props2, DDS::Security::Properties::AuthPrivateKey, prefix + "certs/identity/test_participant_02_private_key.pem");
  append(props2, DDS::Security::Properties::AuthIdentityCertificate, prefix + "certs/identity/test_participant_02_cert.pem");
  append(props2, DDS::Security::Properties::AccessPermissionsCA, prefix + "certs/permissions/permissions_ca_cert.pem");
  append(props2, DDS::Security::Properties::AccessGovernance, prefix + "governance/governance_SC1_ProtectedDomain1_signed.p7s");
  append(props2, DDS::Security::Properties::AccessPermissions, prefix + "permissions/permissions_test_participant_02_signed.p7s");

  DDS::DomainParticipant_var participant2 = dpf->create_participant(domain, participant2_qos, 0, 0);

  TransportConfig_rch config2 = TheTransportRegistry->create_config("config2");
  TransportInst_rch inst2 = TheTransportRegistry->create_inst("inst2", "rtps_udp");
  config2->instances_.push_back(inst2);
  TheTransportRegistry->bind_config(config2, participant2);

  Messenger::MessageTypeSupport_var type_support2 = new Messenger::MessageTypeSupportImpl();
  if (type_support2->register_type(participant2.in(), "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): register_type failed!\n"));
    return EXIT_FAILURE;
  }

  CORBA::String_var type_name2 = type_support2->get_type_name();
  DDS::Topic_var topic2 = participant2->create_topic("Movie Discussion List",
                                                     type_name2.in(),
                                                     TOPIC_QOS_DEFAULT,
                                                     DDS::TopicListener::_nil(),
                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);


  DDS::Publisher_var publisher2 = participant2->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                                 DDS::PublisherListener::_nil(),
                                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataWriter_var datawriter2 = publisher2->create_datawriter(topic2.in(),
                                                                  DATAWRITER_QOS_DEFAULT,
                                                                  DDS::DataWriterListener::_nil(),
                                                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  Messenger::MessageDataWriter_var mdatawriter2 = Messenger::MessageDataWriter::_narrow(datawriter2);

  DDS::Subscriber_var subscriber2 = participant2->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                    DDS::SubscriberListener::_nil(),
                                                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Messenger::Message message;
  message.subject_id = 0;

  ACE_DEBUG((LM_DEBUG, "SCENARIO: check_local_datawriter_register_instance failure\n"));
  custom_plugin->reset_flags();
  custom_plugin->set_return(false, false);
  if (mdatawriter2->register_instance(message) != DDS::HANDLE_NIL) {
    ACE_ERROR((LM_ERROR, "ERROR: check_local_datawriter_register_instance allowed an instance to be registered when not expected\n"));
    return EXIT_FAILURE;
  }
  custom_plugin->wait_for(CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_FALSE);

  ACE_DEBUG((LM_DEBUG, "SCENARIO: check_local_datawriter_register_instance sucess\n"));
  custom_plugin->reset_flags();
  custom_plugin->set_return(true, true);
  const DDS::InstanceHandle_t ih = mdatawriter2->register_instance(message);
  if (ih == DDS::HANDLE_NIL) {
    ACE_ERROR((LM_ERROR, "ERROR: check_local_datawriter_register_instance did not allow an instance to be registered when expected\n"));
    return EXIT_FAILURE;
  }
  custom_plugin->wait_for(CHECK_LOCAL_DATAWRITER_REGISTER_INSTANCE_TRUE);

  ACE_DEBUG((LM_DEBUG, "SCENARIO: check_local_datawriter_dispose_instance failure\n"));
  custom_plugin->reset_flags();
  custom_plugin->set_return(false, false);
  DDS::ReturnCode_t rc = mdatawriter2->dispose(message, ih);
  if (rc != DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY) {
    ACE_ERROR((LM_ERROR, "ERROR: check_local_datawriter_dispose_instance allowed an instance to be dipsosed when not expected\n"));
    return EXIT_FAILURE;
  }
  custom_plugin->wait_for(CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_FALSE);

  ACE_DEBUG((LM_DEBUG, "SCENARIO: check_local_datawriter_dispose_instance success\n"));
  custom_plugin->reset_flags();
  custom_plugin->set_return(true, true);
  if (mdatawriter2->dispose(message, ih) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: check_local_datawriter_dispose_instance did not allow an instance to be disposed when expected\n"));
    return EXIT_FAILURE;
  }
  custom_plugin->wait_for(CHECK_LOCAL_DATAWRITER_DISPOSE_INSTANCE_TRUE);

  // Done with writer2.
  publisher2->delete_datawriter(datawriter2);

  DDS::DataWriter_var datawriter1 = publisher1->create_datawriter(topic1.in(),
                                                                  DATAWRITER_QOS_DEFAULT,
                                                                  DDS::DataWriterListener::_nil(),
                                                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  Messenger::MessageDataWriter_var mdatawriter1 = Messenger::MessageDataWriter::_narrow(datawriter1);

  DDS::DataReaderQos dr_qos;
  subscriber2->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  DDS::DataReader_var reader2 = subscriber2->create_datareader(topic2.in(),
                                                               dr_qos,
                                                               DDS::DataReaderListener::_nil(),
                                                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Utils::wait_match(datawriter1, 1);

  ACE_DEBUG((LM_DEBUG, "SCENARIO: check_remote_datawriter_register_instance failure\n"));
  custom_plugin->reset_flags();
  custom_plugin->set_return(false, false);
  mdatawriter1->write(message, 0);
  custom_plugin->wait_for(CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_FALSE);

  ACE_DEBUG((LM_DEBUG, "SCENARIO: check_remote_datawriter_register_instance success\n"));
  custom_plugin->reset_flags();
  custom_plugin->set_return(true, true);
  mdatawriter1->write(message, 0);
  custom_plugin->wait_for(CHECK_REMOTE_DATAWRITER_REGISTER_INSTANCE_TRUE);

  ACE_DEBUG((LM_DEBUG, "SCENARIO: check_remote_datawriter_dispose_instance failure\n"));
  custom_plugin->reset_flags();
  custom_plugin->set_return(false, false);
  mdatawriter1->dispose(message, 0);
  custom_plugin->wait_for(CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_FALSE);

  ACE_DEBUG((LM_DEBUG, "SCENARIO: check_remote_datawriter_dispose_instance success\n"));
  custom_plugin->reset_flags();
  mdatawriter1->write(message, 0);
  custom_plugin->set_return(true, true);
  mdatawriter1->dispose(message, 0);
  custom_plugin->wait_for(CHECK_REMOTE_DATAWRITER_DISPOSE_INSTANCE_TRUE);

  participant1->delete_contained_entities();
  dpf->delete_participant(participant1);
  participant2->delete_contained_entities();
  dpf->delete_participant(participant2);
  TheServiceParticipant->shutdown();

  return EXIT_SUCCESS;
}
