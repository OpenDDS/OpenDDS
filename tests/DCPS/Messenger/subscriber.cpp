/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include "Args.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/OpenDDSConfigWrapper.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#if OPENDDS_CONFIG_SECURITY
#  include <dds/DCPS/security/framework/Properties.h>
#endif
#include <dds/DCPS/StaticIncludes.h>
#if OPENDDS_DO_MANUAL_STATIC_INCLUDES
#  ifndef OPENDDS_SAFETY_PROFILE
#    include <dds/DCPS/transport/udp/Udp.h>
#    include <dds/DCPS/transport/multicast/Multicast.h>
#    include <dds/DCPS/RTPS/RtpsDiscovery.h>
#    include <dds/DCPS/transport/shmem/Shmem.h>
#    if OPENDDS_CONFIG_SECURITY
#      include <dds/DCPS/security/BuiltInPlugins.h>
#    endif
#  endif
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <cstdlib>

using OpenDDS::DCPS::String;

void append(DDS::PropertySeq& props, const String& name, const String& value, bool propagate = false)
{
  const DDS::Property_t prop = {name.c_str(), value.c_str(), propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int status = EXIT_SUCCESS;

  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    if ((status = parse_args(argc, argv)) != EXIT_SUCCESS) {
      return status;
    }

    DDS::DomainParticipantQos part_qos;
    dpf->get_default_participant_qos(part_qos);

#if OPENDDS_CONFIG_SECURITY
    if (TheServiceParticipant->get_security()) {
      // Determine the path to the keys
      String path_to_tests;
      const char* const source_root = ACE_OS::getenv("OPENDDS_SOURCE_DIR");
      if (source_root && source_root[0]) {
        // Use OPENDDS_SOURCE_DIR in case we are one of the CMake tests
        path_to_tests = String("file:") + source_root + "/tests/";
      } else {
        // Else try to do it relative to the traditional location
        path_to_tests = "file:../../";
      }

      const String certs = path_to_tests + "security/certs/";
      const String identity = certs + "identity/";
      const String messenger = path_to_tests + "DCPS/Messenger/";
      using namespace DDS::Security::Properties;
      DDS::PropertySeq& props = part_qos.property.value;
      append(props, AuthIdentityCA, identity + "identity_ca_cert.pem");
      append(props, AuthIdentityCertificate, identity + "test_participant_02_cert.pem");
      append(props, AuthPrivateKey, identity + "test_participant_02_private_key.pem");
      append(props, AccessPermissionsCA, certs + "permissions/permissions_ca_cert.pem");
      append(props, AccessGovernance, messenger + "governance_signed.p7s");
      append(props, AccessPermissions, messenger + "permissions_2_signed.p7s");
    }
#endif

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(4,
                              part_qos,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_participant() failed!\n"));
      return EXIT_FAILURE;
    }

    // Register Type (Messenger::Message)
    Messenger::MessageTypeSupport_var ts =
      new Messenger::MessageTypeSupportImpl();
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): register_type() failed!\n"));
      return EXIT_FAILURE;
    }

    // Create Topic (Movie Discussion List)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_topic() failed!\n"));
      return EXIT_FAILURE;
    }

    // Create Subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!sub) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_subscriber() failed!\n"));
      return EXIT_FAILURE;
    }

    // Create DataReader
    DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl;
    DDS::DataReaderListener_var listener(listener_servant);

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    if (DataReaderListenerImpl::is_reliable()) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Reliable DataReader\n"));
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      listener_servant->set_expected_reads(40);
    } else {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Unreliable DataReader\n"));
      listener_servant->set_expected_reads(1);
    }

    DDS::GuardCondition_var gc = new DDS::GuardCondition;
    DDS::WaitSet_var ws = new DDS::WaitSet;
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): calling attach_condition\n"));
    DDS::ReturnCode_t ret = ws->attach_condition(gc);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): attach_condition failed!\n"));
      return EXIT_FAILURE;
    }
    listener_servant->set_guard_condition(gc);

    DDS::DataReader_var reader =
      sub->create_datareader(topic.in(),
                             dr_qos,
                             listener.in(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!reader) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_datareader() failed!\n"));
      return EXIT_FAILURE;
    }

    // Block until GuardCondition is released
    DDS::Duration_t timeout =
      { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    ret = ws->wait(conditions, timeout);
    ws->detach_condition(gc);

    if (!listener_servant->is_valid()) {
      status = EXIT_FAILURE;
    }

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
