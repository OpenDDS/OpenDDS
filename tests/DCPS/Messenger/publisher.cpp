/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include "Args.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/framework/Properties.h>
#endif
#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  ifndef OPENDDS_SAFETY_PROFILE
#    include <dds/DCPS/transport/udp/Udp.h>
#    include <dds/DCPS/transport/multicast/Multicast.h>
#    include <dds/DCPS/RTPS/RtpsDiscovery.h>
#    include <dds/DCPS/transport/shmem/Shmem.h>
#    ifdef OPENDDS_SECURITY
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

#ifdef OPENDDS_SECURITY
const char auth_ca_file_from_tests[] = "security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file_from_tests[] = "security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file_from_tests[] = "security/certs/identity/test_participant_01_cert.pem";
const char id_key_file_from_tests[] = "security/certs/identity/test_participant_01_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_1_signed.p7s";
#endif

bool dw_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return gc->instances_[0]->transport_type_ != "udp" &&
         !(gc->instances_[0]->transport_type_ == "multicast" && !gc->instances_[0]->is_reliable());
}

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
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

      DDS::PropertySeq& props = part_qos.property.value;
      append(props, "OpenDDS.RtpsRelay.Groups", "Messenger", true);

#ifdef OPENDDS_SECURITY
      using OpenDDS::DCPS::String;
      // Determine the path to the keys
      String path_to_tests;
      const char* dds_root = ACE_OS::getenv("DDS_ROOT");
      if (dds_root && dds_root[0]) {
        // Use DDS_ROOT in case we are one of the CMake tests
        path_to_tests = String("file:") + dds_root + "/tests/";
      } else {
        // Else if DDS_ROOT isn't defined try to do it relative to the traditional location
        path_to_tests = "file:../../";
      }
      const String auth_ca_file = path_to_tests + auth_ca_file_from_tests;
      const String perm_ca_file = path_to_tests + perm_ca_file_from_tests;
      const String id_cert_file = path_to_tests + id_cert_file_from_tests;
      const String id_key_file = path_to_tests + id_key_file_from_tests;
      if (TheServiceParticipant->get_security()) {
        using namespace DDS::Security::Properties;
        append(props, AuthIdentityCA, auth_ca_file.c_str());
        append(props, AuthIdentityCertificate, id_cert_file.c_str());
        append(props, AuthPrivateKey, id_key_file.c_str());
        append(props, AccessPermissionsCA, perm_ca_file.c_str());
        append(props, AccessGovernance, governance_file);
        append(props, AccessPermissions, permissions_file);
      }
#endif

      // Create DomainParticipant
      participant =
        dpf->create_participant(4,
                                part_qos,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (!participant) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_participant failed!\n")),
                         EXIT_FAILURE);
      }

      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();
      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type failed!\n")),
                         EXIT_FAILURE);
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
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic failed!\n")),
                         EXIT_FAILURE);
      }

      // Create Publisher
      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (!pub) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher failed!\n")),
                         EXIT_FAILURE);
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
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                         EXIT_FAILURE);
      }

      // Start writing threads
      std::cout << "Creating Writer" << std::endl;
      Writer* writer = new Writer(dw.in(), dw_reliable());
      std::cout << "Starting Writer" << std::endl;
      writer->start();

      while (!writer->is_finished()) {
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
      }

      std::cout << "Writer finished " << std::endl;
      writer->end();

      if (dw_reliable()) {
        std::cout << "Writer wait for ACKS" << std::endl;

        const DDS::Duration_t timeout =
          { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
        dw->wait_for_acknowledgments(timeout);
      } else {
        // let any missed multicast/rtps messages get re-delivered
        std::cout << "Writer wait small time" << std::endl;
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
      }

      std::cerr << "deleting DW" << std::endl;
      delete writer;
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
