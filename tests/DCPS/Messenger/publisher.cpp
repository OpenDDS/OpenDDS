/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MessengerTypeSupportImpl.h"
#include "Args.h"

#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>

#include <dds/OpenDDSConfigWrapper.h>

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

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <cstdlib>

bool dw_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return gc->instances_[0]->transport_type_ != "udp" &&
         !(gc->instances_[0]->transport_type_ == "multicast" && !gc->instances_[0]->is_reliable());
}

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
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try {

    std::cout << "Starting publisher" << std::endl;
    {
      // Initialize DomainParticipantFactory
      dpf = TheParticipantFactoryWithArgs(argc, argv);

      std::cout << "Starting publisher with " << argc << " args" << std::endl;
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
        append(props, AuthIdentityCertificate, identity + "test_participant_01_cert.pem");
        append(props, AuthPrivateKey, identity + "test_participant_01_private_key.pem");
        append(props, AccessPermissionsCA, certs + "permissions/permissions_ca_cert.pem");
        append(props, AccessGovernance, messenger + "governance_signed.p7s");
        append(props, AccessPermissions, messenger + "permissions_1_signed.p7s");
      }
#endif

      // Create DomainParticipant
      participant =
        dpf->create_participant(4,
                                part_qos,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (!participant) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_participant failed!\n"));
        return EXIT_FAILURE;
      }

      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();
      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): register_type failed!\n"));
        return EXIT_FAILURE;
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (!topic) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_topic failed!\n"));
        return EXIT_FAILURE;
      }

      // Create Publisher
      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (!pub) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_publisher failed!\n"));
        return EXIT_FAILURE;
      }

      DDS::DataWriterQos qos;
      pub->get_default_datawriter_qos(qos);
      if (dw_reliable()) {
        std::cout << "Reliable DataWriter" << std::endl;
        qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      // Create DataWriter
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in(),
                               qos,
                               DDS::DataWriterListener::_nil(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (!dw) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_datawriter failed!\n"));
        return EXIT_FAILURE;
      }

      // Block until Subscriber is available
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): DataWriter waiting for match\n"));
      if (Utils::wait_match(dw, 1, Utils::EQ)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): Error waiting for match for dw\n"));
        return EXIT_FAILURE;
      }

      std::cout << "Start Writing Samples" << std::endl;

      // Write samples
      Messenger::MessageDataWriter_var message_dw
        = Messenger::MessageDataWriter::_narrow(dw);
      if (!message_dw) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): _narrow failed!\n"));
        return EXIT_FAILURE;
      }

      Messenger::Message message;
      message.subject_id = 99;

      const DDS::InstanceHandle_t handle = message_dw->register_instance(message);

      message.from = "Comic Book Guy";
      message.subject = "Review";
      message.text = "Worst. Movie. Ever.";
      message.count = 0;

      for (size_t i = 0; i < num_messages; i++) {
        DDS::ReturnCode_t error;
        do {
          error = message_dw->write(message, handle);
          if (!dw_reliable()) {
            //spread out unreliable messages some
            ACE_Time_Value small_time(0, 250000);
            ACE_OS::sleep(small_time);
          }
        } while (error == DDS::RETCODE_TIMEOUT);
        if (error != DDS::RETCODE_OK) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: main(): write returned %C!\n", OpenDDS::DCPS::retcode_to_string(error)));
        }
        message.count++;
      }

      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): finished writing messages, waiting for subscriber.\n"));
      // Block until Subscriber goes away
      if (Utils::wait_match(dw, 0, Utils::EQ)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): waiting for unmatch for writer\n"));
        return EXIT_FAILURE;
      }

    }

    // Clean-up!
    std::cerr << "deleting contained entities" << std::endl;
    participant->delete_contained_entities();
    std::cerr << "deleting participant" << std::endl;
    dpf->delete_participant(participant.in());
    std::cerr << "shutdown" << std::endl;
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
